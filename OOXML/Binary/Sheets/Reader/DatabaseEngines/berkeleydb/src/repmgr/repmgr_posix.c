/*-
 * Copyright (c) 2005, 2020 Oracle and/or its affiliates.  All rights reserved.
 *
 * See the file LICENSE for license information.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * Invalid open file descriptor value, that can be used as an out-of-band
 * sentinel to mark our signalling pipe as unopened.
 */
#define	NO_SUCH_FILE_DESC	(-1)

/*
 * Among other things the code in this file is responsible for figuring out
 * the network-event polling method to enable multiplexing of i/o over a set
 * of fds belonging to repmgr's connections. Some older pollling methods are
 * available across the board on almost all platforms but perform poorly in
 * terms of speed and scalability. Others like epoll() are flexible in terms
 * of number of fd's that can be supported in a scalable fashion but are pr-
 * esent on only few of the non-windows platforms.  So here based on availa-
 * bility of a method on the target platform and user input, we decide on a
 * polling method and then that method is used for network event polling.
 * Here we support 3 methods - select(), poll() and epoll().  Select() being
 * one of the oldest is almost omnipresent and poll() is widely supported.
 * Epoll() is present in linux and is highly scalable for larger loads(more
 * than 1000 connections).
 */

/* monitered event types */
typedef enum {
	POLL_WRITE=1,
	POLL_READ=2
} poll_eventtype_t;

/*
 * Aggregated control info needed for preparing
 * for select()/poll()/epoll() call.
 */
typedef struct repmgr_polling_method {
	void *fd_set_info;
	int (*fdlist_add) __P((ENV *, int, void *, poll_eventtype_t));
	int (*fdlist_search) __P((int, void *, poll_eventtype_t));
	int (*fdlist_delete) __P((int, void *));
	void (*fdlist_reset) __P((ENV *, struct repmgr_polling_method *));
	int (*network_event_wait) __P((struct repmgr_polling_method *,
					 db_timespec *));
	int fd_set_size;
	poll_method_t poll_method;
	int read_pending;
} REPMGR_POLLING_METHOD;

/*
 * fd-set info for select()
 */
typedef struct repmgr_select_io_info {
	fd_set *reads, *writes;
	int maxfd;
} REPMGR_SELECT_INFO;

#if defined(HAVE_POLL)
/*
 * fd-set info for poll()
 */
typedef struct repmgr_poll_io_info {
	struct	pollfd	*fd_list;
	int nfds;
	int fd_set_size;
} REPMGR_POLL_INFO;
#endif

#if defined(HAVE_EPOLL)
/*
 * fd-set info for epoll()
 */
typedef struct repmgr_epoll_io_info {
	struct epoll_event *events;
	int nfds;
	int epfd;
} REPMGR_EPOLL_INFO;
#endif

static int __repmgr_conn_work __P((ENV *, REPMGR_CONNECTION *, void *));
static int __repmgr_prepare_io __P((ENV *, REPMGR_CONNECTION *, void *));

/*
 * Starts the thread described in the argument, and stores the resulting thread
 * ID therein.
 *
 * PUBLIC: int __repmgr_thread_start __P((ENV *, REPMGR_RUNNABLE *));
 */
int
__repmgr_thread_start(env, runnable)
	ENV *env;
	REPMGR_RUNNABLE *runnable;
{
	pthread_attr_t *attrp;
	int ret, t_ret;
#if defined(_POSIX_THREAD_ATTR_STACKSIZE) && defined(DB_STACKSIZE)
	pthread_attr_t attributes;
	size_t size;

	attrp = &attributes;
	if ((ret = pthread_attr_init(&attributes)) != 0) {
		__db_err(env, ret, DB_STR("3630",
		    "pthread_attr_init in repmgr_thread_start"));
		return (ret);
	}

	size = DB_STACKSIZE;

#ifdef PTHREAD_STACK_MIN
	if (size < PTHREAD_STACK_MIN)
		size = PTHREAD_STACK_MIN;
#endif
	if ((ret = pthread_attr_setstacksize(&attributes, size)) != 0) {
		__db_err(env, ret, DB_STR("3631",
		    "pthread_attr_setstacksize in repmgr_thread_start"));
		return (ret);
	}
#else
	attrp = NULL;
#endif

	runnable->finished = FALSE;
	runnable->quit_requested = FALSE;
	runnable->env = env;

	ret = pthread_create(&runnable->thread_id, attrp,
		    runnable->run, runnable);

	if (attrp != NULL) {
		t_ret = pthread_attr_destroy(attrp);
		if (t_ret != 0) {
			__db_err(env, ret, DB_STR("3712",
			    "pthread_attr_destroy in repmgr_thread_start"));
			if (ret == 0)
				ret = t_ret;
		}
	}
	return (ret);
}

/*
 * PUBLIC: int __repmgr_thread_join __P((REPMGR_RUNNABLE *));
 */
int
__repmgr_thread_join(thread)
	REPMGR_RUNNABLE *thread;
{
	return (pthread_join(thread->thread_id, NULL));
}

/*
 * PUBLIC: int __repmgr_set_nonblock_conn __P((REPMGR_CONNECTION *));
 */
int
__repmgr_set_nonblock_conn(conn)
	REPMGR_CONNECTION *conn;
{
	return (__repmgr_set_nonblocking(conn->fd));
}

/*
 * PUBLIC: int __repmgr_set_nonblocking __P((socket_t));
 */
int
__repmgr_set_nonblocking(fd)
	socket_t fd;
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		return (errno);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return (errno);
	return (0);
}

/*
 * PUBLIC: int __repmgr_wake_waiters __P((ENV *, waiter_t *));
 *
 * Wake any "waiter" threads (either sending threads waiting for acks, or
 * channel users waiting for response to request).
 *
 * !!!
 * Caller must hold the db_rep->mutex, if this thread synchronization is to work
 * properly.
 */
int
__repmgr_wake_waiters(env, waiter)
	ENV *env;
	waiter_t *waiter;
{
	COMPQUIET(env, NULL);
	return (pthread_cond_broadcast(waiter));
}

/*
 * Waits a limited time for a condition to become true.  (If the limit is 0 we
 * wait forever.)  All calls share just the one db_rep->mutex, but use whatever
 * waiter_t the caller passes us.
 *
 * PUBLIC: int __repmgr_await_cond __P((ENV *,
 * PUBLIC:     PREDICATE, void *, db_timeout_t, waiter_t *));
 */
int
__repmgr_await_cond(env, pred, ctx, timeout, wait_condition)
	ENV *env;
	PREDICATE pred;
	void *ctx;
	db_timeout_t timeout;
	waiter_t *wait_condition;
{
	DB_REP *db_rep;
	struct timespec deadline;
	int ret, timed;

	db_rep = env->rep_handle;
	if ((timed = (timeout > 0)))
		__repmgr_compute_wait_deadline(env, &deadline, timeout);
	else
		COMPQUIET(deadline.tv_sec, 0);

	while (!(*pred)(env, ctx)) {
		if (timed)
			ret = pthread_cond_timedwait(wait_condition,
			    db_rep->mutex, &deadline);
		else
			ret = pthread_cond_wait(wait_condition, db_rep->mutex);
		if (db_rep->repmgr_status == stopped)
			return (DB_REP_UNAVAIL);
		if (ret == ETIMEDOUT)
			return (DB_TIMEOUT);
		if (ret != 0)
			return (ret);
	}
	return (0);
}

/*
 * Waits for an in-progress membership DB operation (if any) to complete.
 *
 * PUBLIC: int __repmgr_await_gmdbop __P((ENV *));
 *
 * Caller holds mutex; we drop it while waiting.
 */
int
__repmgr_await_gmdbop(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret;

	db_rep = env->rep_handle;
	while (db_rep->gmdb_busy)
		if ((ret = pthread_cond_wait(&db_rep->gmdb_idle,
		    db_rep->mutex)) != 0)
			return (ret);
	return (0);
}

/*
 * __repmgr_compute_wait_deadline --
 *	Computes a deadline time a certain distance into the future.
 *
 * PUBLIC: void __repmgr_compute_wait_deadline __P((ENV*,
 * PUBLIC:    struct timespec *, db_timeout_t));
 */
void
__repmgr_compute_wait_deadline(env, result, wait)
	ENV *env;
	struct timespec *result;
	db_timeout_t wait;
{
	/*
	 * The result is suitable for the pthread_cond_timewait call.  (That
	 * call uses nano-second resolution; elsewhere we use microseconds.)
	 *
	 * Start with "now"; then add the "wait" offset.
	 *
	 * A db_timespec is the same as a "struct timespec" so we can pass
	 * result directly to the underlying Berkeley DB OS routine.
	 *
	 * !!!
	 * We use the system clock for the pthread_cond_timedwait call, but
	 * that's not optimal on systems with monotonic timers.   Instead,
	 * we should call pthread_condattr_setclock on systems where it and
	 * monotonic timers are available, and then configure both this call
	 * and the subsequent pthread_cond_timewait call to use a monotonic
	 * timer.
	 */
	__os_gettime(env, (db_timespec *)result, 0);
	TIMESPEC_ADD_DB_TIMEOUT(result, wait);
}

/*
 * PUBLIC: int __repmgr_await_drain __P((ENV *,
 * PUBLIC:    REPMGR_CONNECTION *, db_timeout_t));
 *
 * Waits for space to become available on the connection's output queue.
 * Various ways we can exit:
 *
 * 1. queue becomes non-full
 * 2. exceed time limit
 * 3. connection becomes defunct (due to error in another thread)
 * 4. repmgr is shutting down
 * 5. any unexpected system resource failure
 *
 * In cases #3 and #5 we return an error code.  Caller is responsible for
 * distinguishing the remaining cases if desired, though we do help with #2 by
 * showing the connection as congested.
 *
 * !!!
 * Caller must hold repmgr->mutex.
 */
int
__repmgr_await_drain(env, conn, timeout)
	ENV *env;
	REPMGR_CONNECTION *conn;
	db_timeout_t timeout;
{
	DB_REP *db_rep;
	struct timespec deadline;
	int ret;

	db_rep = env->rep_handle;

	__repmgr_compute_wait_deadline(env, &deadline, timeout);

	ret = 0;
	while (conn->out_queue_length >= OUT_QUEUE_LIMIT) {
		ret = pthread_cond_timedwait(&conn->drained,
		    db_rep->mutex, &deadline);
		switch (ret) {
		case 0:
			if (db_rep->repmgr_status == stopped)
				goto out; /* #4. */
			/*
			 * Another thread could have stumbled into an error on
			 * the socket while we were waiting.
			 */
			if (conn->state == CONN_DEFUNCT) {
				ret = DB_REP_UNAVAIL; /* #3. */
				goto out;
			}
			break;
		case ETIMEDOUT:
			conn->state = CONN_CONGESTED;
			ret = 0;
			goto out; /* #2. */
		default:
			goto out; /* #5. */
		}
	}
	/* #1. */

out:
	return (ret);
}

/*
 * PUBLIC: int __repmgr_alloc_cond __P((cond_var_t *));
 *
 * Initialize a condition variable (in allocated space).
 */
int
__repmgr_alloc_cond(c)
	cond_var_t *c;
{
	return (pthread_cond_init(c, NULL));
}

/*
 * PUBLIC: int __repmgr_free_cond __P((cond_var_t *));
 *
 * Clean up a previously initialized condition variable.
 */
int
__repmgr_free_cond(c)
	cond_var_t *c;
{
	return (pthread_cond_destroy(c));
}

/*
 * PUBLIC: void __repmgr_env_create_pf __P((DB_REP *));
 */
void
__repmgr_env_create_pf(db_rep)
	DB_REP *db_rep;
{
	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;
}

/*
 * "Platform"-specific mutex creation function.
 *
 * PUBLIC: int __repmgr_create_mutex_pf __P((mgr_mutex_t *));
 */
int
__repmgr_create_mutex_pf(mutex)
	mgr_mutex_t *mutex;
{
	return (pthread_mutex_init(mutex, NULL));
}

/*
 * PUBLIC: int __repmgr_destroy_mutex_pf __P((mgr_mutex_t *));
 */
int
__repmgr_destroy_mutex_pf(mutex)
	mgr_mutex_t *mutex;
{
	return (pthread_mutex_destroy(mutex));
}

/*
 * PUBLIC: int __repmgr_init __P((ENV *));
 */
int
__repmgr_init(env)
	ENV *env;
{
	DB_REP *db_rep;
	struct sigaction sigact;
	int ack_inited, elect_inited, file_desc[2], gmdb_inited, queue_inited;
	int ret;

	db_rep = env->rep_handle;

	/*
	 * Make sure we're not ignoring SIGPIPE, 'cuz otherwise we'd be killed
	 * just for trying to write onto a socket that had been reset.  Note
	 * that we don't undo this in case of a later error, since we document
	 * that we leave the signal handling state like this, even after env
	 * close.
	 */
	if (sigaction(SIGPIPE, NULL, &sigact) == -1) {
		ret = errno;
		__db_err(env, ret, DB_STR("3632",
		    "can't access signal handler"));
		return (ret);
	}
	if (sigact.sa_handler == SIG_DFL) {
		sigact.sa_handler = SIG_IGN;
		sigact.sa_flags = 0;
		if (sigaction(SIGPIPE, &sigact, NULL) == -1) {
			ret = errno;
			__db_err(env, ret, DB_STR("3632",
			    "can't access signal handler"));
			return (ret);
		}
	}

	ack_inited = elect_inited = gmdb_inited = queue_inited = FALSE;
	if ((ret = __repmgr_init_waiters(env, &db_rep->ack_waiters)) != 0)
		goto err;
	ack_inited = TRUE;

	if ((ret = pthread_cond_init(&db_rep->check_election, NULL)) != 0)
		goto err;
	elect_inited = TRUE;

	if ((ret = pthread_cond_init(&db_rep->gmdb_idle, NULL)) != 0)
		goto err;
	gmdb_inited = TRUE;

	if ((ret = pthread_cond_init(&db_rep->msg_avail, NULL)) != 0)
		goto err;
	queue_inited = TRUE;

	if ((ret = pipe(file_desc)) == -1) {
		ret = errno;
		goto err;
	}

	db_rep->read_pipe = file_desc[0];
	db_rep->write_pipe = file_desc[1];
	return (0);
err:
	if (queue_inited)
		(void)pthread_cond_destroy(&db_rep->msg_avail);
	if (gmdb_inited)
		(void)pthread_cond_destroy(&db_rep->gmdb_idle);
	if (elect_inited)
		(void)pthread_cond_destroy(&db_rep->check_election);
	if (ack_inited)
		(void)__repmgr_destroy_waiters(env, &db_rep->ack_waiters);
	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;

	return (ret);
}

/*
 * PUBLIC: int __repmgr_deinit __P((ENV *));
 */
int
__repmgr_deinit(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret, t_ret;

	db_rep = env->rep_handle;

	if (!(REPMGR_INITED(db_rep)))
		return (0);

	ret = pthread_cond_destroy(&db_rep->msg_avail);

	if ((t_ret = pthread_cond_destroy(&db_rep->gmdb_idle)) != 0 &&
	    ret == 0)
		ret = t_ret;

	if ((t_ret = pthread_cond_destroy(&db_rep->check_election)) != 0 &&
	    ret == 0)
		ret = t_ret;

	if ((t_ret = __repmgr_destroy_waiters(env,
	    &db_rep->ack_waiters)) != 0 && ret == 0)
		ret = t_ret;

	if (close(db_rep->read_pipe) == -1 && ret == 0)
		ret = errno;
	if (close(db_rep->write_pipe) == -1 && ret == 0)
		ret = errno;

	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;
	return (ret);
}

/*
 * PUBLIC: int __repmgr_init_waiters __P((ENV *, waiter_t *));
 */
int
__repmgr_init_waiters(env, waiters)
	ENV *env;
	waiter_t *waiters;
{
	COMPQUIET(env, NULL);
	return (pthread_cond_init(waiters, NULL));
}

/*
 * PUBLIC: int __repmgr_destroy_waiters __P((ENV *, waiter_t *));
 */
int
__repmgr_destroy_waiters(env, waiters)
	ENV *env;
	waiter_t *waiters;
{
	COMPQUIET(env, NULL);
	return (pthread_cond_destroy(waiters));
}

/*
 * PUBLIC: int __repmgr_lock_mutex __P((mgr_mutex_t *));
 */
int
__repmgr_lock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	return (pthread_mutex_lock(mutex));
}

/*
 * PUBLIC: int __repmgr_unlock_mutex __P((mgr_mutex_t *));
 */
int
__repmgr_unlock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	return (pthread_mutex_unlock(mutex));
}

/*
 * Signals a condition variable.
 *
 * !!!
 * Caller must hold mutex.
 *
 * PUBLIC: int __repmgr_signal __P((cond_var_t *));
 */
int
__repmgr_signal(v)
	cond_var_t *v;
{
	return (pthread_cond_broadcast(v));
}

/*
 * Wake repmgr message processing threads, expressly for the purpose of shutting
 * some subset of them down.
 *
 * !!!
 * Caller must hold mutex.
 *
 * PUBLIC: int __repmgr_wake_msngers __P((ENV*, u_int));
 */
int
__repmgr_wake_msngers(env, n)
	ENV *env;
	u_int n;
{
	DB_REP *db_rep;

	COMPQUIET(n, 0);

	db_rep = env->rep_handle;
	return (__repmgr_signal(&db_rep->msg_avail));
}

/*
 * PUBLIC: int __repmgr_wake_main_thread __P((ENV*));
 *
 * Can be called either with or without the mutex being held.
 */
int
__repmgr_wake_main_thread(env)
	ENV *env;
{
	DB_REP *db_rep;
	u_int8_t any_value;

	COMPQUIET(any_value, 0);
	db_rep = env->rep_handle;

	/*
	 * It doesn't matter what byte value we write.  Just the appearance of a
	 * byte in the stream is enough to wake up the select() thread reading
	 * the pipe.
	 */
	if (write(db_rep->write_pipe, (void *) &any_value, 1) == -1)
		return (errno);
	return (0);
}

/*
 * PUBLIC: int __repmgr_writev __P((socket_t, db_iovec_t *, int, size_t *));
 */
int
__repmgr_writev(fd, iovec, buf_count, byte_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *byte_count_p;
{
	int nw, result;

	if ((nw = writev(fd, iovec, buf_count)) == -1) {
		/* Why?  See note at __repmgr_readv(). */
		result = errno;
		DB_ASSERT(NULL, result != 0);
		return (result);
	}
	*byte_count_p = (size_t)nw;
	return (0);
}

/*
 * PUBLIC: int __repmgr_readv __P((socket_t, db_iovec_t *, int, size_t *));
 */
int
__repmgr_readv(fd, iovec, buf_count, byte_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *byte_count_p;
{
	int result;
	ssize_t nw;

	if ((nw = readv(fd, iovec, buf_count)) == -1) {
		/*
		 * Why bother to assert this obvious "truth"?  On some systems
		 * when the library is loaded into a single-threaded Tcl
		 * configuration the differing errno mechanisms apparently
		 * conflict, and we occasionally "see" a 0 value here!  And that
		 * turns out to be painful to debug.
		 */
		result = errno;
		DB_ASSERT(NULL, result != 0);
		return (result);
	}
	*byte_count_p = (size_t)nw;
	return (0);
}

/*
 * __repmgr_prepare_io --
 *
 * Examines a connection to see what sort of I/O to ask for.  Clean up defunct
 * connections.
 */
static int
__repmgr_prepare_io(env, conn, handler_info)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *handler_info;
{
	REPMGR_POLLING_METHOD *info;
	int type;
	int ret;

	info = handler_info;
	type = 0;

	if (conn->state == CONN_DEFUNCT) {
		SSL_DEBUG_SHUTDOWN(env, "Conn_defunct detected.");

		if ((ret = info->fdlist_delete(conn->fd,
		    info->fd_set_info)) != 0) {
			return ret;
		}
		return (__repmgr_cleanup_defunct(env, conn));
	}

	if (!STAILQ_EMPTY(&conn->outbound_queue)) {
		type |= POLL_WRITE;
		if ((ret = info->fdlist_add(env, conn->fd,
		    info->fd_set_info, type)) != 0)
			return (ret);
	}
	/*
	 * For now we always accept incoming data.  If we ever implement some
	 * kind of flow control, we should override it for fledgling connections
	 * (!IS_VALID_EID(conn->eid)) -- in other words, allow reading such a
	 * connection even during flow control duress.
	 */
	type |= POLL_READ;

	if ((ret = info->fdlist_add(env, conn->fd,
	    info->fd_set_info, type)) != 0)
		return (ret);

	return (0);
}

/*
 * __repmgr_conn_work --
 *
 * Examine a connection, to see what work needs to be done.
 */
static int
__repmgr_conn_work(env, conn, handler_info)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *handler_info;
{
	REPMGR_POLLING_METHOD *info;
	int ret;
	int fd;
	int read_allowed;
	int write_allowed;
	int use_ssl_api;

	ret = 0;
	fd = conn->fd;
	info = handler_info;
	use_ssl_api = 0;

#if defined(HAVE_REPMGR_SSL)
	if (IS_REPMGR_SSL_ENABLED(env))
		use_ssl_api = 1;
#endif

	if (conn->state == CONN_DEFUNCT)
	{
		return (0);
	}

	read_allowed = info->fdlist_search(fd, info->fd_set_info, POLL_READ);
	write_allowed = info->fdlist_search(fd, info->fd_set_info, POLL_WRITE);

	if (use_ssl_api == 1) {
#if defined(HAVE_REPMGR_SSL)
		if (__repmgr_ssl_write_possible(conn, read_allowed,
		    write_allowed) == 1)
			ret = __repmgr_write_some(env, conn);

		if (ret == 0 && (__repmgr_ssl_read_possible(conn, read_allowed,
		    write_allowed) == 1))
			ret = __repmgr_read_from_site(env, conn);

		if (conn && conn->repmgr_ssl_info->ssl
		    && SSL_pending(conn->repmgr_ssl_info->ssl)) {
			if (!(conn->repmgr_ssl_info->ssl_io_state
			    & (REPMGR_SSL_READ_PENDING_ON_READ
			    | REPMGR_SSL_READ_PENDING_ON_WRITE)))
				info->read_pending |= 1;
		}	
#endif
	} else {
		if (write_allowed)
			ret = __repmgr_write_some(env, conn);

		if (ret == 0 && read_allowed)
			ret = __repmgr_read_from_site(env, conn);
	}

	if (ret == DB_REP_UNAVAIL)
		ret = __repmgr_bust_connection(env, conn);

	return (ret);
}

/*
 * __repmgr_event_poll_loop --
 *
 * This function takes care of the commonalities between select(), poll()
 * and epoll() with respect to connection-fd management and issuing actions
 * for the fds. The difference between the different polling methods lie
 * in the way they add, delete connection-fds and the way they wait for
 * events. We call this function once the suitable functions are plugged
 * into handler_info for the selected method. Then this function invokes
 * those subroutines as needed for the aforementioned tasks of connection
 * fd management.
 */
static int
__repmgr_event_poll_loop(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	DB_REP *db_rep;
	void *info;
	db_timespec timeout;
	db_timespec *wait_timeout;
	int ret;
	int nfds;
	u_int8_t buf[10];	/* arbitrary size */

	db_rep = env->rep_handle;
	info = handler_info->fd_set_info;

	/*
	 * Almost this  entire thread  operates while holding the mutex.
	 * But note that it never blocks, except in the call to select()
	 * (which is the one place we relinquish the mutex).
	 */
	LOCK_MUTEX(db_rep->mutex);
	if ((ret = __repmgr_first_try_connections(env)) != 0)
		goto cleanup;

	for (;;) {
		handler_info->fdlist_reset(env, handler_info);

		if ((ret = handler_info->fdlist_add(env,
		    db_rep->read_pipe, info, POLL_READ)) != 0)
			goto cleanup;

		if (!IS_SUBORDINATE(db_rep)) {
			/* For master moniter the listener fd too. */
			if ((ret = handler_info->fdlist_add(env,
			    db_rep->listen_fd, info, POLL_READ)) != 0)
			goto cleanup;
		}

		if ((ret = __repmgr_each_connection(env, __repmgr_prepare_io,
		    handler_info, TRUE)) != 0)
			goto cleanup;

#if defined(HAVE_REPMGR_SSL)
		if (IS_REPMGR_SSL_ENABLED(env)) {
			if (handler_info->read_pending) {
				handler_info->fdlist_reset(env, handler_info);
				handler_info->read_pending = 0;
				goto process_io;
			}
		}
#endif

		if (__repmgr_compute_timeout(env, &timeout))
			wait_timeout = &timeout;
		else {
			/* No time-based events, so wait only for I/O. */
			wait_timeout = NULL;
		}

		UNLOCK_MUTEX(db_rep->mutex);

		if ((nfds = handler_info->network_event_wait(handler_info,
		    wait_timeout)) == -1) {
			/* wait funcs return -1 on error and sets errno. */
			switch (ret = errno) {
			case EINTR:
			case EWOULDBLOCK:
				LOCK_MUTEX(db_rep->mutex);
				continue; /* simply retry */
			default:
				__db_err(env, ret, DB_STR("0167",
				    "network_event_wait"));
				return (ret);
			}
		}

		LOCK_MUTEX(db_rep->mutex);
		if (db_rep->repmgr_status == stopped) {
			ret = 0;
			goto cleanup;
		}

		/*
		 * Timer expiration events include retrying of lost connections.
		 * Obviously elements can be added to the connection list there.
		 */
		if ((ret = __repmgr_check_timeouts(env)) != 0)
			goto cleanup;

		/*
		 * In case of  no events and  event-wait timing out,  we should
		 * not return or continue before checking heartbeat from master
		 * (__repmgr_check_timeouts )  or  repmgr_status.  Otherwise we
		 * might get  stuck here forever. (only  in  absence of any new
		 * events)
		 */
		if (nfds == 0)
			continue;

process_io:
		if ((ret = __repmgr_each_connection(env, __repmgr_conn_work,
		    handler_info, TRUE)) != 0)
			goto cleanup;

		/*
		 * Read any bytes in the  signaling pipe.  Note that we don't
		 * actually need to do anything with them; they're just there
		 * to wake us up when necessary.
		 */
		if (handler_info->fdlist_search(
		    db_rep->read_pipe, info, POLL_READ) &&
		    read(db_rep->read_pipe, (void *) buf, sizeof(buf)) <= 0) {
			ret = errno;
			goto cleanup;
		}

		/*
		 * Obviously elements can be added to the connection list here.
		 */
		if (!IS_SUBORDINATE(db_rep) &&
		    handler_info->fdlist_search(db_rep->listen_fd,
		    info, POLL_READ) &&
		    (ret = __repmgr_accept(env)) != 0) {
			goto cleanup;
		}
	}

cleanup:
	UNLOCK_MUTEX(db_rep->mutex);

	if (ret == DB_DELETED)
		ret = __repmgr_bow_out(env);

	LOCK_MUTEX(db_rep->mutex);
	(void)__repmgr_net_close(env);
	UNLOCK_MUTEX(db_rep->mutex);

	return (ret);
}

#if defined(HAVE_EPOLL)
/*
 * __repmgr_epoll_event_wait --
 *
 * Polls and delivers notification for epoll() method.
 */
static int
__repmgr_epoll_event_wait(handler_info, timeout)
	REPMGR_POLLING_METHOD *handler_info;
	db_timespec *timeout;
{
	int epoll_timeout;
	REPMGR_EPOLL_INFO *info;
	int maxfdSetSize;
	int ret;

	maxfdSetSize = handler_info->fd_set_size;
	info = handler_info->fd_set_info;

	if (timeout) {
		/* timeout in milliseconds */
		epoll_timeout = (timeout->tv_sec * 1000) +
		    (timeout->tv_nsec / NS_PER_MS);
	} else
		epoll_timeout = -1;

	/*
	 * On error, -1 is returned and errno is set. This errno is processed
	 * in __repmgr_event_poll_loop.
	 */
	if ((ret = epoll_wait(info->epfd, info->events, maxfdSetSize,
	    epoll_timeout)) != -1) {
		/*
		 * On success, update io_info with event_info available for
		 * processing.
		 */
		info->nfds = ret;
	}

	return (ret);
}

/*
 * __repmgr_epoll_fdlist_reset --
 *
 * Supposed to reset fd-set for epoll.  Not needed for epoll().
 */
static void
__repmgr_epoll_fdlist_reset(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	/* NO-OP for epoll */
	COMPQUIET(env, NULL);
	COMPQUIET(handler_info, NULL);
}

/*
 * __repmgr_epoll_fdlist_search --
 *
 * Checks if the supplied fd is present in the fd-set returned by
 * epoll_wait. Return true only if the event notification matches
 * the type supplied(READ/WRITE).
 */
static int
__repmgr_epoll_fdlist_search(conn_fd, info_, type)
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_EPOLL_INFO *info;
	int ret;
	int i;

	info = info_;
	ret = 0;

	for (i = 0; i < info->nfds; i++) {
		if (info->events[i].data.fd == conn_fd) {
			if (type & POLL_WRITE)
				ret |= info->events[i].events & EPOLLOUT;

			if (type & POLL_READ)
				ret |= info->events[i].events & EPOLLIN;

			/*
			 * We have single record for a fd in  epoll-set. This
			 * record is updated with both  read/write flags. So if
			 * we found a record with matching fd, we can break.
			 */
			break;
		}
	}

	return (ret);
}

/*
 * __repmgr_epoll_fdlist_delete --
 *
 * Removes the supplied fd from the monitered/registered fd-set.
 */
static int
__repmgr_epoll_fdlist_delete(conn_fd, info_)
	int conn_fd;
	void *info_;
{
	REPMGR_EPOLL_INFO *info;
	struct epoll_event ev;

	info = info_;
	memset(&ev, 0, sizeof(struct epoll_event));

	ev.data.fd = conn_fd;

	/*
	 * Deregister the fd from epoll set, since we know that the
	 * corresponding connection is defunct.
	 */
	epoll_ctl(info->epfd, EPOLL_CTL_DEL, conn_fd, &ev);

	/*
	 * !!!
	 * Since closing a file descriptor causes it to be removed from all
	 * epoll sets automatically, return success even if some error was
	 * encountered.
	 * Any error here is not worth closing the repmgr.
	 */
	return (0);
}

/*
 * __repmgr_epoll_fdlist_add --
 *
 * Depending on request type registers the supplied fd with epoll-set
 * via epoll_ctl.
 */
static int
__repmgr_epoll_fdlist_add(env, conn_fd, info_, type)
	ENV *env;
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_EPOLL_INFO *info;
	struct epoll_event ev;
	int ret;

	COMPQUIET(env, NULL);
	info = info_;

	memset(&ev, 0, sizeof(struct epoll_event));
	ev.data.fd = conn_fd;

	if (type & POLL_WRITE)
		ev.events |= EPOLLOUT;

	if (type & POLL_READ)
		ev.events |= EPOLLIN;

	/* If fd already exists in the epoll_set then modify it. */
	if ((ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, conn_fd, &ev)) == -1) {
		if ((ret = epoll_ctl(info->epfd,
		    EPOLL_CTL_MOD, conn_fd, &ev)) != 0)
			ret = errno;
	}

	return (ret);
}

/*
 * __repmgr_epoll_exec --
 *
 * Main function for epoll() method, where we
 *	-setup event_handler info (function pts for different
 *		actions).
 *	-invoke the common functions to poll and notify about
 *		network i/o events/readiness-state.
 */
static int
__repmgr_epoll_exec(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	REPMGR_EPOLL_INFO *info;
	struct epoll_event *events;
	int epfd;

	int ret;
	int maxfdSetSize;

	/* Initializations. */
	maxfdSetSize = handler_info->fd_set_size;
	info = NULL;
	events = NULL;

	/* Install functions corresponding to epoll. */
	handler_info->fdlist_add = __repmgr_epoll_fdlist_add;
	handler_info->fdlist_search = __repmgr_epoll_fdlist_search;
	handler_info->fdlist_delete = __repmgr_epoll_fdlist_delete;
	handler_info->fdlist_reset = __repmgr_epoll_fdlist_reset;
	handler_info->network_event_wait = __repmgr_epoll_event_wait;
	handler_info->poll_method = EPOLL;

	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_EPOLL_INFO),
	    &handler_info->fd_set_info)) != 0) {
		__db_err(env, ret, DB_STR("3716",
		    "memory allocation for  epoll_info failed"));

		goto cleanup;
	}

	info = handler_info->fd_set_info;

	/* Allocate and clear the epoll-fd-set. */
	if ((ret = __os_calloc(env, maxfdSetSize,
	    sizeof(struct epoll_event), &events)) != 0) {
		__db_errx(env, DB_STR_A("3723",
		    "failed to create epoll_fd_set for fd_set_size=%d","%d"),
		    maxfdSetSize);

		goto cleanup;
	}

	if ((epfd = epoll_create(maxfdSetSize)) == -1) {
		__db_errx(env, DB_STR_A("3722",
		    "epoll_create() failed for fd_set_size=%d","%d"),
		    maxfdSetSize);

		goto cleanup;
	}

	info->epfd = epfd;
	info->nfds = 0;
	info->events = events;

	/* Invoke the event polling func decided above. */
	ret = __repmgr_event_poll_loop(env, handler_info);

cleanup:
	/* Get rid of allocated memory earlier in this function. */
	if (events)
		__os_free(env, events);

	if (handler_info->fd_set_info)
		__os_free(env, handler_info->fd_set_info);

	return (ret);
}
#endif

#if defined(HAVE_POLL)
/*
 * __repmgr_poll_event_wait --
 *
 * Polls and delivers notification for poll() method.
 */
static int
__repmgr_poll_event_wait(handler_info, timeout)
	REPMGR_POLLING_METHOD *handler_info;
	db_timespec *timeout;
{
	int poll_timeout;
	REPMGR_POLL_INFO *info;
	int ret;

	info = handler_info->fd_set_info;

	if (timeout) {
		/* timeout in milliseconds */
		poll_timeout = (timeout->tv_sec * 1000) +
		    (timeout->tv_nsec / NS_PER_MS);
	} else
		poll_timeout = -1;

	/*
	 * On error, -1 is returned and errno is set. This errno is processed
	 * in __repmgr_event_poll_loop.
	 */
	ret = poll(info->fd_list, info->nfds, poll_timeout);

	return (ret);
}

/*
 * __repmgr_poll_fdlist_reset --
 *
 * Resets fd_set array for poll(), so it can be used again for subsequent
 * poll().
 */
static void
__repmgr_poll_fdlist_reset(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	int i;
	REPMGR_POLL_INFO *info;

	info = handler_info->fd_set_info;

	/*
	 * Reset revents so these array records can be reused by a subsequent
	 * poll
	 */
	for (i = 0; i < info->nfds; i++) {
		if (info->fd_list[i].revents != 0) {
			info->fd_list[i].revents = 0;
		}
	}

	COMPQUIET(env, NULL);
}

/*
 * __repmgr_poll_fdlist_delete --
 *
 * Removes the supplied fd from the monitered fd-set.
 */
static int
__repmgr_poll_fdlist_delete(conn_fd, info_)
	int conn_fd;
	void *info_;
{
	REPMGR_POLL_INFO *info;
	int i;

	info = info_;

	for (i = 0; i < info->nfds; i++) {
		if (info->fd_list[i].fd == conn_fd) {
			/* Now this location can be used for another fd */
			info->fd_list[i].fd = 0;
			memset(&info->fd_list[i], 0, sizeof(REPMGR_POLL_INFO));
		}
	}

	return (0);
}

/*
 * __repmgr_poll_fdlist_search --
 *
 * Checks if the supplied fd received an event (indicated by type).
 */
static int
__repmgr_poll_fdlist_search(conn_fd, info_, type)
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_POLL_INFO *info;
	int ret;
	int i;

	info = info_;
	ret = 0;

	for (i = 0; i < info->nfds; i++) {
		if (info->fd_list[i].fd == conn_fd) {
			/*
			 * NOTE :: we may have more than one entry for
			 * same fd for POLL.
			 */
			if (type & POLL_WRITE)
				ret |= info->fd_list[i].revents & POLLOUT;

			if (type & POLL_READ)
				ret |= info->fd_list[i].revents &
				    (POLLIN | POLLPRI);

			/*
			 * fd-array can have atmost 2 records for each fd
			 * If we found the record we were looking for,
			 * then rest of records belong to other fds or
			 * different io-type. So break.
			 */
			if (ret)
				break;
		}
	}

	return (ret);
}

/*
 * __repmgr_poll_fdlist_resize --
 *
 * doubles the size of the fd_list for poll.
 */
static int
__repmgr_poll_fdlist_resize(env, info)
	ENV *env;
	REPMGR_POLL_INFO *info;
{
	int ret;
	int new_size;
	int old_size;

	new_size = 2 * info->fd_set_size;
	old_size = info->fd_set_size;

	/* double the size of the poll_fd_array using __os_realloc. */
	if ((ret = __os_realloc(env, new_size * sizeof(REPMGR_POLL_INFO),
	    &info->fd_list)) == 0) {
		/*
		 * Need to memzero the newly allocated portion.  Note that
		 * nfds does not represent the true size of current fd
		 * array It's fd_set_size that represents the true size.
		 */
		memset(&info->fd_list[old_size], 0,
		    sizeof(REPMGR_POLL_INFO) * (new_size - old_size));

		info->fd_set_size *= 2;
	} else {
		__db_err(env, ret, DB_STR("3720",
		    "Too many connection fds for poll and memory allocation for"
		    " poll-fd-array failed. Try restarting repmgr after "
		    "configuring select or epoll with rep_set_config"));
	}

	return (ret);
}

/*
 * __repmgr_poll_fdlist_add --
 *
 * Depending on request type puts the supplied fd in the array of fd's
 * monitered by poll.
 */
static int
__repmgr_poll_fdlist_add(env, conn_fd, info_, type)
	ENV *env;
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_POLL_INFO *info;
	int i;
	int ret;
	int fd_found;

	ret = EXIT_FAILURE;
	fd_found = 0;
	info = info_;

	/*
	 * Check if we are already watching for this 'type' of event
	 * for conn_fd.
	 */
	for (i = 0; i < info->nfds; i++) {
		if (info->fd_list[i].fd == conn_fd) {
			/*
			 * NOTE :: we may have more than one entry for
			 * same fd for POLL.
			 */
			if (type & POLL_WRITE)
				fd_found |= info->fd_list[i].events & POLLOUT;

			if (type & POLL_READ)
				fd_found |= info->fd_list[i].events &
				    (POLLIN | POLLPRI);

			if (fd_found)
				break;
		}
	}

	/* Add an entry for conn_fd if one doesn't exist already. */
	if (!fd_found) {
		/*
		 * Double fd_array if we have almost filled it up. Note
		 * that its safe to modify fd_array outside of poll().
		 */
		if ((info->nfds + 1) >= info->fd_set_size) {
			ret = __repmgr_poll_fdlist_resize(env, info);

			if (ret)
				return (ret);
		}

		for (i = 0; i < info->nfds + 1; i++) {
			if (info->fd_list[i].fd <= 0) {
				/* Found a free slot in fd-array. */
				info->fd_list[i].fd = conn_fd;

				if (type & POLL_READ)
					info->fd_list[i].events |=
					    (POLLIN | POLLPRI);
				else if (type & POLL_WRITE)
					info->fd_list[i].events |= POLLOUT;

				ret = 0;

				break;
			}
		}

		if (i >= info->nfds)
			info->nfds++;
	} else
		ret = 0;

	return (ret);
}

/*
 * __repmgr_poll_exec --
 *
 * Main function for poll() method, where we
 *	-setup event_handler info (function pts for different
 *		actions).
 *	-invoked the common function to poll and notify about
 *		network i/o events/readiness-state.
 */
static int
__repmgr_poll_exec(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	REPMGR_POLL_INFO *info;
	int ret;
	int maxfdSetSize;

	/* Initializations */
	info = NULL;
	maxfdSetSize = handler_info->fd_set_size;

	/* Install functions corresponding to poll(). */
	handler_info->fdlist_add = __repmgr_poll_fdlist_add;
	handler_info->fdlist_search = __repmgr_poll_fdlist_search;
	handler_info->fdlist_delete = __repmgr_poll_fdlist_delete;
	handler_info->fdlist_reset = __repmgr_poll_fdlist_reset;
	handler_info->network_event_wait = __repmgr_poll_event_wait;
	handler_info->poll_method = POLL;

	/* allocate space for poll management info. */
	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_POLL_INFO),
	    &handler_info->fd_set_info)) != 0) {
		__db_err(env, ret, DB_STR("3717",
		    "memory allocation for  poll_info failed"));
		goto cleanup;
	}

	info = handler_info->fd_set_info;
	info->fd_set_size = handler_info->fd_set_size;

	/* Allocate and clear the fd-array. */
	if ((ret = __os_calloc(env, maxfdSetSize,
	    sizeof(struct pollfd), &info->fd_list)) != 0) {
		__db_err(env, ret, DB_STR("3721",
		    "Failed to allocate fd_list for poll"));
		goto cleanup;
	}

	info->nfds = 0;

	/* Invoke the event polling func decided above */
	ret = __repmgr_event_poll_loop(env, handler_info);

cleanup:
	/* Get rid of allocated memory earlier in this function. */
	if (info && info->fd_list)
		__os_free(env, info->fd_list);

	if (handler_info->fd_set_info)
		__os_free(env, handler_info->fd_set_info);

	return (ret);
}
#endif

/*
 * __repmgr_select_event_wait --
 *
 * Polls and delivers notification for select() method.
 */
static int
__repmgr_select_event_wait(handler_info, timeout)
	REPMGR_POLLING_METHOD *handler_info;
	db_timespec *timeout;
{
	int ret;
	struct timeval *select_timeout_p,select_timeout;
	REPMGR_SELECT_INFO *info;

	info = handler_info->fd_set_info;

	if (timeout) {
		/* Convert the timespec to a timeval. */
		select_timeout.tv_sec = timeout->tv_sec;
		select_timeout.tv_usec = timeout->tv_nsec / NS_PER_US;
		select_timeout_p = &select_timeout;
	} else
		select_timeout_p = NULL;

	/*
	 * On error, -1 is returned and errno is set. This errno is processed
	 * in __repmgr_event_poll_loop
	 */
	ret = select(info->maxfd + 1, info->reads, info->writes,
	    NULL, select_timeout_p);

	return (ret);
}

/*
 * __repmgr_select_fdlist_reset --
 *
 * Resets read/write fd_sets for select(), so they can be reused for subsequent
 * select calls.
 */
static void
__repmgr_select_fdlist_reset(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	REPMGR_SELECT_INFO *info;

	COMPQUIET(env, NULL);
	info = handler_info->fd_set_info;

	FD_ZERO(info->reads);
	FD_ZERO(info->writes);
}

/*
 * __repmgr_select_fdlist_delete --
 *
 * Removes the supplied fd from the monitered fd-set no-op for select. This
 * is taken care of by logic in __repmgr_event_poll_loop.
 */
static int
__repmgr_select_fdlist_delete(conn_fd, info_)
	int conn_fd;
	void *info_;
{
	/*
	 * NO-OP -> FD_ZERO in the reset func of select loop takes care of
	 * removing fd's associated with dead connections.
	 */
	COMPQUIET(conn_fd, 0);
	COMPQUIET(info_, NULL);

	return (0);
}

/*
 * __repmgr_select_fdlist_search --
 *
 * Checks if the supplied fd received an event (indicated by type).
 */
static int
__repmgr_select_fdlist_search(conn_fd, info_, type)
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_SELECT_INFO *info;
	int ret;

	info = info_;
	ret = 0;

	if (type & POLL_WRITE)
		ret = FD_ISSET((u_int)conn_fd, info->writes);

	if (type & POLL_READ)
		ret = FD_ISSET((u_int)conn_fd, info->reads);

	return (ret);
}

/*
 * __repmgr_select_fdlist_add --
 *
 * Depending on request type adds the supplied fd to the read/write fd set.
 */
static int
__repmgr_select_fdlist_add(env, conn_fd, info_, type)
	ENV *env;
	int conn_fd;
	void *info_;
	poll_eventtype_t type;
{
	REPMGR_SELECT_INFO *info;

	info = info_;

	if (conn_fd >= FD_SETSIZE) {
		__db_errx(env, DB_STR_A("3729",
		    "Select does not support fd >= %d on this system."
		    "Use rep_set_config to enable poll or epoll."
		    ,"%d"), FD_SETSIZE);

		return (EINVAL);
	}

	if (type & POLL_WRITE)
		FD_SET((u_int)conn_fd, info->writes);

	if (type & POLL_READ)
		FD_SET((u_int)conn_fd, info->reads);

	if (conn_fd > info->maxfd)
		info->maxfd = conn_fd;

	return (0);
}

/*
 * __repmgr_select_exec --
 *
 * Main function for select() method, where we
 *	-setup event_handler info (function pts for different
 *		actions).
 *	-invoked the common function to poll and notify about
 *		network i/o events/readiness-state.
 */
static int
__repmgr_select_exec(env, handler_info)
	ENV *env;
	REPMGR_POLLING_METHOD *handler_info;
{
	REPMGR_SELECT_INFO *info;
	fd_set *reads, *writes;
	int ret;

	/* Init */
	reads = NULL;
	writes = NULL;

	/* Install functions corresponding to select() */
	handler_info->fdlist_add = __repmgr_select_fdlist_add;
	handler_info->fdlist_search = __repmgr_select_fdlist_search;
	handler_info->fdlist_delete = __repmgr_select_fdlist_delete;
	handler_info->fdlist_reset = __repmgr_select_fdlist_reset;
	handler_info->network_event_wait = __repmgr_select_event_wait;
	handler_info->poll_method = SELECT;
	handler_info->read_pending = 0;

	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_SELECT_INFO),
	    &handler_info->fd_set_info)) != 0) {
		__db_err(env, ret, DB_STR("3718",
		    "memory allocation for  select_info failed"));
		goto cleanup;
	}

	if ((ret = __os_calloc(env, 1, sizeof(fd_set), &reads)) != 0) {
		__db_err(env, ret, DB_STR("3725",
		    "memory allocation for read_fd_set failed"));
		goto cleanup;
	}

	if ((ret = __os_calloc(env, 1, sizeof(fd_set), &writes)) != 0) {
		__db_err(env, ret, DB_STR("3726",
		    "memory allocation for write_fd_set failed"));
		goto cleanup;
	}

	info = handler_info->fd_set_info;
	info->reads = reads;
	info->writes = writes;

	/* Invoke the event polling func decided above */
	ret = __repmgr_event_poll_loop(env, handler_info);

cleanup:
	if (writes)
		__os_free(env, writes);

	if (reads)
		__os_free(env, reads);

	if (handler_info->fd_set_info)
		__os_free(env, handler_info->fd_set_info);

	return (ret);
}

/*
 * __repmgr_network_event_handler --
 *
 * Identifies the event notification method to be used and then invokes
 * the corresponding function. The decision is made based on libraries
 * available and preference stated by user(select/poll/epoll) in terms
 * of rep_config flags.
 *
 * PUBLIC: int __repmgr_network_event_handler __P((ENV *));
 */
int
__repmgr_network_event_handler(env)
	ENV	*env;
{
	DB_REP *db_rep;
	REP *rep;

	REPMGR_POLLING_METHOD *handler_info;
	int ret;
	poll_method_t method;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	handler_info = NULL;

	/* allocate the structure for handler_info */
	if ((ret = __os_calloc(env, 1,
	    sizeof(REPMGR_POLLING_METHOD), &handler_info)) != 0) {
		__db_err(env, ret, DB_STR("3724",
		    "memory allocation for network io handler failed"));
		return (ret);
	}

	/* set the fd-set-size to REPMGR_FD_SET_DEFAULT_SIZE */
	handler_info->fd_set_size = REPMGR_FD_SET_DEFAULT_SIZE;

	/*
	 * select the  event-polling  method based on user supplied flags and
	 * availability of required header files.  Select()  is  available on
	 * all platforms and is chosen only when poll/epoll are not available
	 * or poll is explicitly disabled.  If Poll() is available and is not
	 * explicitly  disabled  using  DB_REPMGR_CONF_DISABLE_POLL,  it will
	 * be given preference over select.  If user expects  large number of
	 * connections  they  may explicitly  enable  epoll for  its superior
	 * performance and scalability. If its available it will be choosen.
	 */
#if defined(HAVE_EPOLL)
	if (FLD_ISSET(rep->config, REP_C_ENABLE_EPOLL))
		method = EPOLL;
	else
#endif
	{
		method = SELECT;

#if defined(HAVE_POLL)
		if (!FLD_ISSET(rep->config, REP_C_DISABLE_POLL))
			method = POLL;
#endif
	}

	/* Add method information to stats to be used for testing. */
	STAT(rep->mstat.st_polling_method = method);

	/*
	 * Based on the method selected above and availability of
	 * epoll.h/poll.h, choose to execute the corresponding method
	 */
	switch (method) {
#if defined(HAVE_EPOLL)
	case EPOLL:
		if ((ret = __repmgr_epoll_exec(env, handler_info)) != 0)
			goto cleanup;

		break;
#endif
#if defined(HAVE_POLL)
	case POLL:
		if ((ret = __repmgr_poll_exec(env, handler_info)) != 0)
			goto cleanup;

		break;
#endif
	case SELECT:
	default :
		if ((ret = __repmgr_select_exec(env, handler_info)) != 0)
			goto cleanup;

		break;
	}

	/*
	 * Thread  is done. So free up all the memory allocated so far.
	 */
cleanup:
	if (handler_info)
		__os_free(env, handler_info);

	return (ret);
}

/*-
 * Copyright (c) 1997, 2020 Oracle and/or its affiliates.  All rights reserved.
 *
 * See the file LICENSE for license information.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_FLOCK
#include <sys/file.h>
#endif

#if !defined(HAVE_FCNTL) || !defined(HAVE_FLOCK)
static int __os_filelocking_notsup __P((ENV *));
#endif

/*
 * __os_fdlock --
 *	Acquire/release a lock on a byte in a file.
 *
 *	The lock modes supported here are:
 *	DB_LOCK_NG	- release the lock
 *	DB_LOCK_READ	- get shared access
 *	DB_LOCK_WRITE	- get exclusive access
 *
 *	In practice, the file is one of:
 *		__db.register	DB_REGISTER's REGISTER_FILE, or
 *		__db.001	the first region file
 *
 *	If the input offset is negative, then lock the whole file instead of
 *	just a single byte.
 *
 *	Use fcntl()-like semantics most of the time (DB_REGISTER support). Fcntl
 *	supports range locking, but has the additional broken semantics that
 *	closing any of the file's descriptors releases any locks, even if its
 *	other file descriptors remain open. Thanks SYSV & POSIX.
 *	However, if the offset is negative (which is allowed, because POSIX
 *	off_t a signed integer) then use flock() instead.  It has only whole-
 *	file locks, but they persist until explicitly unlocked or the process
 *	exits.
 *
 * PUBLIC: int __os_fdlock __P((ENV *, DB_FH *, off_t, db_lockmode_t, int));
 */
int
__os_fdlock(env, fhp, offset, lockmode, nowait)
	ENV *env;
	DB_FH *fhp;
	off_t offset;
	db_lockmode_t lockmode;
	int nowait;
{
#if defined(HAVE_FCNTL) || defined(HAVE_FLOCK)
	DB_ENV *dbenv;
	int ret, t_ret;
	static char *mode_string[DB_LOCK_WRITE + 1] = {
		"unlock",
		"read",
		"write"
	};
#ifdef HAVE_FCNTL
	struct flock fl;
	static short mode_fcntl[DB_LOCK_WRITE + 1] = {
		F_UNLCK,
		F_RDLCK,
		F_WRLCK
	};
#endif
#ifdef HAVE_FLOCK
	static short mode_flock[DB_LOCK_WRITE + 1] = {
		LOCK_UN,
		LOCK_SH,
		LOCK_EX
	};
#endif

	dbenv = env == NULL ? NULL : env->dbenv;

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);
	DB_ASSERT(env, lockmode <= DB_LOCK_WRITE);

	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL)) {
		if (offset < 0)
		      __db_msg(env, DB_STR_A("5510",
			  "fileops: flock %s %s %s", "%s %s %s"),
			  fhp->name, mode_string[lockmode],
			  nowait ? "nowait" : "");
	      else
		      __db_msg(env, DB_STR_A("0020",
			  "fileops: fcntls %s %s offset %lu", "%s %s %lu"),
			  fhp->name, mode_string[lockmode], (u_long)offset);
	}

	if (offset < 0) {
#ifdef HAVE_FLOCK
		RETRY_CHK_EINTR_ONLY(flock(fhp->fd,
		    mode_flock[lockmode] | (nowait ? LOCK_NB : 0)), ret);
#else
		ret = __os_filelocking_notsup(env);
#endif
	} else {
#ifdef HAVE_FCNTL
		fl.l_start = offset;
		fl.l_len = 1;
		fl.l_whence = SEEK_SET;
		fl.l_type = mode_fcntl[lockmode];
		   RETRY_CHK_EINTR_ONLY(
		    fcntl(fhp->fd, nowait ? F_SETLK : F_SETLKW, &fl), ret);
#else
		ret = __os_filelocking_notsup(env);
#endif
	}

	if (offset < 0 && dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("5511",
		    "fileops: flock %s %s %s returns %s", "%s %s %s"),
		    fhp->name, mode_string[lockmode],
		    nowait ? "nowait" : "", db_strerror(ret));

	if (ret == 0)
		return (0);

	t_ret = USR_ERR(env, __os_posix_err(ret));
	if (t_ret != EACCES && t_ret != EAGAIN)
		__db_syserr(env, ret, DB_STR("0139", "fcntl"));
	return (t_ret);
#else
	COMPQUIET(fhp, NULL);
	COMPQUIET(lockmode, 0);
	COMPQUIET(nowait, 0);
	COMPQUIET(offset, 0);
	return (__os_filelocking_notsup(env));
#endif
}

#if !defined(HAVE_FCNTL) || !defined(HAVE_FLOCK)
/*
 * __os_filelocking_notsup --
 *	Generate an error message if fcntl() or flock() is requested on a
 *	platform that does not support it.
 *
 */
static int
__os_filelocking_notsup(env)
	ENV *env;
{
	__db_syserr(env, DB_OPNOTSUP, DB_STR("0140",
	    "advisory file locking unavailable"));
	return (DB_OPNOTSUP);
}
#endif

/*-
 * Copyright (c) 1997, 2020 Oracle and/or its affiliates.  All rights reserved.
 *
 * See the file LICENSE for license information.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_fdlock --
 *	Acquire/release a lock on a either one byte or most of a file.
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
 *	If the input offset is negative, then lock essentially the whole file
 *	by using a byte count of nearly INT64_MAX instead of just 1.
 */
int
__os_fdlock(env, fhp, offset, lockmode, nowait)
	ENV *env;
	DB_FH *fhp;
	off_t offset;
	db_lockmode_t lockmode;
	int nowait;
{
#ifdef DB_WINCE
	/*
	 * This functionality is not supported by WinCE, so just fail.
	 *
	 * Should only happen if an app attempts to open an environment
	 * with the DB_REGISTER flag.
	 */
	 __db_errx(env, DB_STR("0019",
	    "fdlock API not implemented for WinCE, DB_REGISTER "
	    "environment flag not supported."));
	return (EFAULT);
#else
	DWORD lowoff, highoff;
	DB_ENV *dbenv;
	OVERLAPPED over;
	DWORD lowcount, highcount, lockflags;
	static char *mode_string[DB_LOCK_WRITE + 1] = {
		"unlock",
		"read",
		"write"
	};
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	DB_ASSERT(env,
	    F_ISSET(fhp, DB_FH_OPENED) && fhp->handle != INVALID_HANDLE_VALUE);
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

	/*
	 * Windows file locking interferes with read/write operations, so we
	 * map the ranges to an area past the end of the file. The mapping
	 * starts from UINT64_MAX and goes down, backwards, towards INT64_MAX.
	 */
	if (offset < 0) {
		offset = INT64_MAX;
		lowcount = 0;
		highcount = (DWORD)(INT64_MAX >> 32);
	} else {
		DB_ASSERT(env, offset < (u_int64_t)INT64_MAX);
		offset = UINT64_MAX - offset;
		lowcount = 1;
		highcount = 0;
	}
	lowoff = (DWORD)offset;
	highoff = (DWORD)(offset >> 32);

	if (lockmode == DB_LOCK_NG) {
		RETRY_CHK_EINTR_ONLY(!UnlockFile(fhp->handle,
		    lowoff, highoff, lowcount, highcount), ret);
	} else {
		lockflags = 0;
		if (lockmode == DB_LOCK_WRITE)
			lockflags = LOCKFILE_EXCLUSIVE_LOCK;
		if (nowait)
			lockflags |= LOCKFILE_FAIL_IMMEDIATELY;
		memset(&over, 0, sizeof(over));
		over.Offset = lowoff;
		over.OffsetHigh = highoff;
		RETRY_CHK_EINTR_ONLY(!LockFileEx(fhp->handle,
		    lockflags, 0, lowcount, highcount, &over), ret);
	}

	/* Record lock not granted conditions, as well as real errors. */
	if (ret != 0)
		ret = USR_ERR(env, __os_posix_err(ret));

	return (ret);
#endif
}

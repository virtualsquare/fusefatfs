/**
 * Copyright (c) 2020 Renzo Davoli <renzo@cs.unibo.it>
 *
 * This program  is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define FUSE_USE_VERSION 29

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>
#include <time.h>
#include <stddef.h>
#include <pthread.h>

#include <ff.h>
#include <fftable.h>
#include <config.h>

int fuse_reentrant_tag = 0;

static pthread_mutex_t fff_mutex = PTHREAD_MUTEX_INITIALIZER;
#define mutex_in() pthread_mutex_lock(&fff_mutex)
#define mutex_out() pthread_mutex_unlock(&fff_mutex)
#define mutex_out_return(RETVAL) do {mutex_out(); return(RETVAL); } while (0)

#define fffpath(index, path) \
  *fffpath; \
  ssize_t __fffpathlen = (index == 0) ? 0 : strlen(path) + 3; \
  char __fffpath[__fffpathlen]; \
  if (index != 0) { \
    snprintf(__fffpath, __fffpathlen, "%d:%s", index, path); \
    fffpath = __fffpath; \
  } else \
    fffpath = path

static int fr2errno(FRESULT fres) {
	switch (fres) {
		case FR_OK: return 0;
		case FR_NO_FILE:
		case FR_NO_PATH:
								return -ENOENT;
		case FR_INVALID_NAME:
		case FR_INVALID_PARAMETER:
								return -EINVAL;
		case FR_DENIED:
								return -EACCES;
		case FR_WRITE_PROTECTED:
								return -EROFS;
		case FR_EXIST:
								return -EEXIST;
		case FR_NOT_ENOUGH_CORE:
								return -ENOMEM;
		default:
								return -EIO;
	}
}

static BYTE flags2ffmode(int flags) {
	// O_RDONLY -> FA_READ, O_WRONLY -> FA_WRITE, O_RDWR -> FA_READ | FA_WRITE
	BYTE ffmode = ((flags & O_ACCMODE) + 1) & O_ACCMODE;
	if (flags & O_CREAT) {
		if (flags & O_EXCL)
			ffmode |= FA_CREATE_NEW;
		else if (flags & O_TRUNC)
			ffmode |= FA_CREATE_ALWAYS;
		else
			ffmode |= FA_OPEN_ALWAYS;
	}
	if (flags & O_APPEND) {
		ffmode |= FA_OPEN_APPEND;
	}
	return ffmode;
}

static time_t fftime2time(WORD fdate, WORD ftime) {
	if (fdate == 0 && ftime == 0)
		return 0;
	else {
		struct tm tm = {0};

		tm.tm_year = ((fdate >> 9) & 0x7f) + 80;
		tm.tm_mon = ((fdate >> 5) & 0xf) - 1;
		tm.tm_mday = fdate & 0x1f;

		tm.tm_hour = (ftime >> 11) & 0x1f;
		tm.tm_min = (ftime >> 5) & 0x3f;
		tm.tm_sec = (ftime & 0x1f) * 2;

		return mktime(&tm);
	}
}

static int fff_getattr(const char *path, struct stat *stbuf)
{
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	FRESULT fres;
	// f_stat path: The object must not be the root directory */
	if (strcmp(path, "/") == 0) {
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_mode = 0755 | S_IFDIR;
		stbuf->st_nlink = 2;
		mutex_out_return(0);
	} else {
		const char fffpath(ffentry->index, path);
		FILINFO fileinfo;
		fres = f_stat(fffpath, &fileinfo);
		//printf("getattr %s %s -> %d\n", path, fffpath, fres);
		if (fres != FR_OK) goto err;
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_size = fileinfo.fsize;
		stbuf->st_ctime = stbuf->st_mtime =
			fftime2time(fileinfo.fdate, fileinfo.ftime);
		if (fileinfo.fattrib & AM_DIR) {
			stbuf->st_mode = 0755 | S_IFDIR;
			stbuf->st_nlink = 2;
		} else {
			stbuf->st_nlink = 1;
			stbuf->st_mode = 0755 | S_IFREG;
		}
		if (fileinfo.fattrib & AM_RDO)
			stbuf->st_mode &= ~0222;
	}
	mutex_out_return(0);
err:
	mutex_out_return(fr2errno(fres));
}

static int fff_open(const char *path, struct fuse_file_info *fi){
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if ((ffentry->flags & FFFF_RDONLY) && (fi->flags & O_ACCMODE) != O_RDONLY)
		mutex_out_return(-EROFS);
	FIL fp;
	FRESULT fres = f_open(&fp, fffpath, flags2ffmode(fi->flags));
	if (fres == FR_OK)
		f_close(&fp);
	mutex_out_return(fr2errno(fres));
}

static int fff_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	(void) fi;
	(void) mode; // XXX set readonly?
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FIL fp;
	FRESULT fres = f_open(&fp, fffpath, flags2ffmode(fi->flags | O_CREAT));
	if (fres == FR_OK)
		f_close(&fp);
	mutex_out_return(fr2errno(fres));
}

static int fff_release(const char *path, struct fuse_file_info *fi){
	(void) path;
	(void) fi;
	return 0;
}

static int fff_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	FIL fp;
	UINT br;
	FRESULT fres = f_open(&fp, fffpath, flags2ffmode(fi->flags));
	if (fres != FR_OK)
		goto earlyerr;
	fres = f_lseek(&fp, offset);
	if (fres != FR_OK) goto err;
	fres = f_read(&fp, buf, size, &br);
	if (fres != FR_OK) goto err;
	f_close(&fp);
	mutex_out_return(br);
err:
	f_close(&fp);
earlyerr:
	mutex_out_return(fr2errno(fres));
}

static int fff_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	FIL fp;
	UINT bw;
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FRESULT fres = f_open(&fp, fffpath, flags2ffmode(fi->flags));
	if (fres != FR_OK)
		goto earlyerr;
	fres = f_lseek(&fp, offset);
	if (fres != FR_OK) goto err;
	fres = f_write(&fp, buf, size, &bw);
	if (fres != FR_OK) goto err;
	fres = f_sync(&fp);
	if (fres != FR_OK) goto err;
	f_close(&fp);
	mutex_out_return(bw);
err:
	f_close(&fp);
earlyerr:
	mutex_out_return(fr2errno(fres));
}

static int fff_opendir(const char *path, struct fuse_file_info *fi){
	(void) fi;
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	DIR dp;
	FRESULT fres = f_opendir(&dp, fffpath);
	f_closedir(&dp);
	mutex_out_return(fr2errno(fres));
}

static int fff_releasedir(const char *path, struct fuse_file_info *fi){
	(void) path;
	(void) fi;
	return 0;
}

static int fff_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi){
	(void) offset;
	(void) fi;
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	DIR dp;
	FRESULT fres = f_opendir(&dp, fffpath);
	if (fres != FR_OK)
		goto mutexout_leave;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	while(1) {
		FILINFO fileinfo;
		fres = f_readdir(&dp, &fileinfo);
		if (fres != FR_OK) break;
		if (fileinfo.fname[0] == 0) break;
		filler(buf, fileinfo.fname, NULL, 0);
	}
	f_closedir(&dp);
mutexout_leave:
	mutex_out_return(fr2errno(fres));
}

static int fff_mkdir(const char *path, mode_t mode) {
	(void) mode;  // XXX set readonly
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FRESULT fres = f_mkdir(fffpath);
	if (fres != FR_OK) return fr2errno(fres);
	// XXX mode?
	mutex_out_return(0);
}

static int fff_unlink(const char *path) {
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	// XXX ck is it reg file ?
	FRESULT fres = f_unlink(fffpath);
	mutex_out_return(fr2errno(fres));
}

static int fff_rmdir(const char *path) {
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	// XXX ck is it a dir ?
	FRESULT fres = f_unlink(fffpath);
	mutex_out_return(fr2errno(fres));
}

static int fff_rename(const char *path, const char *newpath) {
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FRESULT fres = f_rename(fffpath, newpath);
	mutex_out_return(fr2errno(fres));
}

static int fff_truncate(const char *path, off_t size) {
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
	struct fftab *ffentry = cntx->private_data;
	const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FIL fp;
	memset(&fp, 0, sizeof(fp));
	FRESULT fres = f_open(&fp, fffpath, FA_WRITE);
	if (fres != FR_OK) goto openerr;
	fres = f_lseek(&fp, size);
	if (fres != FR_OK) goto err;
	fres = f_truncate(&fp);
	if (fres != FR_OK) goto err;
	fres = f_close(&fp);
openerr:
	mutex_out_return(fr2errno(fres));
err:
	f_close(&fp);
	mutex_out_return(fr2errno(fres));
}

static int fff_utimens(const char *path, const struct timespec tv[2]) {
	mutex_in();
	struct fuse_context *cntx=fuse_get_context();
  struct fftab *ffentry = cntx->private_data;
  const char fffpath(ffentry->index, path);
	if (ffentry->flags & FFFF_RDONLY)
		mutex_out_return(-EROFS);
	FILINFO fno;
	struct tm tm;
	time_t newtime = tv[1].tv_sec;
	if (localtime_r(&newtime, &tm) == NULL)
		mutex_out_return(-EINVAL);
	fno.fdate =
		/* bit15:9: Year origin from the 1980 (0..127, e.g. 37 for 2017) */
		(((tm.tm_year - 80) & 0x7f) << 9) |
		/* bit8:5: Month (1..12) */
		(((tm.tm_mon + 1) & 0xf) << 5) |
		/* bit4:0: Day of the month (1..31) */
		(tm.tm_mday & 0x1f);
	fno.ftime =
		/* bit15:11: Hour (0..23)) */
		((tm.tm_hour & 0x1f) << 11) |
		/* bit10:5: Minute (0..59) */
		((tm.tm_min & 0x3f) << 5) |
		/* bit4:0 Second / 2 (0..29, e.g. 25 for 50) */
		((tm.tm_sec & 0x3f) / 2);
	FRESULT fres = f_utime(fffpath, &fno);
	mutex_out_return(fr2errno(fres));
}

static struct fftab *fff_init(const char *source, int flags) {
	int index = fftab_new(source, flags);
	if (index >= 0) {
		struct fftab *ffentry = fftab_get(index);
		char sdrv[12];
		snprintf(sdrv, 12, "%d:", index);
		FRESULT fres = f_mount(&ffentry->fs, sdrv, 1);
		if (fres != FR_OK) {
			fftab_del(index);
			return NULL;
		}
		return ffentry;
	} else
		return NULL;
}

static void fff_destroy(struct fftab *ffentry) {
	char sdrv[12];
	snprintf(sdrv, 12, "%d:", ffentry->index);
	f_mount(0, sdrv, 1);
	fftab_del(ffentry->index);
}

static const struct fuse_operations fusefat_ops = {
	.getattr  = fff_getattr,
	.open           = fff_open,
	.create         = fff_create,
	.read           = fff_read,
	.write          = fff_write,
	.release        = fff_release,
	.opendir        = fff_opendir,
	.readdir        = fff_readdir,
	.releasedir     = fff_releasedir,
	.mkdir          = fff_mkdir,
	.unlink         = fff_unlink,
	.rmdir          = fff_rmdir,
	.rename         = fff_rename,
	.truncate       = fff_truncate,
	.utimens        = fff_utimens,
};

static void usage(void)
{
	fprintf(stderr,
			"usage: " PROGNAME " image mountpoint [options]\n"
			"\n"
			"general options:\n"
			"    -o opt,[opt...]    mount options\n"
			"    -h   --help        print help\n"
			"    -V   --version     print version\n"
			"\n"
			PROGNAME " options:\n"
			"    -o ro     disable write support\n"
			"    -o rw+    enable write support\n"
			"    -o rw     enable write support only together with -force\n"
			"    -o force  enable write support only together with -rw\n"
			"\n"
			"    this software is still experimental\n"
			"\n");
}

struct options {
	const char *source;
	const char *mountpoint;
	int ro;
	int rw;
	int rwplus;
	int force;
};

#define FFF_OPT(t, p, v) { t, offsetof(struct options, p), v }

static struct fuse_opt fff_opts[] =
{
	FFF_OPT("ro", ro, 1),
	FFF_OPT("rw", rw, 1),
	FFF_OPT("rw+", rwplus, 1),
	FFF_OPT("force", force, 1),

	FUSE_OPT_KEY("-V", 'V'),
	FUSE_OPT_KEY("--version", 'V'),
	FUSE_OPT_KEY("-h", 'h'),
	FUSE_OPT_KEY("--help", 'h'),
	FUSE_OPT_END
};

	static int
fff_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	struct options *options = data;
	switch(key) {
		case FUSE_OPT_KEY_OPT:
			return 1;
		case FUSE_OPT_KEY_NONOPT:
			if (!options->source) {
				options->source = arg;
				return 0;
			} else if(!options->mountpoint) {
				options->mountpoint = arg;
				return 1;
			} else
				return -1;
			break;
		case 'h':
			usage();
			fuse_opt_add_arg(outargs, "-ho");
			fuse_main(outargs->argc, outargs->argv, &fusefat_ops, NULL);
			return -1;

		case 'V':
			fprintf(stderr, PROGNAME " version %s\n", VERSION);
			fuse_opt_add_arg(outargs, "--version");
			fuse_main(outargs->argc, outargs->argv, &fusefat_ops, NULL);
			return -1;

		default:
			return -1;
	}
}

int main(int argc, char *argv[])
{
	int err;
	struct options options = {0};
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fftab *ffentry;
	int flags = 0;
	struct stat sbuf;
	if (fuse_opt_parse(&args, &options, fff_opts, fff_opt_proc) == -1) {
		fuse_opt_free_args(&args);
		return -1;
	}
	if (options.rw == 0 && options.rwplus == 0)
		options.ro = 1;
	if (options.rw == 1 && options.force == 0) {
		fprintf(stderr,
				"The file system will be mounted in read-only mode.\n"
				"This is still experimental code.\n"
				"The option to mount in read-write mode is: -o rw+\n"
				"or: -o rw,force\n\n");
		options.ro = 1;
	}

	if (options.source == NULL || options.mountpoint == NULL) {
		usage();
		goto returnerr;
	}

	if (stat(options.source, &sbuf) < 0) {
		fprintf(stderr, "%s: %s\n", options.source, strerror(errno));
		goto returnerr;
	}

	if (! S_ISREG(sbuf.st_mode) && ! S_ISBLK(sbuf.st_mode)) {
		fprintf(stderr, "%s: source must be a block device or a regular file (image)\n", options.source);
		goto returnerr;
	}

	if (options.ro) flags |= FFFF_RDONLY;
	if ((ffentry = fff_init(options.source, flags)) == NULL) {
		fprintf(stderr, "Fuse init error\n");
		goto returnerr;
	}
	err = fuse_main(args.argc, args.argv, &fusefat_ops, ffentry);
	fff_destroy(ffentry);
	fuse_opt_free_args(&args);
	if (err) fprintf(stderr, "Fuse error %d\n", err);
	return err;
returnerr:
	fuse_opt_free_args(&args);
	return -1;
}

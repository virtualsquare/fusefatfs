/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "fftable.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	(void) pdrv;

	return 0;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	struct fftab *drv = fftab_get(pdrv);
	if (!drv) return STA_NOINIT;

	if (drv->flags & FFFF_RDONLY)
		drv->fd = open(drv->path, O_RDONLY);
	else
		drv->fd = open(drv->path, O_SYNC|O_RDWR);
	if (drv->fd < 0)
		return STA_NOINIT;
	
	return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	struct fftab *drv = fftab_get(pdrv);
	WORD ssize;
	//printf("disk_read %d %p\n", pdrv, drv);
  if (!drv) return RES_PARERR;
#if FF_MAX_SS != FF_MIN_SS
	ssize = drv.fs->ssize;
#else
	ssize = FF_MIN_SS;
#endif
	//printf("disk_read %d\n", drv->fd);
	if (lseek(drv->fd, sector * ssize, SEEK_SET) < 0)
		return RES_ERROR;
	if (read(drv->fd, buff, count * ssize) != count * ssize)
		return RES_ERROR;

	res = RES_OK;
	return res;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	struct fftab *drv = fftab_get(pdrv);
  WORD ssize;
  if (!drv) return RES_PARERR;
#if FF_MAX_SS != FF_MIN_SS
  ssize = drv.fs->ssize;
#else
  ssize = FF_MIN_SS;
#endif
	if (drv->flags & FFFF_RDONLY)
		return RES_WRPRT;
  if (lseek(drv->fd, sector * ssize, SEEK_SET) < 0)
    return RES_ERROR;
  if (write(drv->fd, buff, count * ssize) != count * ssize)
    return RES_ERROR;

  res = RES_OK;
  return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	struct fftab *drv = fftab_get(pdrv);
  if (!drv) return RES_PARERR;

	switch (cmd) {
		case CTRL_SYNC:
			return RES_OK;
		case GET_SECTOR_SIZE:
#if FF_MAX_SS != FF_MIN_SS
			*((WORD*)buff) = drv.fs->ssize;
#else
			*((WORD*)buff) = FF_MIN_SS;
#endif
			return RES_OK;
	}
	return RES_PARERR;
}

DWORD get_fattime() 
{ 
	struct tm tm;
	time_t now = time(NULL);
	if (localtime_r(&now, &tm) != NULL) {
		DWORD fattime =
			/* bit31:25: Year origin from the 1980 (0..127, e.g. 37 for 2017) */
			(((tm.tm_year - 80) & 0x7f) << 25) |
			/* bit24:21: Month (1..12) */
			(((tm.tm_mon + 1) & 0xf) << 21) |
			/* bit20:16: Day of the month (1..31) */
			((tm.tm_mday & 0x1f) << 16) |
			/* bit15:11: Hour (0..23)) */
			((tm.tm_hour & 0x1f) << 11) |
			/* bit10:5: Minute (0..59) */
			((tm.tm_min & 0x3f) << 5) |
			/* bit4:0 Second / 2 (0..29, e.g. 25 for 50) */
			((tm.tm_sec & 0x3f) / 2);
		//printf("get_fattime %x\n", fattime);
		return fattime;
	} else
		return 1; 
}

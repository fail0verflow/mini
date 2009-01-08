/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "sdhc.h"
#include <string.h>

static sdhci_t sdhci;
static u8 buffer[512] MEM2_BSS ALIGNED(32);

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	s32 ret;

	sd_init(&sdhci, 0);
	ret = sd_mount(&sdhci);
	if(ret < 0)
		return STA_NOINIT;
	else
		return disk_status(drv);
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	if(sd_inserted(&sdhci) == 0)
		return STA_NODISK;
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	int i;
	DRESULT res;

	res = RES_OK;
	for(i = 0; i < count; i++)
	{
		if(sd_read(&sdhci, sector + i, 1, buffer) != 0)
		{
			res = RES_ERROR;
			break;
		}
		memcpy(buff + i * 512, buffer, 512);
	}
	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	int i;
	DRESULT res;

	res = RES_OK;
	for(i = 0; i < count; i++)
	{
		memcpy(buffer, buff + i * 512, 512);
		if(sd_write(&sdhci, sector + i, 1, buffer) != 0)
		{
			res = RES_ERROR;
			break;
		}
	}
	return res;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	if(ctrl == CTRL_SYNC)
		return RES_OK;
	return RES_PARERR;
}

DWORD get_fattime (void)
{
	return 0; // TODO
}


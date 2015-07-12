/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sdio_sd.h"

/* Definitions of physical drive number for each media */
#define MMC		0
#define ATA		1
#define USB		2


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	switch (pdrv) {
	case MMC :
		return (SD_Init() == SD_OK) ? (RES_OK) : (STA_NOINIT);
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	switch (pdrv) {
	case MMC :
		return RES_OK;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	SD_Error RE = SD_OK;
	unsigned long i;
	
	if(!count) {  /* count must >= 1 */
		return RES_PARERR;
	}

	switch (pdrv) {
	case MMC :
		if(count == 1) {  /* read one block */
			RE = SD_ReadBlock(buff, sector << 9, 512);
			SD_WaitReadOperation();
			while(SD_GetStatus() != SD_TRANSFER_OK);
		}
		else {  /* read multi block */
//			RE = SD_ReadMultiBlocks(buff, sector << 9, 512, count);
			for(i = 0; i < count; i++) {
				RE = SD_ReadBlock(buff + (i << 9), (sector + i) << 9, 512);
				SD_WaitReadOperation();
				while(SD_GetStatus() != SD_TRANSFER_OK);
			}
		}
		return (RE == SD_OK) ? (RES_OK) : (RES_PARERR);
	}
	return RES_PARERR;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	SD_Error RE = SD_OK;
	unsigned long i;	
	
	if(!count) {  /* count must >= 1 */
		return RES_PARERR;
	}

	switch (pdrv) {
	case MMC :
		if(count == 1) {  /* read one block */
			RE = SD_WriteBlock((uint8_t *)buff, sector << 9, 512);
			SD_WaitReadOperation();
			while(SD_GetStatus() != SD_TRANSFER_OK);
		}
		else {  /* read multi block */
 //			RE = SD_WriteMultiBlocks((uint8_t *)buff, sector << 9, 512, count);
			for(i = 0; i < count; i++) {
				RE = SD_WriteBlock((uint8_t *)(buff + (i << 9)), (sector + i) << 9, 512);
				SD_WaitReadOperation();
				while(SD_GetStatus() != SD_TRANSFER_OK);
			}
			
 		}
		
		return (RE == SD_OK) ? (RES_OK) : (RES_PARERR);
	}
	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	SD_Error RE = SD_OK;
	SD_CardInfo CardInfo;

	switch (pdrv) {
	case MMC :
		RE = SD_GetCardInfo(&CardInfo);
		if(RE == SD_OK) {
			switch(cmd) {
				case GET_SECTOR_COUNT :
					*(DWORD *)buff = CardInfo.CardCapacity/CardInfo.CardBlockSize;
					break;
				case GET_BLOCK_SIZE :
					*(WORD *)buff = CardInfo.CardBlockSize;
					break;
				default :
					break;
			}
			return RES_OK;
		}
		else 
			return RES_PARERR;
	}
	return RES_PARERR;
}
#endif

DWORD get_fattime (void) {
	return 0;
}

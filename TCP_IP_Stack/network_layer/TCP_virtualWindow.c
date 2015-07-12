#include "TCP_virtualWindow.h"
#include "myTCPIP.h"
#include "ff.h"
#include "string.h"

extern FIL  file;

/**
 * @brief 打开文件
 */
mIPErr TCP_vndOpen(UINT8 *dstName, UINT32 *size, VND_METHOD method)
{
	UINT8 res;
	
	if(method == METHOD_FATFS) { /* fatfs */
		res = f_open(&file, (char *)dstName, FA_READ);
		*size = file.fsize;
		if(res != FR_OK) return mIP_ERROR;
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}
	
	return mIP_OK;
}

/**
 * @brief 准备写入文件
 */
mIPErr TCP_vndWriteReady(UINT8 *dstName, VND_METHOD method)
{
	UINT8 res;
	
	if(method == METHOD_FATFS) { /* fatfs */
		res = f_open(&file, (char *)dstName, FA_CREATE_NEW | FA_WRITE);
		if(res != FR_OK) return mIP_ERROR;	
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}	
	
	return mIP_OK;
}



/**
 * @brief 写入数据
 */
mIPErr TCP_vndWrite(UINT8 *data, UINT32 dataLen, VND_METHOD method)
{
	UINT br;
	
	if(method == METHOD_FATFS) { /* fatfs */
		f_write(&file, (char *)data, dataLen, &br);
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}
	
	return mIP_OK;
}

/**
 * @brief 读取数据
 */
mIPErr TCP_vndGet(UINT8 *data, UINT32 dataLen, VND_METHOD method)
{
	UINT br;
	
	if(method == METHOD_FATFS) { /* fatfs */
		f_read(&file, data, dataLen, &br);
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}
	
	return mIP_OK;
}

/**
 * @brief 移动指针
 */
mIPErr TCP_vndMvPtr(UINT32 offset, VND_METHOD method)
{
	if(method == METHOD_FATFS) { /* fatfs */
		f_lseek(&file, offset);
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}
	
	return mIP_OK;
}

/**
 * @brief close
 */
mIPErr TCP_vndClose(VND_METHOD method)
{
	if(method == METHOD_FATFS) { /* fatfs */
		f_close(&file);
	}
	else if(method == METHOD_RAM) { /* RAM */
		
	}
	else {
		
	}
	
	return mIP_OK;	
}

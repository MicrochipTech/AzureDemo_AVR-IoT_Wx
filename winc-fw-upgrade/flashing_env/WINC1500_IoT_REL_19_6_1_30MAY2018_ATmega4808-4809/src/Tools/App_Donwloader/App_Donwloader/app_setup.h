#ifndef __CORTUS_APP_SETUP_H__
#define __CORTUS_APP_SETUP_H__

#include "programmer.h"

#define M2M_MAGIC_APP 0xef522f61UL

/**
*	@fn			sint8 spi_flash_download_app(void)
*	@brief		Download Cortus APP
*	@return		Downloading status
*	@author		M.S.M
*	@version	1.0
*/
sint8 spi_flash_download_app(uint8 *file);

#endif	//__CORTUS_APP_SETUP_H__
#ifndef __FIRMWARE_SETUP_H___
#define __FIRMWARE_SETUP_H___

#include "programmer.h"
/**
*	@fn			sint8 get_firmwarefile_version(char * file,tstrM2mRev *pstrm2mrev);
*	@brief		get the input file firmware version and build data anad time
*	@return
*	@author		M.S.M
*	@version	1.0
*/
sint8 get_firmwarefile_version(char * file,tstrM2mRev *pstrm2mrev);
/**
*	@fn			sint8 get_flash_firmware_version(tstrM2mRev *pstrCurentFirmware,tstrM2mRev *pstrOtaFirmware)
*	@brief		get the flash working and ota imgae firmwware version
*	@return
*	@author		M.S.M
*	@version	1.0
*/
sint8 get_flash_firmware_version(tstrM2mRev *pstrCurentFirmware,tstrM2mRev *pstrOtaFirmware);

/**
*	@fn			sint8 programmer_look_up_table(void)
*	@brief		ceate PLL lookup table
*	@return		creation status
*	@author		M.S.M
*	@version	1.0
*/
sint8 update_look_up_table(uint8* vflash);
#endif
/**
*  @file		main.c
*  @brief		This module contains M2M driver test application
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0
*/
#include "./image_downloader/firmware_setup.h"
#include "driver/include/m2m_svnrev.h"
#include "efuse.h"

/**
* CMD MACROS
*/

/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE
*/
#define NO_WAIT_BIT			(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define FW_ONLY_BIT			(0x04)	/*!< Positiion of fw_only bit in the byte. */
#define BREAK_BIT		    (0x08)	/*!< Positiion of break bit in the byte. */
#define SKIP_FW_BIT		    (0x10)	/*!< Positiion of skip fw bit in the byte. */
#define SKIP_PLL_BIT	    (0x20)	/*!< Positiion of skip pll bit in the byte. */
#define ARGS_ERR_BIT		(0x40)	/*!< Positiion of arguments_error bit in the byte. */

/**
* SET VALUES DEPEND ON COMMAND LINE ARGUMENTS
*/
#define SET_BREAK_BIT(x)    (x |= BREAK_BIT)
#define SET_NO_WAIT_BIT(x)  (x |= NO_WAIT_BIT)	/*!< set no_wait bit in the byte to 1. */
#define SET_FW_ONLY_BIT(x)	(x |= FW_ONLY_BIT)	/*!< set fw_only bit in the byte to 1. */
#define SET_SKIP_FW_BIT(x)	(x |= SKIP_FW_BIT)	/*!< set skip fw bit in the byte to 1. */
#define SET_SKIP_PLL_BIT(x)	(x |= SKIP_PLL_BIT)	/*!< set skip pll bit in the byte to 1. */
#define SET_ARGS_ERR_BIT(x)	(x |= ARGS_ERR_BIT)	/*!< set arguments_error bit in the byte to 1. */

/**
* CHECK FOR VALUES FOR EACH CMD IN COMMAND LINE ARGUMENTS
*/
#define NO_WAIT(x)			(x & NO_WAIT_BIT)	/*!< It will return 1 if no_wait argument had been passed to main */
#define FW_ONLY(x)			(x & FW_ONLY_BIT)	/*!< It will return 1 if fw_only argument had been passed to main */
#define ARGS_ERR(x)			(x & ARGS_ERR_BIT)	/*!< It will return 1 if invalid argument had been passed to main */
#define SKIP_FW(x)			(x & SKIP_FW_BIT)	/*!< It will return 1 if skip_fw argument had been passed to main */
#define SKIP_PLL(x)			(x & SKIP_PLL_BIT)	/*!< It will return 1 if skip_pll argument had been passed to main */
#define BREAK(x)			(x & BREAK_BIT)		/*!< It will return 1 if break argument had been passed to main */


/**
*	@fn		print_nmi
*	@brief	print owener info.
*	@return	void
*	@author
*	@date
*	@version	1.0
*/
static void print_nmi(void)
{
	M2M_PRINT("********************************************\n\r");
	M2M_PRINT("*  >Programmer for WINC1500 SPI Flash<     *\n\r");
	M2M_PRINT("*      Owner:  Atmel Corporation           *\n\r");
	M2M_PRINT("********************************************\n\r");
	M2M_PRINT("SVN REV %u SVN BR %s \n\r",SVN_REVISION,SVN_REL_URL);
	M2M_PRINT("Built at %s\t%s\n", __DATE__, __TIME__);

}

/**
*	@fn			checkArguments
*	@brief		Check for argument passed by user to main function
*	@param[IN]	char * argv[]
*					Argument vector from main
*	@param[IN]	uint8 argc
*					Arguments count
*	@note		If new argument will be added, you MUST modify this function
*	@author
*	@version	1.0
*/
static uint8 checkArguments(char * argv[],uint8 argc, uint8* portNum, char ** image_3a0path, char ** vflash_path)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 1;

	*portNum = 0;
	if(1 >= argc) goto ERR;
	for(;loopCntr<argc;loopCntr++)
	{
		if(strstr(argv[loopCntr],"-fw3a0_path"))
		{
			if(argc-loopCntr > 1) {
				*image_3a0path =  argv[loopCntr+1];
				M2M_PRINT("Firmware Path (3A0) %s\n",*image_3a0path);
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-vflash_path"))
		{
			if(argc-loopCntr > 1) {
				*vflash_path =  argv[loopCntr+1];
				M2M_PRINT("Virtual Flash Path %s\n",*vflash_path);
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-no_wait"))
		{
			SET_NO_WAIT_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-B"))
		{
			SET_BREAK_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-port"))
		{
			if(argc-loopCntr > 1)
			{
				*portNum = atoi(argv[++loopCntr]);
				M2M_PRINT("Port number %d\n", *portNum);
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-skip_pll_update"))
		{
			SET_SKIP_PLL_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-skip_fw_update"))
		{
			SET_SKIP_FW_BIT(ret);
			continue;
		}
		if (strstr(argv[loopCntr], "-fw_only"))
		{
			SET_FW_ONLY_BIT(ret);
			continue;
		}
	}
ERR:
	return ret;
}

sint8 burn_image(char *image_path, uint8 fwOnly, uint8* vflash)
{
	sint8	ret = M2M_SUCCESS;
	FILE	*pf = NULL;
	uint8	*pu8ImageData = NULL;
	uint32	u32ImageSz = 0;
	uint32	u32FlashSize = 0;

	if(image_path != NULL)
	{
		pf = fopen(image_path,"rb+");

		M2M_PRINT("\n");
		if(NULL == pf) {
			M2M_PRINT("[ERR]Unable to open Firmware file:\n\t\"%s\"\r\n",image_path);
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		//M2M_PRINT("Firmware image file has been opened:\n\t\"%s\"\r\n",image_path);

		fseek(pf, 0L, SEEK_END);
		u32ImageSz = ftell(pf);
		fseek(pf, 0L, SEEK_SET);

		u32FlashSize = programmer_get_flash_size();
		if(u32ImageSz > u32FlashSize)
		{
			M2M_ERR("[ERR]Image's size is larger than Flash size.\r\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		pu8ImageData = (uint8 *)malloc(u32ImageSz);
		if (!pu8ImageData)
		{
			fclose(pf);
			M2M_ERR("Error OUT OF RAM\r\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}
		fread(pu8ImageData, u32ImageSz, 1, pf);
		fclose(pf);

		uint32 offset = 0;
		sint8 eraseRet;
		if (fwOnly)
		{
			M2M_PRINT(">FW only mode selected.\r\n");

			if (u32ImageSz < M2M_HTTP_MEM_FLASH_OFFSET + M2M_HTTP_MEM_FLASH_SZ + M2M_CACHED_CONNS_FLASH_SZ + M2M_FIRMWARE_FLASH_SZ)
			{
				M2M_ERR("[ERR]Image size is too small\r\n");
				ret = M2M_ERR_FAIL;
				goto _END;
			}

			// The saved connection parameters section sits between the HTTP files and main firmware, so we can't just do a single erase and write

			// Erase HTTP files
			eraseRet = programmer_erase_http_files(vflash);
			if (eraseRet == M2M_SUCCESS)
			{
				// Write new HTTP files
				if (programmer_write_firmware_image(pu8ImageData + M2M_HTTP_MEM_FLASH_OFFSET, M2M_HTTP_MEM_FLASH_OFFSET, M2M_HTTP_MEM_FLASH_SZ, vflash) != M2M_SUCCESS)
				{
					M2M_PRINT(">\r\n[ERR]Error Writing Image\r\n");
					ret = M2M_ERR_FAIL;
				}

				// Setup vars for flashing main firmware
				offset = M2M_FIRMWARE_FLASH_OFFSET;
				u32ImageSz = M2M_FIRMWARE_FLASH_SZ;

				// Erase main firmware
				eraseRet = programmer_erase_first_partition(vflash);
			}
		}
		else
			eraseRet = programmer_erase_firmware_image(vflash);

		if (M2M_SUCCESS != eraseRet) {
			M2M_PRINT("Error while erasing SPI Flash.\r\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		if(M2M_SUCCESS != programmer_write_firmware_image(pu8ImageData + offset, offset, u32ImageSz, vflash)) {
			M2M_PRINT(">\r\n[ERR]Error Writing Image\r\n");
			ret = M2M_ERR_FAIL;
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
		M2M_ERR("not valid file\n");
	}

_END:
	if(pu8ImageData)
		free(pu8ImageData);
	return ret;
}

/*
* Routine added to correct production problem when MAC address valid bit
* is not programmed in efuse when MAC address is programmed.
*/
sint8 correct_efuse_prog(sint8 * pu8Corrected, uint8* vflash)
{
	EFUSEProdStruct efuse_struct;
	uint8 skip_bank_check;
	sint8 ret = M2M_SUCCESS;
	uint8 zero_mac_addr[6] = {0, };

	if(pu8Corrected) *pu8Corrected = FALSE;

	/* efuse map has changed in 3A0... Bank 4
	* and 5 optionally may be not used for
	* EfuseProdStruct. */
	if(REV(GET_CHIPID()) == REV_3A0) {
		skip_bank_check = 1;
	} else {
		skip_bank_check = 0;
	}

	/* Clear efuse_struct.. Cleanliness is a sign of good faith! */
	memset(&efuse_struct, 0, sizeof(efuse_struct));

	ret = read_efuse_struct(&efuse_struct, skip_bank_check);
	if(ret == EFUSE_SUCCESS) {
		if(efuse_struct.bank_used && !efuse_struct.bank_invalid) {
			if(efuse_struct.MAC_addr_used == 0) { /* If MAC address is invalid and... */
				if(m2m_memcmp(
					(uint8*)&efuse_struct.MAC_addr[0],
					(uint8*)&zero_mac_addr[0],
					sizeof(zero_mac_addr)) != 0) { /* If the invalid MAC address
												   is non-zero... */
						/* The something is not right ....
						* need to correct efuse_struct.MAC_addr_used bit.
						*/
						efuse_struct.MAC_addr_used = 1;
						/* and overwrite the same bank index...
						* it has to be the same bank!!.
						*/
						if(overwrite_efuse_struct(&efuse_struct, efuse_struct.bank_idx) < 0) {
							M2M_ERR("\n>>Error: MAC address correciton failed. Timeout!\n");
							ret = M2M_ERR_FAIL;
						} else {
							M2M_PRINT(">>Successfully corrected MAC address valid bit on efuse_struct\n");
							if(pu8Corrected) *pu8Corrected = TRUE;
							ret = M2M_SUCCESS;
							dump_efuse_struct(&efuse_struct);
						}
				} else {
					ret = M2M_SUCCESS; /* Nothing to correct here..
									   MAC address is zero. No need to be valid. */
				}
			} else {
				ret = M2M_SUCCESS; /* Nothing to correct here..
								   MAC address is already valid!*/
			}
		} else {
			M2M_ERR("\n>>Error: Unexpected bank data when read is valid.. exiting.\n");
			ret = M2M_ERR_FAIL; /* Unexpected case if read_efuse_struct returns EFUSE_SUCCESS.
								However, let's handle it for sanity*/
		}
	} else if(ret == EFUSE_ERR_INVALID_BANK_OR_DATA) {
		ret = M2M_SUCCESS; /* Nothing to correct here..
						   No efuse_struct programming is valid on flash*/
	} else /* if(ret == EFUSE_ERR_CANT_LOAD_DATA) */ {
		M2M_ERR("\n>>Error: while reading bank data.\n");
		ret = M2M_ERR_FAIL;
	}

	return ret;

}
void dump_flash(char * filename)
{
//#define DUMP_FLASH
#ifdef DUMP_FLASH
	M2M_PRINT("Dumping flash to %s\n", filename);
	FILE *fp;
	uint8 * pf;
	uint32 sz = programmer_get_flash_size();
	pf = malloc(sz);
	if(pf != NULL)
	{
		programmer_read(pf,0,sz);
		fp = fopen(filename,"wb");
		if(fp != NULL)
		{
			fwrite(pf,1,sz,fp);
			fclose(fp);
		}
		free(pf);
	}

#endif
}
int main(int argc, char* argv[])
{
	sint8 ret = M2M_SUCCESS;
	tstrM2mRev strFirmFileInfo;
	tstrM2mRev strCurrentFirmInfo;
	tstrM2mRev strOtaFirmInfo;
	uint8 commands_val = 0;
	char * image_3a0path = NULL;
	char * vflash_path = NULL;
	uint8* vflash = NULL;
	char * image_path = NULL;
	nm_bsp_init();
	print_nmi();
	commands_val = checkArguments(argv,argc,&ret,&image_3a0path,&vflash_path);
	if(ARGS_ERR(commands_val))
	{
		M2M_ERR("\n>>Invalid arguments.\n");
		goto END;
	}

	M2M_PRINT(">>Initialize programmer.\n");
	ret = programmer_init(&ret,BREAK(commands_val));
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("(ERR)Failed To initialize programmer\n");
		goto END;
	}
	dump_flash("Before_id.bin");

	if(vflash_path != NULL)
    {
        uint32 sz = programmer_get_flash_size();
        vflash = malloc(sz);
        if(vflash)
        {
			FILE *fp;
			memset(vflash,0xFF,sz);
			fp = fopen(vflash_path,"rb");
			if(fp != NULL)
			{
				M2M_PRINT("Reading vflash from %s\n", vflash_path);
				fread(vflash,1,sz,fp);
				fclose(fp);
			}
        }
    }

	if(REV(GET_CHIPID()) == REV_3A0){
		image_path = image_3a0path;
	} else {
		goto ERR;
	}

	M2M_PRINT(">>Loading this FW: %s\n", image_path);

	M2M_PRINT("\n");
	if(NULL == image_path)
	{
		goto ERR;
	}

	ret = get_firmwarefile_version(image_path,&strFirmFileInfo);
	if(ret == M2M_SUCCESS)
	{
		M2M_PRINT("----- NOW Programming Firmware Image Version -----\n");
		M2M_PRINT("Firmware ver   : %u.%u.%u Svnrev %u\n", strFirmFileInfo.u8FirmwareMajor, strFirmFileInfo.u8FirmwareMinor,strFirmFileInfo.u8FirmwarePatch,strFirmFileInfo.u16FirmwareSvnNum);
		M2M_PRINT("Min driver ver : %u.%u.%u\n", strFirmFileInfo.u8DriverMajor,strFirmFileInfo.u8DriverMinor,strFirmFileInfo.u8DriverPatch);
		M2M_PRINT("Firmware Build %s Time %s\n",strFirmFileInfo.BuildDate,strFirmFileInfo.BuildTime);
	}
	else
	{
		goto ERR;
	}

	M2M_PRINT("\n");
	ret = get_flash_firmware_version(&strCurrentFirmInfo,&strOtaFirmInfo);
	if(ret == M2M_SUCCESS)
	{
		if((strCurrentFirmInfo.u8FirmwareMajor != 0)||(strCurrentFirmInfo.u8FirmwareMinor != 0))
		{
			M2M_PRINT("----- Previous Firmware Image Version -----\n");
			M2M_PRINT("Firmware ver   : %u.%u.%u Svnrev %u\n", strCurrentFirmInfo.u8FirmwareMajor, strCurrentFirmInfo.u8FirmwareMinor,strCurrentFirmInfo.u8FirmwarePatch,strCurrentFirmInfo.u16FirmwareSvnNum);
			M2M_PRINT("Min driver ver : %u.%u.%u\n", strCurrentFirmInfo.u8DriverMajor,strCurrentFirmInfo.u8DriverMinor,strCurrentFirmInfo.u8DriverPatch);
			M2M_PRINT("Firmware Build %s Time %s\n",strCurrentFirmInfo.BuildDate,strCurrentFirmInfo.BuildTime);
		}
	}
#ifdef PROFILING
	{
	uint32 u32T1=GetTickCount();
#endif
	ret = M2M_SUCCESS;
	if(!SKIP_FW(commands_val))
	{
		ret = burn_image(image_path, FW_ONLY(commands_val), vflash);
	}
	
	if(M2M_SUCCESS == ret) {
		if(SKIP_PLL(commands_val)){
			M2M_PRINT("\n>>Image downloaded successfully.\r\n");
		}
		else{
			if(M2M_SUCCESS == update_look_up_table(vflash)) {
				M2M_PRINT("\n>>Image downloaded successfully.\r\n");
			} else {
				M2M_PRINT("\n>>Image downloaded but failed to update PLL table.\r\n");
				goto ERR;
			}
		}
	}
	else
	{
		M2M_PRINT("\n>>Failed to downloaded image.\r\n");
		goto ERR;
	}
	if(!SKIP_PLL(commands_val))
	{
		/* Efuse correction */
		{
			sint8 corrected;

			ret = correct_efuse_prog(&corrected, vflash);
			if(M2M_SUCCESS == ret) {
				if(corrected) {
					M2M_PRINT("\nEfuse corrected\n");
				} else {
					M2M_PRINT("\nNo nEfuse correction applied.\n");
				}
			} else {
				M2M_ERR("\n>>An error occurred during efuse correction.. See the above log messages.\n");
			}
		}
	}
	dump_flash("After_id.bin");

	if(vflash)
    {
        M2M_PRINT("Saving vflash to %s\n", vflash_path);
        FILE *fp;
        uint8 * pf;
        uint32 sz = programmer_get_flash_size();
        fp = fopen(vflash_path,"wb");
        if(fp != NULL)
        {
            fwrite(vflash,1,sz,fp);
            fclose(fp);
        }
        free(pf);
    }

ERR:
	programmer_deinit();
#ifdef PROFILING
	M2M_PRINT("\n>>This task finished after %5.2f sec\n\r",(GetTickCount() - u32T1)/1000.0);
	}
#endif
END:
	if(!NO_WAIT(commands_val))
		system("pause");
	return ret;
}

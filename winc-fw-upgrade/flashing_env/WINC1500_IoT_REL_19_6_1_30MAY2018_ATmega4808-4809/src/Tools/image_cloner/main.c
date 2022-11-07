/**
*  @file		main.c
*  @brief		This module contains M2M driver test application
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0
*/
#include "./image_cloner/firmware_setup.h"
#include "driver/include/m2m_svnrev.h"
#include "efuse.h"
/**
* CMD MACROS
*/

/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE
*/
#define NO_WAIT_BIT			(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define BREAK_BIT		    (0x08)	/*!< Positiion of break bit in the byte. */
#define ARGS_ERR_BIT		(0x40)	/*!< Positiion of arguments_error bit in the byte. */

/**
* SET VALUES DEPEND ON COMMAND LINE ARGUMENTS
*/
#define SET_BREAK_BIT(x)    (x |= BREAK_BIT)
#define SET_NO_WAITE_BIT(x) (x |= NO_WAIT_BIT)	/*!< set no_wait bit in the byte to 1. */
#define SET_ARGS_ERR_BIT(x)	(x |= ARGS_ERR_BIT)	/*!< set arguments_error bit in the byte to 1. */
/**
* CHECK FOR VALUES FOR EACH CMD IN COMMAND LINE ARGUMENTS
*/
#define NO_WAIT(x)			(x & NO_WAIT_BIT)	/*!< It will return 1 if no_wait argument had been passed to main */
#define ARGS_ERR(x)			(x & ARGS_ERR_BIT)	/*!< It will return 1 if invalid argument had been passed to main */
#define BREAK(x)			(x & BREAK_BIT)


// globals
uint8	*pu8ImageData = NULL;
uint32	u32ImageSz = 0;


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
static uint8 checkArguments(char * argv[],uint8 argc, uint8* portNum, char ** in_path, char ** out_path, uint32* check_range)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 1;

	*portNum = 0;
	if(1 >= argc) goto ERR;
	for(;loopCntr<argc;loopCntr++)
	{
		if(strstr(argv[loopCntr],"-port"))
		{
			if(argc-loopCntr > 1)
			{
				*portNum = atoi(argv[++loopCntr]);
				M2M_PRINT("Port number %d\n", *portNum);
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-span"))
		{
			if(argc-loopCntr > 1)
			{
				*check_range = atol(argv[++loopCntr]);
			}
			continue;
		}
		if (strstr(argv[loopCntr], "-in_path"))
		{
			if (argc - loopCntr > 1) {
				*in_path = argv[loopCntr + 1];
				M2M_PRINT("Image to write to flash file path %s\n", *in_path);
			}
			continue;
		}
		if (strstr(argv[loopCntr], "-out_path"))
		{
			if (argc - loopCntr > 1) {
				*out_path = argv[loopCntr + 1];
				M2M_PRINT("File to save copy of flash path %s\n", *out_path);
			}
			continue;
		}
	}
ERR:
	return ret;
}

sint8 burn_image(char *image_path)
{
	sint8	ret = M2M_SUCCESS;
	FILE	*pf = NULL;
	uint32	u32FlashSize = 0;

	if(image_path != NULL)
	{
		pf = fopen(image_path,"rb+");

		M2M_PRINT("\n");
		if(NULL == pf) {
			M2M_PRINT("[ERR]Unable to open image file:\n\t\"%s\"\r\n",image_path);
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		M2M_PRINT("Firmware image file has been opened:\n\t\"%s\"\r\n",image_path);

		fseek(pf, 0L, SEEK_END);
		u32ImageSz = ftell(pf);
		fseek(pf, 0L, SEEK_SET);

		u32FlashSize = programmer_get_flash_size();
		if(u32ImageSz != u32FlashSize) {
			M2M_PRINT("[ERR]Image's size does not match Flash size.\r\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		pu8ImageData = (uint8 *)malloc(u32ImageSz);
		fread(pu8ImageData, u32ImageSz, 1, pf);
		fclose(pf);

		if(M2M_SUCCESS != programmer_erase_firmware_image(NULL)) {
			M2M_PRINT("Error while erasing flash.\r\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}

		if(M2M_SUCCESS != programmer_write_firmware_image(pu8ImageData, 0, u32ImageSz, NULL)) {
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


sint8 read_verify_flash(char * filename, uint8	*vflash, uint32	szvflash, uint32 check_range)
{
	sint8	ret = M2M_ERR_FAIL;
	FILE *fp;
	uint8 * pf;
	uint32 sz = programmer_get_flash_size();
	pf = malloc(sz);
	if(pf != NULL)
	{
        M2M_PRINT("Retrieving flash image... %s\r\n", filename);
        memset(pf,0xFF,sz);
		if (M2M_SUCCESS == programmer_read(pf, 0, check_range)) // we ignore top part as it is not programmed - user sets -span
		{
			fp = fopen(filename, "wb");
			if (fp != NULL)
			{
				if (sz == fwrite(pf, 1, sz, fp))
				{
                    M2M_PRINT("Wrote flash image to %s\r\n", filename);
					ret = M2M_SUCCESS;
				}
				fclose(fp);
			}

			if(vflash && szvflash) // if we burned it let us verify too
            {
                if(sz == szvflash)
                {
                    if(memcmp(pf, vflash, szvflash))
                    {
                        M2M_ERR("Readback verify contents differ\n");
                        ret = M2M_ERR_FIRMWARE_bURN;
                    }
                    else
                    {
                   		M2M_PRINT("Readback verify PASS\n");
                    }
                }
                else
                {
                    M2M_ERR("Readback verify sizes differ\n");
                    ret = M2M_ERR_FIRMWARE_bURN;
                }
            }
		}
		free(pf);
	}

	return ret;
}

int main(int argc, char* argv[])
{
	sint8 ret = M2M_SUCCESS;
	uint8 commands_val = 0;
	char * in_path = NULL;
	char * out_path = NULL;
	uint32 check_range = FLASH_4M_TOTAL_SZ;

	nm_bsp_init();
	print_nmi();
	commands_val = checkArguments(argv,argc,&ret,&in_path,&out_path,&check_range);
	if(ARGS_ERR(commands_val))
	{
		M2M_ERR("\n>>Invalid arguments.\n");
		goto END;
	}

	M2M_PRINT(">>Initialize programmer.\n");
	ret = programmer_init(&ret,BREAK(commands_val));
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("(ERR)Failed To intilize programmer\n");
		goto END;
	}

	if(in_path)
	{
		ret = burn_image(in_path);
        if(M2M_SUCCESS != ret)
        {
            M2M_PRINT("\n>>Failed to write flash.\r\n");
        }
	}

	if(out_path)
	{
		ret = read_verify_flash(out_path, pu8ImageData, u32ImageSz, check_range);
        if(M2M_SUCCESS != ret)
        {
            M2M_PRINT("\n>>Failed to read/verify flash.\r\n");
        }
	}

ERR:
	programmer_deinit();

END:
	return ret;
}


#include "./image_builder/builder.h"

/**
* CMD MACROS
*/
/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE
*/
#define NO_WAIT_BIT				(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define OTA_IMAGE_BIT			(0x04)	/*!< Positiion of type of image bit in the byte. */
#define BURST_IMAGE_BIT			(0x08)	/*!< Positiion of burst image bit in the byte. */
#define HTTP_MODIFY_BIT 		(0x10)	/*!< Positiion of http modify bit in the byte. */
#define AOCHIP_BIT 				(0x20)	/*!< Positiion of http modify bit in the byte. */
#define NMC1003A0_BIT			(0x40)

/**
* SET VALUES DEPEND ON COMMAND LINE ARGUMENTS
*/
#define SET_NO_WAITE_BIT(x)		(x |= NO_WAIT_BIT)		/*!< set no_wait bit in the byte to 1. */
#define SET_OTA_IMAGE_BIT(x)	(x |= OTA_IMAGE_BIT)	/*!< set image type bit in the byte to 1. */
#define SET_BURST_IMAGE_BIT(x)	(x |= BURST_IMAGE_BIT)	/*!< set image type bit in the byte to 1. */
#define SET_HTTP_MODIFY_BIT(x)	(x |= HTTP_MODIFY_BIT)	/*!< set http modify bit in the byte to 1. */
#define SET_AOCHIP_BIT(x)		(x |= AOCHIP_BIT)	/*!< set http modify bit in the byte to 1. */
/**
* CHECK FOR VALUES FOR EACH CMD IN COMMAND LINE ARGUMENTS
*/
#define NO_WAIT(x)				(x & NO_WAIT_BIT)		/*!< return 1 if no_wait argument has been passed to main */
#define OTA_IMAGE(x)			(x & OTA_IMAGE_BIT)		/*!< return 1 if image type is OTA*/
#define BURST_IMAGE(x)			(x & BURST_IMAGE_BIT)	/*!< return 1 ifimage type is ATE */
#define HTTP_MODIFY(x)			(x & HTTP_MODIFY_BIT)	/*!< return 1 if http modify argument has been passed to main */
#define AOCHIP(x)				(x & AOCHIP_BIT)		/*!< set http modify bit in the byte to 1. */
#define IS_NMC1003A0(x)			(x & NMC1003A0_BIT)

#define NUMBER_OF_BINARIES				(sizeof(binaries) / sizeof(binaries[0]))

#define ENABLE_WRITE_BOOT_FW			TRUE
#define ENABLE_WRITE_CTRL_SECT			TRUE
#define ENABLE_WRITE_MAIN_FW			TRUE
#define ENABLE_WRITE_PLL_SECTOR			TRUE
#define ENABLE_WRITE_CERTS				TRUE
#define ENABLE_WRITE_HTTP_FILES			TRUE
#define ENABLE_WRITE_PS_FW_SECTION		FALSE
#define ENABLE_WRITE_CONN_PARAMS		TRUE
#define ENABLE_WRITE_TLS_SERVER_SECTION	TRUE
#define ENABLE_WRITE_BURST_FW_SECTION	TRUE

char binaries[][64] = {
"[BOOT FIRMWARE]",
"[CONTROL SECTION]",
"[FIRMWARE]",
"[LUT SECTOR]",
"[CERTIFICATES]",
"[HTTP FILES]",
"[CONN. PARAMETERS]",
"[TLS SERVER]",
"[Burst FW]"
};
static double paramXOOffset = 0;

static void print_nmi(void)
{
	M2M_PRINT("********************************************\n\r");
	M2M_PRINT("*  >All-In-One SPI Flash Image Builder<    *\n\r");
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
static uint8 checkArguments(char * argv[], uint8 argc, uint8* portNum, char** image_path, char** ate_path, char** output_path, char** otafilename, char** ppcHttpSrcFolder,
							char** custom_image_path,char** gain_table_image_path,uint32* chipId, char** vflash_path, char** metrics_path)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 1;
	if(1 >= argc) goto ERR;
	for(;loopCntr<argc;loopCntr++)
	{
		if(strstr(argv[loopCntr],"-metrics"))
		{
			if(argc-loopCntr > 1) {
				*metrics_path = argv[loopCntr+1];
			}
			continue;
		}

		if(strstr(argv[loopCntr],"-fw_path"))
		{
			if(argc-loopCntr > 1) {
				*image_path = argv[loopCntr+1];
				//memcpy(image_path, argv[loopCntr+1], strlen(argv[loopCntr+1]));
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-vflash_path"))
		{
			if(argc-loopCntr > 1) {
				*vflash_path =  argv[loopCntr+1];
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-op_path"))
		{
			if(argc-loopCntr > 1) {
				//memset(output_path,0,sizeof(output_path));
				*output_path = argv[loopCntr+1];
				//memcpy(output_path, argv[loopCntr+1], strlen(argv[loopCntr+1]));
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-no_wait"))
		{
			SET_NO_WAITE_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-ota_img"))
		{
			SET_OTA_IMAGE_BIT(ret);
			if(argc-loopCntr > 1) {
				//memset(output_path,0,sizeof(output_path));
				*otafilename = argv[loopCntr+1];
				//memcpy(output_path, argv[loopCntr+1], strlen(argv[loopCntr+1]));
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-ate_img"))
		{
			SET_BURST_IMAGE_BIT(ret);
			if(argc-loopCntr > 1) {
				*ate_path = argv[loopCntr+1];
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-http_modify"))
		{
			SET_HTTP_MODIFY_BIT(ret);
			if(argc-loopCntr > 1)
			{
				*ppcHttpSrcFolder = argv[loopCntr+1];
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-1003A0"))
		{
#ifdef 	DIS_DOWNLODER_ON1003A0
			SET_AOCHIP_BIT(ret);
#endif
			ret |= NMC1003A0_BIT;
			*chipId = 0x1503a0;
			continue;
		}
		if(strstr(argv[loopCntr],"-xo"))
		{
			paramXOOffset = atof(argv[loopCntr+1]);
			continue;
		}
		if(strstr(argv[loopCntr],"-custom_image_path"))
		{
			*custom_image_path = argv[loopCntr+1];
			continue;
		}
		if(strstr(argv[loopCntr],"-gain_image_path"))
		{
			*gain_table_image_path = argv[loopCntr+1];
			continue;
		}
		if(strstr(argv[loopCntr],"-rev"))
		{
			if(*argv[loopCntr+1] == 'A')
			{
				*chipId = 0x1502b1;
				continue;
			}
			if(*argv[loopCntr+1] == 'B')
			{
				*chipId = 0x1503a0;
				continue;
			}
		}
	}
ERR:
	return ret;
}

int main(int argc, char* argv[])
{
	sint8 ret;
	uint16 buildStatus = 0;
	uint8 loopCntr = 0;
	tstrM2mRev strFirmFileInfo;
	uint8 commands_val = 0;

	char* output_path = NULL;
	char* image_path = NULL;
	char* metrics_path = NULL;
	char* custom_image_path = NULL;
	char* gain_table_image_path = NULL;
	char* ate_path = NULL;
	char* otafilename = NULL;
	char*  pcHttpSrcFolder = NULL;
	char * vflash_path = NULL;
	uint8* vflash = NULL;       // for image builder this will stay NULL.
	uint32 chipId = 0;
	
	pManifest = malloc(sizeof(tstrManifest));
	if(!pManifest)
    {
		M2M_PRINT("(ERROR) manifest Malloc failed \n");
		goto _END_3;
    }
    memset(pManifest, 0, sizeof(tstrManifest));
	
	print_nmi();

	commands_val = checkArguments(argv,argc,&ret,&image_path,&ate_path,&output_path,&otafilename,&pcHttpSrcFolder,&custom_image_path,&gain_table_image_path,
								  &chipId,&vflash_path, &metrics_path);
	M2M_PRINT(">>Init All-In-One Image Builder\n");

	if(custom_image_path != NULL && gain_table_image_path != NULL)
	{
		M2M_PRINT(">>Generating Custom Image\n");
		ret = init_custom_image_builder(custom_image_path);
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Unable to create custom image\n");
		}
	}

#ifdef PROFILING
	{
	uint32 u32T1=GetTickCount();
#endif
	if (HTTP_MODIFY(commands_val))
	{
		if(M2M_SUCCESS == builder_init(image_path,"rb+"))
		{
			M2M_PRINT("\n Firmware image opened to modify HTTP files\n");
			ret = build_http_config(&strFirmFileInfo,pcHttpSrcFolder,/*OTA_IMAGE(commands_val)*/ 0);
			if(ret == M2M_SUCCESS)
			{
				M2M_PRINT("\n HTTP files has been successfully modified \n");
			}
			else
			{
				M2M_PRINT("(ERROR) HTTP files modification Failed! \n");
			}
		}
		else
		{
			M2M_PRINT("\n Unable to open Firmware image \n");
		}
#if 0
		if(OTA_IMAGE(commands_val))
		{
			PrepOtaModifiedHttp(image_path);
		}
#endif
		builder_deinit();
		goto _END_2;
	}

	if(M2M_SUCCESS == builder_init(output_path,"wb")) {
		M2M_PRINT("Initialization has been done.\n");
	} else {
		M2M_PRINT("Unable to initialize file.\n");
		goto _END_2;
	}

	M2M_PRINT("\t\"%s\"\r\n",image_path);

	M2M_PRINT("\nOutput File:\r\n");
	M2M_PRINT("\t\"%s\"\r\n\n",output_path);

	ret = get_firmwarefile_version(image_path,&strFirmFileInfo);
	strFirmFileInfo.u32Chipid = chipId;
	if(ret == M2M_SUCCESS)
	{
		M2M_PRINT("----- Input Firmware File Version-----\n");
		M2M_PRINT("Firmware ver   : %u.%u.%u Svnrev %u\n", strFirmFileInfo.u8FirmwareMajor, strFirmFileInfo.u8FirmwareMinor,strFirmFileInfo.u8FirmwarePatch,strFirmFileInfo.u16FirmwareSvnNum);
		M2M_PRINT("Min driver ver : %u.%u.%u\n", strFirmFileInfo.u8DriverMajor,strFirmFileInfo.u8DriverMinor,strFirmFileInfo.u8DriverPatch);
		M2M_PRINT("Firmware Build %s Time %s\n",strFirmFileInfo.BuildDate,strFirmFileInfo.BuildTime);
	}
	else
	{
		goto _END_1;
	}


#if(ENABLE_WRITE_BOOT_FW == TRUE)
	ret = build_boot_firmware(&strFirmFileInfo);
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_boot_firmware\"\n");
		buildStatus |= 0x0001;
		goto _END_1;
	}
#endif

#if(ENABLE_WRITE_CTRL_SECT == TRUE)
	ret = build_control_sec(&strFirmFileInfo,AOCHIP(commands_val));
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_control_sec\"\n");
		buildStatus |= 0x0002;
		goto _END_1;
	}
#endif

#if(ENABLE_WRITE_PLL_SECTOR == TRUE)
	ret = build_look_up_table(&strFirmFileInfo,paramXOOffset);
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_look_up_table\"\n");
		buildStatus |= 0x0008;
		goto _END_1;
	}
#endif

#if(ENABLE_WRITE_MAIN_FW == TRUE)
	ret = build_firmware(&strFirmFileInfo,image_path,otafilename,AOCHIP(commands_val),OTA_IMAGE(commands_val));
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_firmware\"\n");
		buildStatus |= 0x0004;
		goto _END_1;
	}
#endif

#if(ENABLE_WRITE_CERTS == TRUE)
	ret = build_certificates(&strFirmFileInfo, vflash);
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_certificates\"\n");
		buildStatus |= 0x0010;
		//goto _END_1;
	}
#endif

#if(ENABLE_WRITE_HTTP_FILES	== TRUE)
	ret = build_http_config(&strFirmFileInfo,HTTP_FILE_PATH,FALSE);
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_http_config\"\n");
		buildStatus |= 0x0020;
		//goto _END_1;
	}
#endif

#if(ENABLE_WRITE_PS_FW_SECTION == TRUE)
	ret = build_ps_firmware();
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_http_config\"\n");
		buildStatus |= 0x0040;
		//goto _END_1;
	}
#endif

#if(ENABLE_WRITE_CONN_PARAMS == TRUE)
	ret = build_conn_params();
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_http_config\"\n");
		buildStatus |= 0x0040;
		//goto _END_1;
	}
#endif

#if(ENABLE_WRITE_TLS_SERVER_SECTION == TRUE)
	ret = build_tls_server_flash();
	if(ret != M2M_SUCCESS) {
		M2M_PRINT("(ERR)Error in \"build_tls_server_flash\"\n");
		buildStatus |= 0x0080;
		//goto _END_1;
	}
#endif

#if(ENABLE_WRITE_BURST_FW_SECTION == TRUE)
	if(BURST_IMAGE(commands_val))
	{
		ret = build_burst_fw(&strFirmFileInfo,ate_path);
		if(ret != M2M_SUCCESS) {
			M2M_PRINT("(ERR)Error in \"burst firmware\"\n");
			buildStatus |= 0x0100;
			//goto _END_1;
		}
	}
	else
	{
		buildStatus |= 0x0100;
	}
#endif


	M2M_PRINT("\n");
	for(loopCntr=0; loopCntr<NUMBER_OF_BINARIES; loopCntr++) {
		if(!BURST_IMAGE(commands_val) && (9 == loopCntr)) {
			break;
		}
		if((0x01<<(loopCntr)) & buildStatus) {
			M2M_PRINT("[%02d]%-20s ... FAIL *\n", loopCntr+1, binaries[loopCntr]);
			ret = M2M_ERR_FAIL;
		} else {
			M2M_PRINT("[%02d]%-20s ... PASS\n", loopCntr+1, binaries[loopCntr]);
		}
	}
_END_1:
	if(custom_image_path != NULL && gain_table_image_path != NULL)
	{
		make_custom_image(gain_table_image_path);
	}
	if(M2M_SUCCESS == builder_deinit()) {
		/*continue*/
	} else {
		M2M_PRINT("Unable to create the file.\n");
		goto _END_2;
	}

#if 0
	if(OTA_IMAGE(commands_val))
	{
		create_ota_bin(output_path,otafilename);
	}
#endif

_END_2:
    {
        char* pManifesftFN = malloc(530); // manifest name, and borrowed for metric manipulation
        if(pManifesftFN)
        {
            FILE* pOpManifest;

            strcpy(pManifesftFN, output_path);
            strcat(pManifesftFN, ".manifest");
            M2M_PRINT("Creating manifest file: %s\n",pManifesftFN);
            pOpManifest = safeopen((const char *)pManifesftFN,"w");
            if(pOpManifest)
            {
                fprintf(pOpManifest, "bootfw_ofs=%u\n",   pManifest->BootFW_ofs);
                fprintf(pOpManifest, "bootfw_sz=%u\n",    pManifest->BootFW_used);
                fprintf(pOpManifest, "bootfw_max=%u\n",   pManifest->BootFW_space);
                fprintf(pOpManifest, "bootfw_free=%d\n",  pManifest->BootFW_space - pManifest->BootFW_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "control_ofs=%u\n",  pManifest->Control_ofs);
                fprintf(pOpManifest, "control_sz=%u\n",   pManifest->Control_used);
                fprintf(pOpManifest, "control_max=%u\n",  pManifest->Control_space);
                fprintf(pOpManifest, "control_free=%d\n", pManifest->Control_space - pManifest->Control_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "pll_ofs=%u\n",      pManifest->PLL_ofs);
                fprintf(pOpManifest, "pll_sz=%u\n",       pManifest->PLL_used);
                fprintf(pOpManifest, "pll_max=%u\n",      pManifest->PLL_space);
                fprintf(pOpManifest, "pll_free=%d\n",     pManifest->PLL_space - pManifest->PLL_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "gain_ofs=%u\n",     pManifest->Gain_ofs);
                fprintf(pOpManifest, "gain_sz=%u\n",      pManifest->Gain_used);
                fprintf(pOpManifest, "gain_max=%u\n",     pManifest->Gain_space);
                fprintf(pOpManifest, "gain_free=%d\n",    pManifest->Gain_space - pManifest->Gain_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "wifidata_ofs=%u\n", pManifest->WIFIData_ofs);
                fprintf(pOpManifest, "wifidata_sz=%u\n",  pManifest->WIFIData_used);
                fprintf(pOpManifest, "wifidata_max=%u\n", pManifest->WIFIData_space);
                fprintf(pOpManifest, "wifidata_free=%d\n",pManifest->WIFIData_space - pManifest->WIFIData_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "fw_ofs=%u\n",       pManifest->FW_ofs);
                fprintf(pOpManifest, "fw_sz=%u\n",        pManifest->FW_used);
                fprintf(pOpManifest, "fw_max=%u\n",       pManifest->FW_space);
                fprintf(pOpManifest, "fw_free=%d\n",      pManifest->FW_space - pManifest->FW_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "http_ofs=%u\n",     pManifest->HTTP_ofs);
                fprintf(pOpManifest, "http_sz=%u\n",      pManifest->HTTP_used);
                fprintf(pOpManifest, "http_max=%u\n",     pManifest->HTTP_space);
                fprintf(pOpManifest, "http_free=%d\n",    pManifest->HTTP_space - pManifest->HTTP_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "bt_ofs=%u\n",       pManifest->BT_ofs);
                fprintf(pOpManifest, "bt_sz=%u\n",        pManifest->BT_used);
                fprintf(pOpManifest, "bt_max=%u\n",       pManifest->BT_space);
                fprintf(pOpManifest, "bt_free=%d\n",      pManifest->BT_space - pManifest->BT_used);
                fprintf(pOpManifest, "\n");
                fprintf(pOpManifest, "total_sz=%u\n",     pManifest->Total_size);

                fclose(pOpManifest);
            }
 
            {
                printf("Flash Utilization\n");
                printf("bootfw_free=%d\n",  pManifest->BootFW_space - pManifest->BootFW_used);
                printf("gain_free=%d\n",    pManifest->Gain_space - pManifest->Gain_used);
                printf("fw_free=%d\n",      pManifest->FW_space - pManifest->FW_used);
                printf("http_free=%d\n",    pManifest->HTTP_space - pManifest->HTTP_used);
                printf("bt_free=%d\n",      pManifest->BT_space - pManifest->BT_used);
                printf("total_sz=%u\n",     pManifest->Total_size);
            }

            if(metrics_path)
            {
                FILE* pfmanifestin;
                FILE* pfmanifestout;

                strcpy(pManifesftFN, metrics_path);
                strcat(pManifesftFN, ".bak");

                M2M_PRINT("Saving metrics file: %s\n", metrics_path);
                remove(pManifesftFN);
                if(rename((const char *)metrics_path, (const char *)pManifesftFN))
                {
                    M2M_PRINT("Backup metrics file failed\n");
                }
                else
                {
                    int undo = 0;
                    pfmanifestin = safeopen((const char *)pManifesftFN,"r");
                    if(pfmanifestin)
                    {
                        pfmanifestout = safeopen((const char *)metrics_path,"w");
                        if(pfmanifestout)
                        {
                            M2M_PRINT("Updating metrics file: %s\n", metrics_path);
                            char* meddling = malloc(1024);
                            if(meddling)
                            {
                                // DataMemFree,ProgMemFree,SharedMemFree,Revision,CodeSpaceFree
                                fscanf(pfmanifestin,"%s",meddling);

                                char* pos1 = strstr(meddling, "DataMemFree");
                                char* pos2 = strstr(meddling, "FWFlashFree");
                                if(pos1 && !pos2) // right format, not already done
                                {
                                    fprintf(pfmanifestout,"%s,%s,%s,%s,%s\n",
                                            meddling,
                                            "FWFlashFree",
                                            "BTFlashFree",
                                            "GainFlashFree",
                                            "HTTPFlashFree");

                                    //1760,1130,272,15904     ,3162
                                    fscanf(pfmanifestin,"%s",meddling);
                                    fprintf(pfmanifestout,"%s,%u,%u,%u,%u\n",
                                            meddling,
                                            pManifest->FW_space - pManifest->FW_used,
                                            pManifest->BT_space - pManifest->BT_used,
                                            pManifest->Gain_space - pManifest->Gain_used,
                                            pManifest->HTTP_space - pManifest->HTTP_used);
                                }
                                else
                                {
                                    if(pos2)
                                        M2M_PRINT("File already processed\n");
                                    if(!pos1)
                                        M2M_PRINT("File format error\n");
                                    undo = 1;
                                }
                                free(meddling);
                            }
                            fclose(pfmanifestout);
                        }
                        else
                        {
                            M2M_PRINT("Failed to open: %s\n",metrics_path);
                        }
                        fclose(pfmanifestin);
                    }

                    if(undo)
                    {
                        remove(metrics_path);
                        rename((const char *)pManifesftFN, (const char *)metrics_path);
                    }
                }
            }

            free(pManifesftFN);
            pManifesftFN = NULL;
        }
        free(pManifest);
        pManifest = NULL;
    }
#ifdef PROFILING
	M2M_PRINT("\n>>This task finished after %5.2f sec\n\r",(GetTickCount() - u32T1)/1000.0);
	}
#endif

_END_3:

	M2M_PRINT("\n\n");
	if(!NO_WAIT(commands_val))
		system("pause");

	return ret;
}

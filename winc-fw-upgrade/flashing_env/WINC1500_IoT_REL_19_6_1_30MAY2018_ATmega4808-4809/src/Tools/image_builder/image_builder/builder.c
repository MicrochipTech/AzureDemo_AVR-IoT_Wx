#include "builder.h"
#include "common_values.h"
#include "..\..\..\common\fw_info\fw_info.h"
#include "..\..\..\common\root_cert\root_setup.h"
#include "ota_hdr.h"
#include "crypto_lib_api.h"
#include "tls_srv_sec.h"

extern int PrepareOta(uint8 *pu8OTAFw, uint32 u32OtaFwSize, char * pcOTAFile);
extern int PrepOtaModifiedHttp(char * file);
extern void ListDirectoryContents(const char *pcDir, char *pcExt, char ***ppacFileList, uint32 *pu32ListSize);
extern sint8 TlsCertStoreWriteCertChain(char *pcPrivKeyFile, char *pcSrvCertFile, char *pcCADirPath, uint8 *pu8TlsSrvSecBuff, uint32 *pu32SecSz, tenuWriteMode enuMode);
extern int ReadFileToBuffer(char *pcFileName, uint8 **ppu8FileData, uint32 *pu32FileSize);

static FILE			*gpFile = NULL;
static FILE			*ciFile = NULL;
static uint8		custom_image_build_enabled = 0;
static uint32		gu32ImageOffset = 0;
static uint32		gu32ImageSize = 0;
static uint32		gu32CustomImageOffset = 0;
static uint32		gu32CustomImageSize = 0;

#define SKIP_BINARY_HEADER              0
/*Macro-to be removed later*/

/*Globals*/
static uint8		gau8Firmware[NM_BSP_PERM_FIRMWARE_SIZE];
static uint8		gainTableFirmwareBuffer[M2M_CONFIG_SECT_TOTAL_SZ];
static uint8		gau8TlsSrvSec[M2M_TLS_SERVER_FLASH_SIZE];

/*Firmware File Revison info*/
tstrOtaControlSec	strOtaControlSec  = {0};
tstrM2mRev			strfirmwareFileRev = {0};

/*
@struct	\
tstrFlashFileHeader

@brief
Header information for a flash file entry
*/
typedef struct{
	char	acFileName[32];
	/*!<
	NULL terminated string holding the file name. For example, default.html
	*/
	uint32	u32FileSize;
	/*!<
	File size in bytes.
	*/
}tstrFlashFileHeader;


FILE* safeopen(const char* filename, const char * mode)
{
#ifdef WIN32
	FILE *fp = NULL;
    if( fopen_s( &fp, filename, mode) !=0 )
		fp = NULL;	// failed
	return fp;
#else
    return fopen(filename, mode);
#endif
}

/*****************************************
	STATIC FUNCTIONS
*****************************************/
static sint8 spitBinary(tstrM2mBinaryHeader* binaryHeader,char* binaryFileName,uint8 *pu8Buf, uint32 u32Sz)
{
	sint8 ret = M2M_ERR_FAIL;
	FILE *fpTemp;
	char tempName[256] = OTA_BINARIES_DIRECTORY;
	if(binaryHeader->binVerInfo.u32Chipid == 0x1503a0){
		strcat(tempName,"/ASIC_3A0/");
		strcat(tempName,binaryFileName);
		strcat(tempName,"_3a0.bin");
	}else{
		return ret;
	}
	fpTemp = fopen(tempName, "wb");
	if(fpTemp){
		fseek(fpTemp, 0L, SEEK_SET);
#if !SKIP_BINARY_HEADER
		fwrite(&binaryHeader->binVerInfo,sizeof(tstrM2mRev),1,fpTemp);
		fwrite(&binaryHeader->flashOffset,sizeof(uint32),1,fpTemp);
		fwrite(&binaryHeader->payloadSize,sizeof(uint32),1,fpTemp);
#endif /*SKIP_BINARY_HEADER*/
		fwrite(pu8Buf,u32Sz, 1, fpTemp);
		fclose(fpTemp);
		ret = M2M_SUCCESS;
	}
	return ret;
}
/**
*	@fn			sint8 create_program_bin(u32offset,* file,* u32Retsz)
*	@brief		create program bin
*	@param[IN]	u32Offset
*					Address to write to at the SPI flash
*	@param[IN]	* file
*					pointer to data file
*	@param[IN]	u32Retsz
*					Size of data
*	@return		creation status
*	@author		M.S.M
*	@version	1.0
*/
static sint8 create_firmware_bin(uint32 offset,char * file,uint32 * u32Retsz,char * str)
{
	FILE *fp;
	int FirmSz = 0,FileSz = 0;
	sint8 ret = M2M_SUCCESS;
	if(file != NULL)
	{
		fp = fopen(file, "rb");

		if(fp)
		{

			fseek(fp, 0L, SEEK_END);
			FileSz = ftell(fp);
			fseek(fp, 0L, SEEK_SET);

			/* Copy "NMIS" string at the start of the SPI flash */
			m2m_memcpy(gau8Firmware + offset, (uint8*)str, 4);
			fread(&FirmSz,4,1,fp);
			FileSz -= 4;
			/*read first 4 byte for the firmware size*/
			fread(&gau8Firmware[8 + offset], FileSz, 1, fp);
			FirmSz += 4;/*NMIS size */
			FileSz += 8;/*NMIS  + size*/
			fclose(fp);


			/* Copy file size at offset 4 */
			m2m_memcpy(&gau8Firmware[4 + offset], (uint8*)&FirmSz, 4);
		}
		else
		{
			ret = M2M_ERR_FAIL;
			NM_BSP_PRINTF("failed to open firmware file \n%s\n",file);
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
		NM_BSP_PRINTF("failed to open firmware file \n%s\n",file);
	}
	*u32Retsz = FileSz;
	return ret;
}


sint8 write_to_file(uint8 *pu8Buf, uint32 u32Offset, uint32 u32Sz)
{
	sint8 ret = M2M_SUCCESS;

	if(custom_image_build_enabled)
	{
		if(ciFile) {
			fseek(ciFile, u32Offset, SEEK_SET);
			fwrite(pu8Buf,u32Sz,1,ciFile);
			gu32CustomImageSize+= u32Sz;
		} else {
			M2M_ERR("can't write to custom image file\n");
			ret = M2M_ERR_FAIL;
		}
	}
	if(gpFile) {
		fseek(gpFile, u32Offset, SEEK_SET);
		fwrite(pu8Buf,u32Sz,1,gpFile);
		gu32ImageSize+= u32Sz;
	} else {
		M2M_ERR("can't write to image file\n");
		ret = M2M_ERR_FAIL;
	}
	return ret;
}

static uint16 http_load_file(char *pcSrcFolder, char *pcFileName, uint8 **ppu8OutBuffer)
{
	uint16	u16FileSize = M2M_ERR_FAIL;
	uint8	*pu8Buffer = *ppu8OutBuffer;
	char	acFileName[256] ;
	if (strlen(pcSrcFolder) < sizeof(acFileName))
	{
		strcpy(acFileName,pcSrcFolder);
	}
	else
	{
		M2M_ERR("HTTP Source Folder Path is too long\n");
		goto END;
	}

	if(pcFileName != NULL)
	{
		FILE	*fp;
		if((strlen(pcSrcFolder) + strlen(pcFileName) ) < sizeof(acFileName))
		{
			strncat(acFileName, pcFileName,strlen(pcFileName));
		}
		else
		{
			M2M_ERR("HTTP File Path is too long\n");
			goto END;
		}
		fp = fopen(acFileName, "rb");
		if(fp) {
			tstrFlashFileHeader	*pstrFileHdr = (tstrFlashFileHeader*)pu8Buffer;

			fseek(fp,0,SEEK_END);
			u16FileSize = (uint16)ftell(fp);
			fseek(fp,0,SEEK_SET);

			/* Put the file header.
			*/
			strcpy(pstrFileHdr->acFileName, pcFileName);
			pstrFileHdr->u32FileSize = u16FileSize;
			pu8Buffer += sizeof(tstrFlashFileHeader);

			fread(pu8Buffer, 1, u16FileSize, fp);
			fclose(fp);

			u16FileSize = WORD_ALIGN(u16FileSize);
			pu8Buffer += u16FileSize;
			*ppu8OutBuffer = pu8Buffer;

		    pManifest->HTTP_used += sizeof(tstrFlashFileHeader);
            pManifest->HTTP_used += u16FileSize;
		} else {
			/**/
		}
	} else {
		/**/
	}
END:
	return u16FileSize;
}

/*********************************************/
static sint8 tls_cert_install(uint32 *pu32SrvSecSz)
{
	uint32			u32SrvSecSz		= 0;
	tenuWriteMode	enuWriteMode	= TLS_SRV_SEC_MODE_WRITE;

#ifdef __TLS_INSTALL_RSA_CRT__
	if(TlsCertStoreWriteCertChain(TLS_RSA_PRIV_KEY, TLS_RSA_CERT, TLS_CA_DIR, gau8TlsSrvSec, &u32SrvSecSz, enuWriteMode) == M2M_SUCCESS)
		enuWriteMode = TLS_SRV_SEC_MODE_APPEND;
#endif

#ifdef __TLS_INSTALL_ECDSA_CRT__
	TlsCertStoreWriteCertChain(NULL, TLS_ECDSA_CERT, TLS_CA_DIR, gau8TlsSrvSec, &u32SrvSecSz, enuWriteMode);
#endif

	if(pu32SrvSecSz != NULL)
		*pu32SrvSecSz = u32SrvSecSz;

	return M2M_SUCCESS;
}

/*****************************************
	GLOBAL FUNCTIONS
*****************************************/
sint8 get_firmwarefile_version(char * file, tstrM2mRev *pstrm2mrev)
{
	FILE *fp;
	sint8 ret = M2M_SUCCESS;
	uint32 sz = 0;
	fp = fopen(file, "rb");

	if(fp)
	{
		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fseek(fp, FIRMWARE_INFO_VECTOR_OFFSET, SEEK_SET);
		if(sz > sizeof(tstrInfoVector))
		{
			tstrInfoVector strinfo;
			fread(&strinfo,1,sizeof(tstrInfoVector),fp);
			if(strinfo.MagicNumber == INFO_VECTOR_MAGIC)
			{
				strfirmwareFileRev.u8DriverMajor	= M2M_GET_DRV_MAJOR(strinfo.VersionInfo);
				strfirmwareFileRev.u8DriverMinor	= M2M_GET_DRV_MINOR(strinfo.VersionInfo);
				strfirmwareFileRev.u8DriverPatch	= M2M_GET_DRV_PATCH(strinfo.VersionInfo);
				strfirmwareFileRev.u8FirmwareMajor	= M2M_GET_FW_MAJOR(strinfo.VersionInfo);
				strfirmwareFileRev.u8FirmwareMinor	= M2M_GET_FW_MINOR(strinfo.VersionInfo);
				strfirmwareFileRev.u8FirmwarePatch	= M2M_GET_FW_PATCH(strinfo.VersionInfo);
				strfirmwareFileRev.u16FirmwareSvnNum = strinfo.SvnNumber;

				m2m_memcpy(strfirmwareFileRev.BuildDate,strinfo.Builddate,sizeof(__DATE__));
				m2m_memcpy(strfirmwareFileRev.BuildTime,strinfo.Buildtime,sizeof(__TIME__));
				if(pstrm2mrev != NULL)
				{
					m2m_memcpy((uint8*)pstrm2mrev,(uint8*)&strfirmwareFileRev,sizeof(tstrM2mRev));
				}

				M2M_DBG("Firmware ver   : %u.%u.%u Svnrev %u\n", strfirmwareFileRev.u8FirmwareMajor, strfirmwareFileRev.u8FirmwareMinor,strfirmwareFileRev.u8FirmwarePatch,strfirmwareFileRev.u16FirmwareSvnNum);
				M2M_DBG("Min driver ver : %u.%u.%u\n", strfirmwareFileRev.u8DriverMajor,strfirmwareFileRev.u8DriverMinor,strfirmwareFileRev.u8DriverPatch);
				M2M_DBG("Firmware Build %s Time %s\n",strfirmwareFileRev.BuildDate,strfirmwareFileRev.BuildTime);

			}
			else
			{
				ret = M2M_ERR_FAIL;
				NM_BSP_PRINTF("File is not eligable for firmware version number <17.2 \n%s\n",file);
			}
		}
		else
		{
			ret = M2M_ERR_FAIL;
			NM_BSP_PRINTF("Incorrect file size \n%s\n",file);
		}
		fclose(fp);
	}
	else
	{
		ret = M2M_ERR_FAIL;
		NM_BSP_PRINTF("failed to open firmware file \n%s\n",file);
	}

	return ret;

}

sint8 init_custom_image_builder(char* custom_image_path)
{
	sint8 ret = M2M_ERR_FAIL;
	ciFile = fopen((const char *)custom_image_path,"wb");
	custom_image_build_enabled = 1;
	if(ciFile) {
		ret = M2M_SUCCESS;
	} else {
		ret = M2M_ERR_FAIL;
	}
	return ret;
}

sint8 make_custom_image(char* gain_file)
{
	FILE *fp;
	sint8 ret = M2M_SUCCESS;
	int FirmSz = 0,FileSz = 0;
	if(gain_file != NULL)
	{
		fp = fopen(gain_file, "rb");

		if(fp)
		{
			fseek(fp, 0L, SEEK_END);
			FileSz = ftell(fp);
			fseek(fp, 0L, SEEK_SET);

			if(FileSz > M2M_GAIN_FLASH_SZ)
			{
				M2M_ERR("Gain Table image greater than the gain flash section\n");
				return M2M_ERR_FAIL;
			}
			fseek(fp,/*sizeof(tstrM2mRev)+8*/0, SEEK_SET);
			fread(&gainTableFirmwareBuffer,FileSz,1,fp);
			fclose(fp);
		}
		else
		{
			ret = M2M_ERR_FAIL;
			NM_BSP_PRINTF("failed to open gain file \n%s\n",gain_file);
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
		NM_BSP_PRINTF("failed to open gain file \n%s\n",gain_file);
	}

	/*Modify the gain table in the custom image*/
	fseek(ciFile, M2M_GAIN_FLASH_OFFSET, SEEK_SET);
	fwrite(&gainTableFirmwareBuffer,FileSz,1,ciFile);
	fclose(ciFile);
	return ret;
}

sint8 builder_init(uint8 *fileName,char * pcFileOption)
{
	sint8 ret = M2M_ERR_FAIL;

	if(fileName != NULL)
	{
		gpFile = fopen((const char *)fileName,(const char *) pcFileOption);
		if(gpFile) {
			ret = M2M_SUCCESS;
		} else {
			ret = M2M_ERR_FAIL;
		}
	}
	else
	{
		 M2M_ERR("Not vaild file name\n");
	}

	return ret;
}

sint8 builder_deinit(void)
{
	sint8 ret = M2M_SUCCESS;
	if(gpFile) {
		fclose(gpFile);
	}

	return ret;
}

sint8 build_boot_firmware(tstrM2mRev *pstrm2mrev)
{
	uint32 tsz = 0,sz = 0;
	sint8 ret = M2M_SUCCESS;
	tstrM2mBinaryHeader binaryHeader;

	m2m_memset(gau8Firmware, 0xFF,M2M_BOOT_FIRMWARE_FLASH_SZ);
	ret = create_firmware_bin(0,M2M_BOOT_FILE,&tsz,"NMIS");
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Prog firmware not found \n");
		goto _END;
	}
	memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
	binaryHeader.flashOffset = M2M_BOOT_FIRMWARE_STARTING_ADDR;
	binaryHeader.payloadSize = M2M_BOOT_FIRMWARE_FLASH_SZ;
	ret = spitBinary(&binaryHeader,(char*)"fs_boot_firmware",gau8Firmware,M2M_BOOT_FIRMWARE_FLASH_SZ);
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed creating boot firmware \n");
		goto _END;
	}
	ret = write_to_file(gau8Firmware, M2M_BOOT_FIRMWARE_STARTING_ADDR, M2M_BOOT_FIRMWARE_FLASH_SZ);

	pManifest->BootFW_space = M2M_BOOT_FIRMWARE_FLASH_SZ;
	pManifest->BootFW_ofs = M2M_BOOT_FIRMWARE_STARTING_ADDR;
	pManifest->BootFW_used = tsz;

_END:
	return ret;
}

sint8 build_burst_fw(tstrM2mRev *pstrm2mrev,char * file)
{
	uint32 tsz = 0,sz = 0;
	sint8 ret = M2M_SUCCESS;
	FILE *pfil = NULL;
	tstrM2mBinaryHeader binaryHeader;

	m2m_memset(gau8Firmware, 0xFF, FLASH_2M_TOTAL_SZ);

	pfil = fopen(file,"rb");

	if(pfil != NULL) {
		uint32 u32Size;
		fseek (pfil , 0 , SEEK_END);
		u32Size = ftell (pfil);
		rewind (pfil);
		if(M2M_FIRMWARE_FLASH_SZ < u32Size) {
			ret = M2M_ERR_FAIL;
			goto _END;
		}
		gau8Firmware[0]='F',gau8Firmware[1]='T',gau8Firmware[2]='M',gau8Firmware[3]='A';
		fread(&gau8Firmware[4], 1, u32Size, pfil);
		fclose(pfil);
		memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
		binaryHeader.flashOffset = M2M_OTA_IMAGE2_OFFSET;
		binaryHeader.payloadSize = u32Size;
		ret = spitBinary(&binaryHeader,(char*)"fs_ate",gau8Firmware,u32Size);
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Failed creating ate firmware image\n");
			goto _END;
		}
		ret = write_to_file(gau8Firmware, M2M_OTA_IMAGE2_OFFSET, u32Size);
	} else {
		M2M_PRINT("ATE file not found \n");
		ret = M2M_ERR_FAIL;
	}

_END:
	return ret;
}

sint8 build_ps_firmware(void)
{
	sint8 ret = M2M_SUCCESS;

#if 0
	m2m_memset(gau8Firmware, 0xFF, M2M_PS_FIRMWARE_FLASH_SZ);
	ret = write_to_file(gau8Firmware, M2M_PS_FIRMWARE_FLASH_OFFSET, M2M_PS_FIRMWARE_FLASH_SZ);
#endif

	return ret;
}

sint8 build_conn_params(void)
{
	sint8 ret = M2M_SUCCESS;

	m2m_memset(gau8Firmware, 0xFF, M2M_CACHED_CONNS_FLASH_SZ);
	ret = write_to_file(gau8Firmware, M2M_CACHED_CONNS_FLASH_OFFSET, M2M_CACHED_CONNS_FLASH_SZ);

	pManifest->WIFIData_space = M2M_CACHED_CONNS_FLASH_SZ;
	pManifest->WIFIData_used = 0;
	pManifest->WIFIData_ofs = M2M_CACHED_CONNS_FLASH_OFFSET;

	return ret;
}
static uint8 crc7(uint8 crc, const uint8 *b, int len)
{
	uint16 i, g;

	uint8 mask = 0x9;

	/* initialize register */
	uint8 reg = crc, inv;

	for(i = 0; i < len; i++)
	{
		for(g = 0; g < 8; g++)
		{
			inv = (((b[i] << g) & 0x80) >> 7) ^ ((reg >> 6) & 1);
			reg = ((reg << 1) & 0x7f) ^ (mask * inv);
		}
	}

	return reg;
}
sint8 build_control_sec(tstrM2mRev *pstrm2mrev,uint8 Chip)
{
	sint8 ret;
	tstrM2mBinaryHeader binaryHeader;
	memset(&strOtaControlSec,0,sizeof(tstrOtaControlSec));
	strOtaControlSec.u32OtaMagicValue = OTA_MAGIC_VALUE;
#ifdef DOWNLOAD_ROLLBACK
	strOtaControlSec.u32OtaCurrentworkingImagOffset = M2M_OTA_IMAGE2;
	strOtaControlSec.u32OtaRollbackImageOffset = M2M_OTA_IMAGE1;
#else
	strOtaControlSec.u32OtaCurrentworkingImagOffset = M2M_OTA_IMAGE1_OFFSET;
#endif
	/**
		u32OtaFormatVersion
		NA   NA   NA   Flash version   cs struct version
		00   00   00   00              00
	*/
	strOtaControlSec.u32OtaFormatVersion = (OTA_FORMAT_VER_1)|(FLASH_MAP_VERSION << 8);
	strOtaControlSec.u32OtaRollbackImageOffset = M2M_OTA_IMAGE2_OFFSET;
	strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer =
		((uint32)strfirmwareFileRev.u8FirmwareMajor << 8)  | ((uint32)strfirmwareFileRev.u8FirmwareMinor <<  4)|
		((uint32)strfirmwareFileRev.u8DriverMajor << 24) | ((uint32)strfirmwareFileRev.u8DriverMinor  << 20)|
	((uint32)strfirmwareFileRev.u8FirmwarePatch << 0) | ((uint32)strfirmwareFileRev.u8DriverPatch  << 16);
	strOtaControlSec.u32OtaRollbackImageValidStatus = OTA_STATUS_INVALID /*OTA_ROLLB_STATUS_inVALID*/;
	strOtaControlSec.u32OtaCortusAppRollbackValidSts = OTA_STATUS_INVALID;
	strOtaControlSec.u32OtaCortusAppWorkingValidSts = OTA_STATUS_INVALID;

	m2m_memset(gau8Firmware, 0xFF, M2M_CONTROL_FLASH_TOTAL_SZ);
	strOtaControlSec.u32OtaControlSecCrc = crc7(0x7f,(uint8*)&strOtaControlSec,sizeof(tstrOtaControlSec) - 4);
	memcpy(gau8Firmware,(uint8*)&strOtaControlSec,sizeof(tstrOtaControlSec));
	memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
	binaryHeader.flashOffset = M2M_CONTROL_FLASH_OFFSET;
	binaryHeader.payloadSize = M2M_CONTROL_FLASH_TOTAL_SZ;
	ret = spitBinary(&binaryHeader,(char*)"fs_control_sec",gau8Firmware,M2M_CONTROL_FLASH_TOTAL_SZ);
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed to create control section binary\n");
		ret = M2M_ERR_FAIL;
		goto _END;
	}
	ret = write_to_file(gau8Firmware, M2M_CONTROL_FLASH_OFFSET, M2M_CONTROL_FLASH_TOTAL_SZ);
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed to download firmware\n");
		ret = M2M_ERR_FAIL;
		goto _END;
	}

	pManifest->Control_space = M2M_CONTROL_FLASH_TOTAL_SZ;
	pManifest->Control_used = sizeof(tstrOtaControlSec);
	pManifest->Control_ofs = M2M_CONTROL_FLASH_OFFSET;

_END:
	return ret;
}

sint8 build_firmware(tstrM2mRev *pstrm2mrev,char * file,char * otafilename,uint8 Chip,uint8 ota)
{
	uint32 tsz = 0,sz = 0;
	sint8 ret = M2M_SUCCESS;
	uint32 u32ImageOffset = 0;
	tstrM2mBinaryHeader binaryHeader;

	m2m_memset(gau8Firmware, 0xFF, M2M_FIRMWARE_FLASH_SZ);
#ifdef	_PROGRAM_POWER_SAVE_
	{
		/*For 1003A0 we ignore the downloader firmware*/
		ret = create_firmware_bin(u32ImageOffset,M2M_PROGRAM_FILE,&sz,"NMIS");
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Prog firmware not found \n");
			goto _END;
		}
		u32ImageOffset += sz;
		tsz += sz;
	}
#endif
	ret = create_firmware_bin(u32ImageOffset,file,&sz,"NMID");
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Main firmware not found \n");
		goto _END;
	}
	u32ImageOffset+=sz;
	tsz += sz;
	if((tsz) > (M2M_FIRMWARE_FLASH_SZ))
	{
		ret = M2M_ERR_FAIL;
		M2M_PRINT("Exceed firmware Size will overwrite user section %x\n",tsz);
		goto _END;
	}
	memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
	binaryHeader.flashOffset = M2M_FIRMWARE_FLASH_OFFSET;
	binaryHeader.payloadSize = M2M_FIRMWARE_FLASH_SZ;
	ret = spitBinary(&binaryHeader,"fs_main_firmware",gau8Firmware,M2M_FIRMWARE_FLASH_SZ);
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed to emit main firmware\n");
		ret = M2M_ERR_FAIL;
		goto _END;
	}
	ret = write_to_file(gau8Firmware, M2M_FIRMWARE_FLASH_OFFSET, M2M_FIRMWARE_FLASH_SZ);
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed to download firmware\n");
		ret = M2M_ERR_FAIL;
		goto _END;
	}

	if(ota)
	{
		uint32	u32OTASize;

		if(pstrm2mrev->u32Chipid == 0x1503a0)
		{
			u32OTASize = OTA_IMAGE_SIZE;
		}
		else
		{
			u32OTASize	= WORD_ALIGN(tsz);
		}
		PrepareOta(gau8Firmware, u32OTASize, otafilename);
		M2M_PRINT("\r\n>>OTA Image has been created.\r\n");
	}

	pManifest->FW_space = M2M_FIRMWARE_FLASH_SZ;
	pManifest->FW_used  = tsz;
	pManifest->FW_ofs   = M2M_FIRMWARE_FLASH_OFFSET;

_END:
	return ret;
}

static wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = malloc(sizeof(wchar_t) * size);
	mbstowcs(wText, text, size);
	return wText;
}

sint8 build_http_config(tstrM2mRev *pstrm2mrev,char *pcSrcFolder, uint8 u8ModifyForOta)
{
	uint32	u32Size = 0;
	uint8	*pu8Tmp = gau8Firmware;
	sint8	ret = M2M_ERR_FAIL;
	tstrM2mBinaryHeader binaryHeader;

	m2m_memset(gau8Firmware, 0xFF, M2M_HTTP_MEM_FLASH_SZ);
	pManifest->Total_size = sizeof(gau8Firmware);

	*((uint32*)pu8Tmp) = HTTP_FLASH_SECTION_MAGIC | HTTP_FLASH_SECTION_VERSION;
	pu8Tmp += sizeof(uint32);

	if (strlen(pcSrcFolder) < 256 - 5)
	{
		WIN32_FIND_DATA fdFile;
		HANDLE hFind = NULL;

		char sPath[256];
		sprintf(sPath, "%s\\*.*", pcSrcFolder);

		// if we ever need wide paths for where the files are stored then mess with LOCALE
		// all etc in gcc, otherwise lets stick with ascii
		#if defined(__GNUC__)
		M2M_PRINT("searching ascii %s\n",sPath);
		hFind = FindFirstFileA(sPath, &fdFile);
		#else
		wchar_t* wPath = charToWChar(sPath);
		M2M_PRINT("searching mbcs %s\n",sPath);
		hFind = FindFirstFile(wPath, &fdFile);
		#endif // GCC_COMPILER

		if (hFind == INVALID_HANDLE_VALUE)
			M2M_ERR("Unable to read HTTP files directory [%s]\n", pcSrcFolder);
		else
		{
		    pManifest->HTTP_used = 0;
		    pManifest->HTTP_space = M2M_HTTP_MEM_FLASH_SZ;
		    pManifest->HTTP_ofs = M2M_HTTP_MEM_FLASH_OFFSET;

			do
			{
				char sFilename[256];
				#if defined(__GNUC__)
				strncpy(sFilename, fdFile.cFileName, 256);
				#else
				wcstombs(sFilename, fdFile.cFileName, 256);
				#endif // GCC_COMPILER
				M2M_PRINT("found %s\n",sFilename);

				if (strcmp(sFilename, ".") != 0 && strcmp(sFilename, "..") != 0)
				{
					if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (http_load_file(pcSrcFolder, sFilename, &pu8Tmp) > 0)
							M2M_PRINT("Loaded HTTP file: %s\n", sFilename);
						else
							M2M_ERR("Failed to load HTTP file: %s\n", sFilename);
					}
				}
			} while (FindNextFile(hFind, &fdFile));

			FindClose(hFind);

			u32Size = (uint32)(pu8Tmp - gau8Firmware);
			if (u32Size <= M2M_HTTP_MEM_FLASH_SZ)
			{
				//if(programmer_erase(M2M_HTTP_MEM_FLASH_OFFSET,M2M_HTTP_MEM_FLASH_SZ) == M2M_SUCCESS)
				{
					while (u32Size < M2M_HTTP_MEM_FLASH_SZ)
					{
						gau8Firmware[u32Size++] = 255;
					}
					if (!u8ModifyForOta)
					{
						memcpy(&binaryHeader.binVerInfo, pstrm2mrev, sizeof(tstrM2mRev));
						binaryHeader.flashOffset = M2M_HTTP_MEM_FLASH_OFFSET;
						binaryHeader.payloadSize = M2M_HTTP_MEM_FLASH_SZ;
						ret = spitBinary(&binaryHeader, (char*)"fs_http", gau8Firmware, M2M_HTTP_MEM_FLASH_SZ);
						/* HTTP offset is calculated from the start of the flash				*/
						ret = write_to_file(gau8Firmware, M2M_HTTP_MEM_FLASH_OFFSET, M2M_HTTP_MEM_FLASH_SZ);
					}
					else
					{
						memcpy(&binaryHeader.binVerInfo, pstrm2mrev, sizeof(tstrM2mRev));
						binaryHeader.flashOffset = M2M_HTTP_MEM_FLASH_OFFSET - (M2M_OTA_IMAGE1_OFFSET - (CRYPTO_SHA256_DIGEST_SIZE + sizeof(tstrOtaInitHdr)));
						binaryHeader.payloadSize = M2M_HTTP_MEM_FLASH_SZ;
						ret = spitBinary(&binaryHeader, (char*)"fs_http_ota", gau8Firmware, M2M_HTTP_MEM_FLASH_SZ);
						/* OTA image- HTTP offset is calculated from the start of the image itself */
						ret = write_to_file(gau8Firmware, M2M_HTTP_MEM_FLASH_OFFSET - (M2M_OTA_IMAGE1_OFFSET - (CRYPTO_SHA256_DIGEST_SIZE +
							sizeof(tstrOtaInitHdr))), M2M_HTTP_MEM_FLASH_SZ);
					}
					if (ret != M2M_SUCCESS)
					{
						M2M_PRINT("Failed to download HTTP File(s)\n");
					}
					else
					{
						/*{
							uint32					u32FlashAddr = M2M_HTTP_MEM_FLASH_OFFSET;
							tstrFlashFileHeader		strFileHdr;

							for(; u32FlashAddr < (M2M_HTTP_MEM_FLASH_OFFSET + M2M_HTTP_MEM_FLASH_SZ) ;)
							{
								programmer_read((uint8*)&strFileHdr, u32FlashAddr, sizeof(tstrFlashFileHeader));
								u32FlashAddr += sizeof(tstrFlashFileHeader);

								if(strFileHdr.u32FileSize >= M2M_HTTP_MEM_FLASH_SZ)
									break;

								M2M_INFO("<%04d><%s>\n", strFileHdr.u32FileSize, strFileHdr.acFileName);
								u32FlashAddr += WORD_ALIGN(strFileHdr.u32FileSize);
							}
						}
						*/
					}
				}
				//else
				{
					//M2M_PRINT("Failed to erase spi flash\n");
				}
			}
			else
			{
				M2M_ERR("HTTP File(s) exceed max size %d %d\n", u32Size, M2M_HTTP_MEM_FLASH_SZ);
			}
		}

		#if !defined(__GNUC__)
		free(wPath);
		#endif // GCC_COMPILER
	}
	else
	{
		M2M_ERR("HTTP Source Folder Path is too long\n");
	}

	return ret;
}

/**
look-up table
**/
#ifdef _PLL_LOOKUP_
typedef unsigned int UWORD32;
#define WIFI_PINMUX_SEL_0		(0x1408)
#define WIFI_PLL_INTERNAL_1		(0x1414)
#define WIFI_PLL_INTERNAL_2		(0x1418)
#define WIFI_PLL_INTERNAL_3		(0x141c)
#define WIFI_PLL_INTERNAL_4		(0x1420)
#define WIFI_MISC_CTRL			(0x1428)
#define USE_XO_DURING_PLL_UPDATE_SYS (BIT19)
#define USE_XO_DURING_PLL_UPDATE_TX  (BIT21)
#define USE_XO_DURING_PLL_UPDATE_RX  (BIT22)
#define USE_XO_DURING_PLL (USE_XO_DURING_PLL_UPDATE_SYS)
#define PWR_SEQ_TX_RX_CTRL_0       (0x1480)
#define PWR_SEQ_DCDC_EN_BYP_EN      (BIT0)
#define PWR_SEQ_DCDC_EN_MASK        (BIT3+BIT2+BIT1+BIT0)
#define WIFI_HOST_XMT_CTRL	(0x106c)
#define WIFI_HOST_RCV_CTRL_0	(0x1070)
#define WIFI_HOST_RCV_CTRL_1	(0x1074)
#define WIFI_HOST_VMM_CTRL (0x1078)
#define WIFI_HOST_RX_CTRL	(0x1080)
#define WIFI_HOST_RX_EXTRA_SIZE	(0x1084)
#define WIFI_HOST_TX_CTRL	(0x1088)
#define NUM_CHANNELS           14 /* Num Channels supp by RF in 2G4 Band */

tstrChannelParm strChnParm[NUM_CHANNELS];

#if 0
uint32	au32RegAddr[TX_GAIN_NUM_OF_REGISTERS];
tstrGain_settingValues	gtstrGainSettingValue[NUM_CHANNELS];

sint8 calc_gain_values(void)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 0;
	while(loopCntr < TX_GAIN_NUM_OF_REGISTERS)
	{
		au32RegAddr[loopCntr] = TX_GAIN_REG_BASE_ADDRESS+(loopCntr*4);
		loopCntr++;
	}
	for(loopCntr=0; loopCntr<NUM_CHANNELS; loopCntr++)
	{
		gtstrGainSettingValue[loopCntr].is_valid = TX_GAIN_VALIDATION_NUMBER>>1;
		gtstrGainSettingValue[loopCntr].reg_ver = TX_GAIN_VER_NUMBER;
		gtstrGainSettingValue[loopCntr].num_of_slots = TX_GAIN_NUM_OF_REGISTERS;
		gtstrGainSettingValue[loopCntr].channel_num = loopCntr;
		gtstrGainSettingValue[loopCntr].reg_40 = 0x019701C9;
		gtstrGainSettingValue[loopCntr].reg_44 = 0x00000143;
		gtstrGainSettingValue[loopCntr].reg_48 = 0x00000000;
		gtstrGainSettingValue[loopCntr].reg_4C = 0x00000000;
		gtstrGainSettingValue[loopCntr].reg_50 = 0x11110000;
		gtstrGainSettingValue[loopCntr].reg_54 = 0x22221111;
		gtstrGainSettingValue[loopCntr].reg_58 = 0x00002222;
	}

	return ret;
}
#endif
/**
*	@fn			sint8 programmer_look_up_table(void)
*	@brief		ceate PLL lookup table
*	@return		creation status
*	@author		M.S.M
*	@version	1.0
*/
sint8 build_look_up_table(tstrM2mRev *pstrm2mrev,double xoOffset)
{
	sint8 ret = M2M_SUCCESS;
	uint32 offset = M2M_PLL_FLASH_OFFSET;
	uint32 val32;
	uint8 ch = 0;
	uint32 magic[2] = {0};
	uint32 n2, f, m, g;
	tstrM2mBinaryHeader binaryHeader;
	int i = 0;
	double xo = 26.0;//chip.inp.xo;166
	double xo_to_VCO = xo;
	double lo, lo_actual;
	double rffrequency, xo_offset = 0;
	double n1,dec,inv;
	double gMoG  = 60000000;
	uint32 chipid;
	EFUSEProdStruct efuse_struct;
	uint8 skip_bank_check = 0;
	/*get xo_offset*/
	chipid = REV_3A0;
	memset(&efuse_struct, 0, sizeof(efuse_struct));

	//programmer_read((uint8*)&magic,offset,sizeof(magic));
	//if((magic[0] != MAGIC_NUMBER_OF_PLL)||(magic[1] != ((uint32)xo_offset)))
	{
		M2M_PRINT("Creating look up table for PLL ..\n");
		for(ch = 0;ch < NUM_CHANNELS; ch++)
		{
			if (ch < 13)
			{
				rffrequency = 2412.0 + (ch*5);
			} else {
				rffrequency = 2484.0;
			}

			if(efuse_struct.FreqOffset_used) {
				uint32 tmp;
				tmp = (efuse_struct.FreqOffset > (1<<14)) ? efuse_struct.FreqOffset-(1<<15): efuse_struct.FreqOffset;
				xo_offset = ((double)tmp) / (1 << 6);
			} else {
				xo_offset = xoOffset;
			}
			xo = xo*(1 + (xo_offset/1000000.0));

			lo = rffrequency *  2;

			if(REV(chipid) < REV_B0) {
				val32 = 0x4008f600;//nm_read_reg(WIFI_PLL_INTERNAL_3);
			} else {
				val32 = 0x4008fa0b;//nm_read_reg(WIFI_PLL_INTERNAL_3);
			}
			if (((val32 >> 8)&0x3)==0x2)
			{
				xo_to_VCO = 2* xo;
			}

			n2 = (UWORD32)(lo/xo_to_VCO);

			f = (UWORD32)( ((lo/xo_to_VCO) - n2) * (1 << 19) + 0.5);

			lo_actual  = (double)xo_to_VCO * (double)(n2+((double)f/(1<<19)));

			val32 = ((n2 & 0x1fful) << 19) | ((f & 0x7fffful) << 0);
			val32 |= (1ul << 31);
			strChnParm[ch].u32PllInternal1 = val32;
			//M2M_PRINT("ch %d pll1 %x \n",ch,val32);

			m = (UWORD32)(lo_actual/80.0);
			g = (UWORD32)((lo_actual/80.0 - m)* (1<<19));
			gMoG = (double)(m+((double)g/(1<<19)));

			val32 = ((m & 0x1fful) << 19) | ((g & 0x7fffful) << 0);
			val32 &= ~(1ul << 28); /* Dither must be disbled */

			strChnParm[ch].u32PllInternal4 = val32;
			n1 = (UWORD32) (((60.0/gMoG) * (1ul << 22)));
			dec = (UWORD32) ((((60.0/gMoG) * (1ul << 22)) - n1) * (1ul << 31));
			inv = (UWORD32) (((1ul << 22)/(n1/(1ul << 11)))+0.5);
			strChnParm[ch].WlanRx1 = (uint32)n1;
			strChnParm[ch].WlanRx3 = (uint32)dec;
			strChnParm[ch].WlanRx2 = (uint32)inv;
			//M2M_PRINT("RX = %f %f %f\n",n1,dec,inv);

			n1 = (UWORD32)(((gMoG/60.0) * (1ul << 22)));
			dec = (UWORD32) ((((gMoG/60.0) * (1ul << 22)) - n1) * (1ul << 31));
			inv = (UWORD32) (((1ul << 22)/(n1/(1ul << 11)))+0.5);
			strChnParm[ch].WlanTx1 = (uint32)n1;
			strChnParm[ch].WlanTx3 = (uint32)dec;
			strChnParm[ch].WlanTx2 = (uint32)inv;
			//M2M_PRINT("TX = %d %d %d\n",n1,dec,inv);
		}
		if((sizeof(strChnParm)+sizeof(magic))>M2M_PLL_FLASH_SZ)
		{
			M2M_PRINT("Excced pll sector size\n");
			goto ERR;
		}

		magic[0] = PLL_MAGIC_NUMBER;
		magic[1] = (uint32)xo_offset;

		m2m_memset(gau8Firmware, 0xFF, M2M_CONFIG_SECT_TOTAL_SZ);

		memcpy(gau8Firmware, (uint8*)&magic, sizeof(magic));
		memcpy(&gau8Firmware[sizeof(magic)], (uint8*)&strChnParm, sizeof(strChnParm));

        pManifest->PLL_space = M2M_PLL_FLASH_SZ;
        pManifest->PLL_used = sizeof(magic) + sizeof(strChnParm);
        pManifest->PLL_ofs = M2M_PLL_FLASH_OFFSET;
	}
	/*Build Gain Configuration */
#if 0
	calc_gain_values();

	memcpy(&gau8Firmware[M2M_PLL_FLASH_SZ],(uint8 *)&gtstrGainSettingValue, sizeof(gtstrGainSettingValue));

	pManifest->Gain_space = M2M_CONFIG_SECT_TOTAL_SZ - M2M_PLL_FLASH_SZ;
	pManifest->Gain_used = sizeof(gtstrGainSettingValue);
	pManifest->Gain_ofs = M2M_PLL_FLASH_OFFSET + M2M_PLL_FLASH_SZ;
#else
	pManifest->Gain_space = M2M_CONFIG_SECT_TOTAL_SZ - M2M_PLL_FLASH_SZ;
	pManifest->Gain_used = 0;
	pManifest->Gain_ofs = M2M_PLL_FLASH_OFFSET + M2M_PLL_FLASH_SZ;
#endif

	memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
	binaryHeader.payloadSize = M2M_CONFIG_SECT_TOTAL_SZ;
	binaryHeader.flashOffset = M2M_PLL_FLASH_OFFSET;
	ret = spitBinary(&binaryHeader,"fs_pll_section",gau8Firmware,M2M_PLL_FLASH_SZ);
	ret = write_to_file(gau8Firmware, M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ);
	if(ret!=M2M_SUCCESS) goto ERR;
	//M2M_PRINT("done\n");
ERR:
	return ret;
}
#endif
/*********************************************/

sint8 build_certificates(tstrM2mRev *pstrm2mrev, uint8* vflash)
{
	sint8	ret			= M2M_SUCCESS;
	uint32	u32Idx;
	uint32	u32nCerts;
	uint32	u32CertSz;
	uint8	*pu8RootCert;
	char	**ppCertNames;

	tstrM2mBinaryHeader binaryHeader;

	ListDirectoryContents(ROOT_CERT_PATH, "cer", &ppCertNames, &u32nCerts);
	if(u32nCerts != 0)
	{
		for(u32Idx = 0; u32Idx < u32nCerts; u32Idx ++)
		{
			if(ReadFileToBuffer(ppCertNames[u32Idx], &pu8RootCert, &u32CertSz) == M2M_SUCCESS)
			{
				if(WriteRootCertificate(pu8RootCert, u32CertSz, vflash) != 0)
				{
					printf("Error Writing certificate.\n");
					ret = M2M_ERR_FAIL;
					goto _END;
				}
				free(pu8RootCert);
			}
		}
	}
	else
	{
		printf("Unable to find certificates\r\n");
		ret = M2M_ERR_FAIL;
		goto _END;
	}

	if(ret == M2M_SUCCESS)
	{
		FILE *fp = NULL;
		uint32 sz;

		fp = fopen("root_cert.bin", "rb");
		if(fp)
		{
			memset(gau8Firmware, 0xFF, M2M_TLS_ROOTCER_FLASH_SIZE);
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			fread(gau8Firmware, sz, 1, fp);
			fclose(fp);
		}
		memcpy(&binaryHeader.binVerInfo,pstrm2mrev,sizeof(tstrM2mRev));
		binaryHeader.flashOffset = M2M_TLS_ROOTCER_FLASH_OFFSET;
		binaryHeader.payloadSize = M2M_TLS_ROOTCER_FLASH_SIZE;
		ret = spitBinary(&binaryHeader,"fs_certificate",gau8Firmware,M2M_TLS_ROOTCER_FLASH_SIZE);
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Failed to emit certificate section\n");
			ret = M2M_ERR_FAIL;
			goto _END;
		}
		ret = write_to_file(gau8Firmware, M2M_TLS_ROOTCER_FLASH_OFFSET, M2M_TLS_ROOTCER_FLASH_SIZE);
		if(ret != M2M_SUCCESS)
		{
			goto _END;
		}
		remove("root_cert.bin");
	}
	else
	{
		ret = -1;
	}

_END:
	return ret;
}

/*********************************************/
sint8 build_tls_server_flash(void)
{
	sint8	ret = M2M_ERR_FAIL;
	if(tls_cert_install(NULL) == M2M_SUCCESS)
	{
		ret = write_to_file(gau8TlsSrvSec, M2M_TLS_SERVER_FLASH_OFFSET, M2M_TLS_SERVER_FLASH_SIZE);
	}
	return ret;
}

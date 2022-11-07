#include "app_setup.h"
#include "driver/include/m2m_types.h"
#include "spi_flash/include/spi_flash.h"


/*Gloabel*/
uint8 gau8Firmware[512*1024];
tstrOtaControlSec  strOtaControlSec  = {0}; 


/*********************************************/
/* STATIC FUNCTIONS							 */
/*********************************************/
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
sint8 update_cortusapp_in_cs()
{
	sint8 ret = M2M_SUCCESS;
	uint32 offset = 0;
	uint32 u32ChipId;

	u32ChipId = GET_CHIPID();
	ret = programmer_read((uint8*)&strOtaControlSec, M2M_CONTROL_FLASH_OFFSET, sizeof(tstrOtaControlSec));
	if(ret != M2M_SUCCESS)
	{
		M2M_PRINT("Failed to read spi flash\n");
		ret = M2M_ERR_FAIL;
		goto ERR;
	}
	if(strOtaControlSec.u32OtaMagicValue == OTA_MAGIC_VALUE)
	{
		if(spi_flash_get_size() == 2){
			strOtaControlSec.u32OtaCortusAppWorkingValidSts = OTA_STATUS_INVALID;
			strOtaControlSec.u32OtaCortusAppRollbackValidSts = OTA_STATUS_INVALID;

		} else if (spi_flash_get_size() == 4){
			strOtaControlSec.u32OtaCortusAppWorkingOffset = M2M_APP_4M_MEM_FLASH_OFFSET;
			strOtaControlSec.u32OtaCortusAppWorkingValidSts = OTA_STATUS_VALID;
			strOtaControlSec.u32OtaCortusAppRollbackValidSts = OTA_STATUS_INVALID;
			

		} else if (spi_flash_get_size() == 8)
		{
			strOtaControlSec.u32OtaCortusAppWorkingOffset    = M2M_APP_8M_MEM_FLASH_OFFSET;
			strOtaControlSec.u32OtaCortusAppWorkingValidSts  = OTA_STATUS_VALID;
			strOtaControlSec.u32OtaCortusAppRollbackOffset   = M2M_APP_OTA_MEM_FLASH_OFFSET;
			strOtaControlSec.u32OtaCortusAppRollbackValidSts = OTA_STATUS_INVALID;
		}
		M2M_INFO("Offset %x\n",strOtaControlSec.u32OtaCortusAppWorkingOffset);
		programmer_erase(M2M_CONTROL_FLASH_OFFSET,sizeof(tstrOtaControlSec));
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Failed to erase spi flash\n");
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		strOtaControlSec.u32OtaControlSecCrc = crc7(0x7f,(uint8*)&strOtaControlSec,sizeof(tstrOtaControlSec) - 4);
		programmer_write((uint8*)&strOtaControlSec,M2M_CONTROL_FLASH_OFFSET,sizeof(tstrOtaControlSec));
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Failed to write spi flash\n");
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
		//M2M_ERR("Invaild Control structure\n");
		goto ERR;
	}
ERR:
	return ret;
}
/**
*	@fn			sint8 Read_app_files(*sz)
*	@brief		Download Cortus APP
*	@param[OUT]	* sz
*					pointer to data size
*	@return		Reading status
*	@author		M.S.M
*	@version	1.0
*/
static sint8 Read_app_files(uint32 *sz,char * file)
{
	sint8 ret = M2M_ERR_FAIL;
	/*4-Magic*/
	/*4-code size*/
	/*code section*/
	if(file != NULL)
	{
		uint8 *pu8Buff;
		FILE *fpc;
		uint32 magic = M2M_MAGIC_APP;
		uint32 u32Csz = 0;
		*sz  = 0;
		pu8Buff = (uint8*)&gau8Firmware[0];
		m2m_memcpy(pu8Buff ,(uint8*)&magic, 4);
		pu8Buff += 4;
		*sz += 4; 
		fpc = fopen(file,"rb");
		if(fpc != NULL)
		{
			fseek(fpc, 0L, SEEK_END);
			u32Csz = ftell(fpc);
			fseek(fpc, 0L, SEEK_SET);
			m2m_memcpy(pu8Buff,(uint8*)&u32Csz, 4);
			pu8Buff += 4;/*code size*/
			*sz += 4; 
			fread(pu8Buff,u32Csz,1,fpc);
			pu8Buff += u32Csz;
			*sz += u32Csz; 
			fclose(fpc);
			ret = M2M_SUCCESS;
		}
	}
	return ret;

}

/**
*	@fn			sint8 spi_flash_download_app(void)
*	@brief		Download Cortus APP
*	@return		Downloading status
*	@author		M.S.M
*	@version	1.0
*/
sint8 spi_flash_download_app(uint8 * file)
{
	uint32 offset;
	uint32 tsz = 0;
	sint8 ret = M2M_SUCCESS;
	uint32 u32FlashAppSz = 0;
	uint32 u32AppSz = 0;
	m2m_memset(gau8Firmware, 0xFF, M2M_APP_4M_MEM_FLASH_SZ);

	ret = Read_app_files(&tsz,file);
	if(ret != M2M_SUCCESS) 
	{
		M2M_ERR("App File not found\n");
		goto ERR;
	}

	if(spi_flash_get_size() > 2){
		u32FlashAppSz = M2M_APP_4M_MEM_FLASH_SZ;
	}else{
		M2M_ERR("Cortus app is not supported any more on 2M flash starting from release 19.4.0\n");
		ret = M2M_ERR_FAIL;
		goto ERR;
	}
	if(tsz > u32FlashAppSz){
		M2M_ERR("%d Firmware exceed max size %d\n",tsz,u32FlashAppSz);
		ret = M2M_ERR_FAIL;
		goto ERR;
	}
	if (update_cortusapp_in_cs() == M2M_SUCCESS)
	{
		offset = strOtaControlSec.u32OtaCortusAppWorkingOffset;
		ret = programmer_erase(offset,u32FlashAppSz);
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("Failed to erase spi flash\n");
			ret = M2M_ERR_FAIL;
			goto ERR;
		}

		ret = programmer_write(gau8Firmware, offset, tsz);
		if(ret != M2M_SUCCESS){
			M2M_PRINT("Failed to download App\n");
			ret = M2M_ERR_FAIL;
			goto ERR;
		}

#if 0
		{
			FILE * fp;
			//m2m_memset(gau8Firmware, 0, u32FlashAppSz);
			fp = fopen("test.bin","wb");
			if(fp != NULL)
			{
				programmer_read(gau8Firmware,0,512*1024);
				fwrite(gau8Firmware,1,512*1024,fp);
				fclose(fp);
			}

		}
#endif
	}
ERR:
	return ret;
	
}

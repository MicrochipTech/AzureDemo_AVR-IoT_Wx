
#include "../efuse/efuse.h"
#include "../fw_info/fw_info.h"
#include "common_values.h"
#include "firmware_setup.h"
#include "programmer.h"
#include "ota_hdr.h"


/*Firmware File Revison info*/
tstrM2mRev strfirmwareFileRev = {0};
tstrOtaControlSec  strOtaControlSec  = {0};

/*********************************************/
/* STATIC FUNCTIONS							 */
/*********************************************/

sint8 get_flash_firmware_version(tstrM2mRev *pstrCurentFirmware,tstrM2mRev *pstrOtaFirmware)
{
	sint8 ret = M2M_SUCCESS;
	uint32 offset = 0;
	uint32 u32ChipId;
	tstrInfoVector strinfo = {0};
	memset(pstrOtaFirmware,0,sizeof(tstrM2mRev));
	memset(pstrCurentFirmware,0,sizeof(tstrM2mRev));
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
		if(pstrCurentFirmware != NULL)
		{
			pstrCurentFirmware->u8DriverMajor	= M2M_GET_DRV_MAJOR(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u8DriverMinor	= M2M_GET_DRV_MINOR(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u8DriverPatch	= M2M_GET_DRV_PATCH(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u8FirmwareMajor	= M2M_GET_FW_MAJOR(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u8FirmwareMinor	= M2M_GET_FW_MINOR(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u8FirmwarePatch	= M2M_GET_FW_PATCH(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer);
			pstrCurentFirmware->u32Chipid = u32ChipId;
		}
		if(pstrOtaFirmware != NULL)
		{
			pstrOtaFirmware->u8DriverMajor		= M2M_GET_DRV_MAJOR(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u8DriverMinor		= M2M_GET_DRV_MINOR(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u8DriverPatch		= M2M_GET_DRV_PATCH(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u8FirmwareMajor	= M2M_GET_FW_MAJOR(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u8FirmwareMinor	= M2M_GET_FW_MINOR(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u8FirmwarePatch	= M2M_GET_FW_PATCH(strOtaControlSec.u32OtaRollbackImagFirmwareVer);
			pstrOtaFirmware->u32Chipid = u32ChipId;
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
		//M2M_ERR("Invaild Control structure\n");
		goto ERR;
	}
	/*
		get the main firmware version
	*/
	offset = strOtaControlSec.u32OtaCurrentworkingImagOffset;
	if(M2M_GET_FW_VER(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer) >= M2M_GET_FW_VER(REL_19_4_0_VER))
	{
		uint32 u32DlSize;
	    /* if (release >= 19.4.0 ) then programmer firmware equal actual size (parse it) for 1003A0/1002B0,
		/** how to detect {NMIS, small size < 10K} then move actual size then read {NMID, huge size > 10K}*/
		ret = programmer_read((uint8*)&u32DlSize, offset + 4 /*NMIS size*/, sizeof(uint32));
		offset += u32DlSize;
	} else if((M2M_GET_FW_VER(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer) >= M2M_GET_FW_VER(REL_19_2_0_VER))&&
		(M2M_GET_FW_VER(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer) < M2M_GET_FW_VER(REL_19_4_0_VER))) {
		/** if (release >= 19.2.0 ) && (release < 19.4.0) then programmer firmware equal (zero for 1003A0) /(2K for 1002B0),
		how to detect {NMIS,Huge size > 10K}*/
		if(REV(u32ChipId) != REV_3A0){
			offset += SFMV0_PROGRAM_SZ; /*TODO: switch it to old map 0*/
		}
	} else if(M2M_GET_FW_VER(strOtaControlSec.u32OtaCurrentworkingImagFirmwareVer) < M2M_GET_FW_VER(REL_19_2_0_VER)) {
		 /**
		 * if (release <  19.2.0 ) then programmer firmware equal 2K for both 1002/1003,
		 * 	how to detect {NMIS,small size < 10K} move 2K then read {NMIS,huge size >10K} then it valid
		 */
		offset += SFMV0_PROGRAM_SZ; /*switch it to old map 0*/
	}
	offset += FLASH_INFO_VECTOR_OFFSET;

	m2m_memcpy(pstrCurentFirmware->BuildDate,"Unknown",sizeof("Unknown"));
	m2m_memcpy(pstrCurentFirmware->BuildTime,"Unknown",sizeof("Unknown"));
	ret = programmer_read((uint8*)&strinfo, offset, sizeof(tstrInfoVector));
	if(strinfo.MagicNumber == INFO_VECTOR_MAGIC)
	{
		if(pstrCurentFirmware != NULL)
		{
			m2m_memcpy(pstrCurentFirmware->BuildDate,strinfo.Builddate,sizeof(__DATE__));
			m2m_memcpy(pstrCurentFirmware->BuildTime,strinfo.Buildtime,sizeof(__TIME__));
			pstrCurentFirmware->u16FirmwareSvnNum = strinfo.SvnNumber;
		}
	}
	/*
		Get the ota firmware version
	*/
	offset = strOtaControlSec.u32OtaRollbackImageOffset;
	if(M2M_GET_FW_VER(strOtaControlSec.u32OtaRollbackImagFirmwareVer) >= M2M_GET_FW_VER(REL_19_4_0_VER))
	{
		uint32 u32DlSize;
	    /* if (release >= 19.4.0 ) then programmer firmware equal actual size (parse it) for 1003A0/1002B0,
		/** how to detect {NMIS, small size < 10K} then move actual size then read {NMID, huge size > 10K}*/
		ret = programmer_read((uint8*)&u32DlSize, offset + 4 /*NMIS size*/, sizeof(uint32));
		offset += u32DlSize;
	} else if((M2M_GET_FW_VER(strOtaControlSec.u32OtaRollbackImagFirmwareVer) >= M2M_GET_FW_VER(REL_19_2_0_VER))&&
		(M2M_GET_FW_VER(strOtaControlSec.u32OtaRollbackImagFirmwareVer) < M2M_GET_FW_VER(REL_19_4_0_VER))) {
		/** if (release >= 19.2.0 ) && (release < 19.4.0) then programmer firmware equal (zero for 1003A0) /(2K for 1002B0),
		how to detect {NMIS,Huge size > 10K}*/
		if(REV(u32ChipId) != REV_3A0){
			offset += SFMV0_PROGRAM_SZ; /*TODO: switch it to old map 0*/
		}
	} else if(M2M_GET_FW_VER(strOtaControlSec.u32OtaRollbackImagFirmwareVer) < M2M_GET_FW_VER(REL_19_2_0_VER)) {
		 /**
		 * if (release <  19.2.0 ) then programmer firmware equal 2K for both 1002/1003,
		 * 	how to detect {NMIS,small size < 10K} move 2K then read {NMIS,huge size >10K} then it valid
		 */
		offset += SFMV0_PROGRAM_SZ; /*switch it to old map 0*/
	}
	offset += FLASH_INFO_VECTOR_OFFSET;
	m2m_memcpy(pstrOtaFirmware->BuildDate,"Unknown",sizeof("Unknown"));
	m2m_memcpy(pstrOtaFirmware->BuildTime,"Unknown",sizeof("Unknown"));
	ret = programmer_read((uint8*)&strinfo, offset, sizeof(tstrInfoVector));
	if(strinfo.MagicNumber == INFO_VECTOR_MAGIC)
	{
		if(pstrCurentFirmware != NULL)
		{
			m2m_memcpy(pstrOtaFirmware->BuildDate,strinfo.Builddate,sizeof(__DATE__));
			m2m_memcpy(pstrOtaFirmware->BuildTime,strinfo.Buildtime,sizeof(__TIME__));
			pstrOtaFirmware->u16FirmwareSvnNum = strinfo.SvnNumber;
		}
	}
ERR:
	return ret;
}


sint8 get_firmwarefile_version(char * file, tstrM2mRev *pstrm2mrev)
{
	FILE *fp;
	sint8 ret = M2M_SUCCESS;
	if(file != NULL)
	{
		fp = fopen(file, "rb");

		if(fp)
		{
			uint32 sz = 0;
			uint32 u32DlSize = 0;
			uint32 offset = M2M_FIRMWARE_FLASH_OFFSET;
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			fseek(fp, offset, SEEK_SET);
			/**
			 * if (release >= 19.4.0 ) then programmer firmware equal actual size (parse it),
			 * how to detect {NMIS, small size < 10K} then move actual size then read {NMID, huge size > 10K}
			 */
			fseek(fp,4,SEEK_CUR); //NMIS
			fread(&u32DlSize,1,4,fp); //Size
			offset += u32DlSize + FLASH_INFO_VECTOR_OFFSET;
			fseek(fp,offset,SEEK_SET);
			if(sz > sizeof(tstrInfoVector))
			{
				tstrInfoVector strinfo;
				fread(&strinfo,1,sizeof(tstrInfoVector),fp);
				if(strinfo.MagicNumber == INFO_VECTOR_MAGIC)
				{
					strfirmwareFileRev.u8DriverMajor    = M2M_GET_DRV_MAJOR(strinfo.VersionInfo);
					strfirmwareFileRev.u8DriverMinor    = M2M_GET_DRV_MINOR(strinfo.VersionInfo);
					strfirmwareFileRev.u8DriverPatch    = M2M_GET_DRV_PATCH(strinfo.VersionInfo);
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
	}
	else
	{
		ret = M2M_ERR_FAIL;
		NM_BSP_PRINTF("failed to open firmware file \n");
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
tstrGain_settingValues	gtstrGainSettingValue;

sint8 calc_gain_values(void)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 0;
	while(loopCntr < TX_GAIN_NUM_OF_REGISTERS)
	{
		au32RegAddr[loopCntr] = TX_GAIN_REG_BASE_ADDRESS+(loopCntr*4);
		loopCntr++;
	}
	gtstrGainSettingValue.is_valid = TX_GAIN_VALIDATION_NUMBER;
	gtstrGainSettingValue.reg_ver = TX_GAIN_VER_NUMBER;
	gtstrGainSettingValue.num_of_slots = TX_GAIN_NUM_OF_REGISTERS;
	loopCntr=0;
	gtstrGainSettingValue.reg_40 = 0x019701C9;
	gtstrGainSettingValue.reg_44 = 0x00000143;
	gtstrGainSettingValue.reg_48 = 0x00000000;
	gtstrGainSettingValue.reg_4C = 0x00000000;
	gtstrGainSettingValue.reg_50 = 0x11110000;
	gtstrGainSettingValue.reg_54 = 0x22221111;
	gtstrGainSettingValue.reg_58 = 0x00002222;
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
sint8 update_look_up_table(void)
{
	sint8 ret = M2M_SUCCESS;
	const double xo_input = 26.0;

	uint32 val32;
	uint32 magic[2];
	uint32 xo_offset_magic;
	uint32 chipid;
	EFUSEProdStruct efuse_struct;
	uint8 skip_bank_check;
	uint8 data[M2M_CONFIG_SECT_TOTAL_SZ];


	chipid = GET_CHIPID();
	if(REV(chipid) >= REV_3A0) {
		skip_bank_check = 1;
	} else {
		skip_bank_check = 0;
	}
	if(read_efuse_struct(&efuse_struct, skip_bank_check) < 0) {
		memset(&efuse_struct, 0, sizeof(efuse_struct));
	}

	if(efuse_struct.FreqOffset_used) { /* Get xo_offset magic */
		xo_offset_magic = efuse_struct.FreqOffset;
	} else {
		xo_offset_magic = 0;
	}

	programmer_read((uint8*)&data, M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ);
	m2m_memcpy((uint8*)&magic[0], (uint8*)&data[0], sizeof(magic));

	if( (magic[0] != PLL_MAGIC_NUMBER) || (magic[1] != xo_offset_magic)) {
		uint8 ch;

		for(ch = 0; ch < NUM_CHANNELS; ch++) {
			double xo;
			double xo_offset;
			double xo_to_VCO;
			uint32 n2, f, m, g;
			double lo, lo_actual;
			double rffrequency;
			double n1,dec,inv;
			double gMoG;

			if (ch < 13) {
				rffrequency = 2412.0 + (ch*5);
			} else {
				rffrequency = 2484.0;
			}

			if(efuse_struct.FreqOffset_used) {
				sint32 tmp; /* Must be SIGNED to handle negative values */
				tmp = (efuse_struct.FreqOffset > (1<<14)) ? efuse_struct.FreqOffset-(1<<15): efuse_struct.FreqOffset;
				xo_offset = ((double)tmp) / (1 << 6);
			} else {
				xo_offset = 0.0;
			}

			if(ch == 0) { /* print once */
				M2M_PRINT("Creating look up table for PLL with xo_offset = %3.4f.\n", xo_offset);
			}

			xo = xo_input*(1 + (xo_offset/1000000.0));

			lo = rffrequency *  2;

			if(REV(chipid) < REV_B0) {
				val32 = 0x4008f600;//nm_read_reg(WIFI_PLL_INTERNAL_3);
			} else {
				val32 = 0x4008fa0b;//nm_read_reg(WIFI_PLL_INTERNAL_3);
			}

			if (((val32 >> 8)&0x3)==0x2) {
				xo_to_VCO = 2* xo;
			} else {
				xo_to_VCO = xo;
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

#if 0
			M2M_PRINT("ch %02d pll1  = %x\n", ch, strChnParm[ch].u32PllInternal1);
			M2M_PRINT("ch %02d pll4  = %x\n", ch, strChnParm[ch].u32PllInternal4);
			M2M_PRINT("ch %02d wlxr1 = %x\n", ch, strChnParm[ch].WlanRx1);
			M2M_PRINT("ch %02d wlxr2 = %x\n", ch, strChnParm[ch].WlanRx2);
			M2M_PRINT("ch %02d wlxr3 = %x\n", ch, strChnParm[ch].WlanRx3);
			M2M_PRINT("ch %02d wlxr1 = %x\n", ch, strChnParm[ch].WlanTx1);
			M2M_PRINT("ch %02d wlxr2 = %x\n", ch, strChnParm[ch].WlanTx2);
			M2M_PRINT("ch %02d wlxr3 = %x\n", ch, strChnParm[ch].WlanTx3);
#endif
		}

		if((sizeof(strChnParm)+sizeof(magic))> M2M_PLL_FLASH_SZ) {
			M2M_PRINT("Excced pll sector size\n");
			goto ERR;
		}

		ret = programmer_erase(M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ, NULL);
		if(ret != M2M_SUCCESS) goto ERR;

		magic[0] = PLL_MAGIC_NUMBER;
		magic[1] = xo_offset_magic;
		m2m_memcpy(&data[0], (uint8*)&magic, sizeof(magic));
		m2m_memcpy(&data[sizeof(magic)], (uint8*)&strChnParm, sizeof(strChnParm));

		ret = programmer_write((uint8*)&data[0], M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ, NULL);
		if(ret!=M2M_SUCCESS) goto ERR;

		memset(magic, 0, sizeof(magic));
		programmer_read((uint8*)&magic[0], M2M_PLL_FLASH_OFFSET, sizeof(magic));
		if(magic[0] == PLL_MAGIC_NUMBER) {
			M2M_PRINT("done\n");
		} else {
			M2M_PRINT("Verify PLL failed. Invalid magic %u\n", magic[0]);
			//goto ERR;
		}
	}
ERR:
	return ret;
}
#endif
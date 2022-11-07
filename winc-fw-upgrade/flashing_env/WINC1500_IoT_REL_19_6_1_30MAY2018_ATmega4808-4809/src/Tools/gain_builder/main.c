/**
*  @file		main.c
*  @brief		This module contains M2M driver test application
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0
*/
#include "windows.h"
#include "conio.h"
#include "process.h"
#include <math.h>
#ifdef __GNUC__
#include <limits.h>
#endif
#include "firmware_addresses.h"
#include "programmer.h"
#include "common_values.h"
#include "driver/include/m2m_svnrev.h"
#include <assert.h>
//#define ENABLE_VERIFICATION
#define SKIP_BINARY_HEADER 1
/**
* CMD MACROS
*/
/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE
*/
#define NO_WAIT_BIT			(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define BREAK_BIT		    (0x08)	/*!< Positiion of break bit in the byte. */
/**
* SET VALUES DEPEND ON COMMAND LINE ARGUMENTS
*/
#define SET_NO_WAITE_BIT(x) (x |= NO_WAIT_BIT)	/*!< set no_wait bit in the byte to 1. */
#define SET_BREAK_BIT(x)    (x |= BREAK_BIT)
/**
* CHECK FOR VALUES FOR EACH CMD IN COMMAND LINE ARGUMENTS
*/
#define NO_WAIT(x)			(x & NO_WAIT_BIT)	/*!< It will return 1 if no_wait argument had been passed to main */
#define BREAK(x)		(x & BREAK_BIT)



#define MAX_NUMBER_OF_DB_ELEMENTS   8
#define MAX_NUMBER_OF_RATES         20
#define MAX_PA_GAIN					2
#define MAX_NUMBER_OF_CHANNELS      14

#define 	HIGH	 (1)
#define 	LP_VBAT1 (2)
#define		LP_VBAT2 (3)
#define 	LP_VBAT3 (4)

typedef struct
{
	char  *image_path;
	uint8 u8pwr;
	sint8 s8ValidEntry;
}entry;

entry gentry[4];

uint32	gau32RegAddr[TX_GAIN_NUM_OF_REGISTERS];
tstrGain_Struct	gatstrGainStruct;
static uint8 skipProgramming = 0;

#ifdef NMC1003A0_HP_MULT_GAIN
#define HP_DEFAULT_INVALID_GAIN_IDX 9999
static uint32 gu32GTbIndx = HP_DEFAULT_INVALID_GAIN_IDX;
static uint32 gu32NoOfTables = 0;
#endif

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
	M2M_PRINT("*   >TX Gain Builder for WINCxxxx <        *\n\r");
	M2M_PRINT("*      Owner:  Atmel Corporation           *\n\r");
	M2M_PRINT("********************************************\n\r");
	M2M_PRINT("SVN REV %u SVN BR %s \n\r",SVN_REVISION,SVN_REL_URL);
	M2M_PRINT("Built at %s\t%s\n", __DATE__, __TIME__);

}

enum{
	INDEX_TABLE_DB_COL	= 0,
	INDEX_TABLE_VALUE_COL
};

/*Like Step 1*/
double	gas16Db_table[MAX_NUMBER_OF_CHANNELS][MAX_NUMBER_OF_DB_ELEMENTS][2] = {0};
/*Like Step 2*/
enum{
	RATES_TABLE_GAIN_IN_DB_COL	= 0,
	RATES_TABLE_GAIN_INDEX_COL
};
double	gf8InputTableGains[MAX_NUMBER_OF_CHANNELS][MAX_NUMBER_OF_RATES][2] = {0};
char	gau8InputTableRates[MAX_NUMBER_OF_RATES + MAX_PA_GAIN][5] = {
	"1",
	"2",
	"5.5",
	"11",
	"6",
	"9",
	"12",
	"18",
	"24",
	"36",
	"48",
	"54",
	"mcs0",
	"mcs1",
	"mcs2",
	"mcs3",
	"mcs4",
	"mcs5",
	"mcs6",
	"mcs7",
	"1e9c",
	"1edc",
};
char	gau8Channels[MAX_NUMBER_OF_CHANNELS+1][3] = {
	"ch",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14"
};

float regPA[2];
uint8 gu8Pwr = 0;
uint32 chipId = 0;

static sint8 spitBinary(char* binaryFileName,uint8 *pu8Buf, uint32 u32Sz)
{
	sint8 ret = M2M_ERR_FAIL;
	FILE *fpTemp;
	char tempName[256] = OTA_BINARIES_DIRECTORY;
	if(chipId == 0x1503a0){
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
		if(binaryHeader != NULL){
			fwrite(binaryHeader->binVerInfo,sizeof(tstrM2mRev),1,fpTemp);
			fwrite(&binaryHeader->flashOffset,sizeof(uint32),1,fpTemp);
			fwrite(&binaryHeader->payloadSize,sizeof(uint32),1,fpTemp);
		}
#endif /*SKIP_BINARY_HEADER*/
		fwrite(pu8Buf,u32Sz, 1, fpTemp);
		fclose(fpTemp);
		ret = M2M_SUCCESS;
	}
	return ret;
}

static sint8 validate_data(const uint8 * pu8Data)
{
	sint8 s8Ret = M2M_SUCCESS;
	uint8 *pu8DupData = _strdup((const uint8 *)pu8Data);
	uint8 *pu8DupData2 = _strdup((const uint8*) pu8Data);
	uint8 *pu8StrTok  = strtok((uint8 *)pu8Data, ",");
	sint32 s32NCommaPerRow = 0;
	sint32 s32ind = 0;
	uint8 u8LoopCntr = 0;
	uint8 *pf = NULL;
	/* Check first Row:
	 * Ch	1	2	3	4	5	6	7	8	9	10	11	12	13	14
	 */
	if(memcmp((const uint8 *)pu8StrTok, (const uint8 *)&gau8Channels[0], strlen((const uint8 *)pu8StrTok))) {
		s8Ret = M2M_ERR_FAIL;
		goto _EXIT;
	}
	do
	{
		pu8StrTok  = strtok(NULL, ",\n");
		if(memcmp((const uint8 *)pu8StrTok, gau8Channels[u8LoopCntr+1], strlen(pu8StrTok))) {
			break;
		}
		if(MAX_NUMBER_OF_CHANNELS == u8LoopCntr){
			break;
		}
	}while((NULL != pu8StrTok) && ++u8LoopCntr);

	if(MAX_NUMBER_OF_CHANNELS != u8LoopCntr) {
		s8Ret = M2M_ERR_FAIL;
		goto _EXIT;
	}

	/* Check first Column:
	 * 1	2	5.5	11
	 * 6	9	12	18	24	36	48	54
	 * mcs0	mcs1	mcs2	mcs3	mcs4	mcs5	mcs6	mcs7
	 */
	pu8StrTok  = strtok(pu8DupData, "\n");
	u8LoopCntr = 0;
	do
	{
		pu8StrTok  = strtok(NULL, "\n");
		if(pu8StrTok == NULL)
		{
			s8Ret = M2M_ERR_FAIL;
			goto _EXIT;
		}
		pf = strstr(pu8StrTok, ",");
		*pf = 0;
		if(memcmp(pu8StrTok, &gau8InputTableRates[u8LoopCntr], strlen(pu8StrTok))) {
			s8Ret = M2M_ERR_FAIL;
			goto _EXIT;
		}
		u8LoopCntr++;
#if 1
		if((MAX_NUMBER_OF_RATES + MAX_PA_GAIN) <= u8LoopCntr) {
			break;
		}
#endif
	}while((NULL != pu8StrTok) && (NULL != pf));
#if 1
	/*TODO: Need to modify condition when merging to release trunk */
	if(MAX_NUMBER_OF_RATES + MAX_PA_GAIN != u8LoopCntr) {
		M2M_PRINT("Incomplete table (%d)\n",u8LoopCntr);
		s8Ret = M2M_ERR_FAIL;
		goto _EXIT;
	}
#endif


	while(pu8DupData2[s32ind] != '\0')
	{
		if(pu8DupData2[s32ind] == ',')
		{
			s32NCommaPerRow++;
		}
		else if(pu8DupData2[s32ind] == '\n')
		{
			if(s32NCommaPerRow != 0 && MAX_NUMBER_OF_CHANNELS != s32NCommaPerRow)
			{
				s32NCommaPerRow = M2M_ERR_FAIL;
				break;
			}
			else
			{
				s32NCommaPerRow = 0;
			}
		}
		s32ind++;
	}

	if(s32NCommaPerRow == M2M_ERR_FAIL)
	{
		M2M_PRINT("Incomplete gain values.\n");
		s8Ret = M2M_ERR_FAIL;
	}

_EXIT:
	free(pu8DupData2);
	free(pu8DupData);
	return s8Ret;
}
#ifdef NMC1003A0_HP_MULT_GAIN
static void init_gain_struct(tstrGain_Struct *ptstrGainSettingValue, uint8 u8pwr, uint8 u8table_idx)
#else
static void init_gain_struct(tstrGain_Struct *ptstrGainSettingValue,uint8 u8pwr)
#endif
{
	uint32 *ptr = NULL;
	uint8 u8LoopCntr = 0, u8ChannelsLoop = 0;;


	while(u8LoopCntr < TX_GAIN_NUM_OF_REGISTERS)
	{
		gau32RegAddr[u8LoopCntr] = TX_GAIN_REG_BASE_ADDRESS+(u8LoopCntr*4);
		u8LoopCntr++;
	}
#if 0
	for(u8ChannelsLoop = 0; u8ChannelsLoop<MAX_NUMBER_OF_CHANNELS; u8ChannelsLoop++) {
		ptstrGainSettingValue->is_valid = TX_GAIN_VALIDATION_NUMBER;
		ptstrGainSettingValue->num_of_slots = TX_GAIN_NUM_OF_REGISTERS;
		ptstrGainSettingValue->reg_ver = TX_GAIN_VER_NUMBER;
		ptstrGainSettingValue->channel_num = u8ChannelsLoop;
		ptstrGainSettingValue++;
	}
#else
#ifdef NMC1003A0_HP_MULT_GAIN
	ptstrGainSettingValue->GainHdr.magic = TX_MULT_GAIN_VALIDATION_NUMBER;
	if(u8pwr == HIGH)
	{
		ptstrGainSettingValue->GainHdr.HpGainTbValid[u8table_idx] = TX_MULT_GAIN_VALIDATION_NUMBER;
	}
#else
	ptstrGainSettingValue->GainHdr.magic = TX_GAIN_VALIDATION_NUMBER;
	if(u8pwr == HIGH)
	{
		ptstrGainSettingValue->GainHdr.HpGainValid = TX_GAIN_VALIDATION_NUMBER;
	}
	if(u8pwr == LP_VBAT1)
	{
		ptstrGainSettingValue->GainHdr.LpDGainValidVbat[0] = TX_GAIN_VALIDATION_NUMBER;
		//ptstrGainSettingValue->GainHdr.LpPAGainValidVbat[0] = TX_GAIN_VALIDATION_NUMBER;

	}
	if(u8pwr == LP_VBAT2)
	{
		ptstrGainSettingValue->GainHdr.LpDGainValidVbat[1] = TX_GAIN_VALIDATION_NUMBER;
		//ptstrGainSettingValue->GainHdr.LpPAGainValidVbat[1] = TX_GAIN_VALIDATION_NUMBER;
	}
	if(u8pwr == LP_VBAT3)
	{
		ptstrGainSettingValue->GainHdr.LpDGainValidVbat[2] = TX_GAIN_VALIDATION_NUMBER;
		//ptstrGainSettingValue->GainHdr.LpPAGainValidVbat[2] = TX_GAIN_VALIDATION_NUMBER;
	}
#endif // NMC1003A0_HP_MULT_GAIN
#endif

	/*Init with default values*/
	for(u8ChannelsLoop=0; u8ChannelsLoop<MAX_NUMBER_OF_CHANNELS; u8ChannelsLoop++)
	{
		for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++)
		{
			gas16Db_table[u8ChannelsLoop][u8LoopCntr][INDEX_TABLE_DB_COL] = -100;
			gas16Db_table[u8ChannelsLoop][u8LoopCntr][INDEX_TABLE_VALUE_COL] = 0;
		}
	}
//#define PRINT_INIT_LOGS
#ifdef	PRINT_INIT_LOGS
	u8LoopCntr = 0;
	u8ChannelsLoop = 0;
	for(u8ChannelsLoop=MAX_NUMBER_OF_CHANNELS; u8ChannelsLoop>0; u8ChannelsLoop--)
	{
		M2M_PRINT(" ___________ _____\r\n");
		M2M_PRINT("|Register   | CH%02d|\r\n", u8ChannelsLoop);
		M2M_PRINT("|___________|_____|\r\n");
		ptr = (uint32 *)&ptstrGainSettingValue->reg_40;
		ptstrGainSettingValue--;
		while(u8LoopCntr < TX_GAIN_NUM_OF_REGISTERS)
		{
			M2M_PRINT("|0x%08X | 0x%02X|\n", gau32RegAddr[u8LoopCntr], *ptr);
			ptr++;
			u8LoopCntr++;
		}
		u8LoopCntr = 0;
	}
	M2M_PRINT("|___________|_____|\r\n");
#endif
}

static uint8 get_rate_index(uint8 * pu8Rate)
{
	uint8 u8LoopCntr;
	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++)
	{
		if(!memcmp(pu8Rate, gau8InputTableRates[u8LoopCntr], strlen(pu8Rate))) {
			break;
		} else {
			continue;
		}
	}
	return u8LoopCntr;
}
/*Get data from fill array with data like:*/
/*
Type	Rate	Dgain dB	Gain Index
B Mode	1		-7			0
		2		-7			0
		5.5		-7			0
		11		-7			0

G Mode	6		-8			1
		9		-8			1
		12		-8			1
		18		-8			1
		24		-8			1
		36		-8			1
		48		-8			1
		54		-8			1

N Mode	mcs0	-10			2
		mcs1	-10			2
		mcs2	-10			2
		mcs3	-10			2
		mcs4	-10			2
		mcs5	-10			2
		mcs6	-10			2
		mcs7	-10			2
*/
static sint8 parse_gains_rates_table(uint8 * pu8Data)
{
	sint8   s8Ret = M2M_SUCCESS;
	uint8	*pu8Ptr = NULL;
	uint8	u8Index = 0;
	uint8	u8LoopCntr = 0, u8LoopChannels = 0;
	if(pu8Data == NULL)
	{
		M2M_PRINT("NULL pointer\n");
		s8Ret = M2M_ERR_FAIL;
		goto _END;
	}
	printf(">Extracting data from file...\r\n");

	pu8Data--;
	for(u8LoopCntr=0; u8LoopCntr<(MAX_NUMBER_OF_RATES + MAX_PA_GAIN); u8LoopCntr++)
	{
		uint8	temp[5]={0};
		pu8Ptr = strstr(pu8Data, ",");
		if(pu8Ptr == NULL)
		{
			M2M_PRINT("NULL Pointer\n");
			s8Ret = M2M_ERR_FAIL;
			goto _END;
		}
		pu8Data++;
		memcpy(temp, pu8Data, pu8Ptr-pu8Data);
		u8Index = get_rate_index(temp);
		if(u8Index >= MAX_NUMBER_OF_RATES)
		{
			float s8Value = (float)strtod(++pu8Ptr, &pu8Data);
			regPA[0] = s8Value;
			pu8Ptr = strstr(pu8Ptr, "\n");
			pu8Ptr = strstr(pu8Ptr, ",");
			pu8Ptr++;
			s8Value = (float)strtod(pu8Ptr, &pu8Data);
			regPA[1] = s8Value;
			goto _END;
			/*Out of index, Not valid entry*/
		}
		for(u8LoopChannels=0; u8LoopChannels<MAX_NUMBER_OF_CHANNELS; u8LoopChannels++)
		{
			double s8Value = strtod(++pu8Ptr, &pu8Data);
			if(s8Value == 0.0)
			{
				M2M_PRINT("[ERR]: Not allawed value.\n");
				s8Ret = M2M_ERR_FAIL;
				goto _END;
			}
			//double s8Value = (sint8)strtol(++pu8Ptr, &pu8Data, 10);
			gf8InputTableGains[u8LoopChannels][u8Index][RATES_TABLE_GAIN_IN_DB_COL] = s8Value;
			pu8Ptr = strstr(pu8Ptr, ",");
			if(pu8Ptr == NULL)
			{
				M2M_PRINT("NULL Pointer\n");
				s8Ret = M2M_ERR_FAIL;
				goto _END;
			}
		}
	}
//#define TEST
#ifdef TEST
	for(u8LoopChannels=0; u8LoopChannels<MAX_NUMBER_OF_CHANNELS; u8LoopChannels++)
	{
		printf("CH: %d\r\n",u8LoopChannels+1);
		for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++)
		{
			printf("%f %04d\r\n",gf8InputTableGains[u8LoopChannels][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL],gf8InputTableGains[u8LoopChannels][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL]);
		}
		printf("------------\r\n");
	}
#endif
_END:
	printf("Done\r\n");
	return s8Ret;
}

double calc_gain_value(double s8InValue)
{
	double dValue;
	double dBase = (double)(s8InValue/(float)20.0);
	dValue = pow(10, dBase);
	dValue *= 1024;
	dValue = floor(dValue);
	return dValue;
}

uint8 is_value_in_array(uint8 u8Channel, double s8Val)
{
	uint8 u8LoopCntr;
	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++)
	{
		if(gas16Db_table[u8Channel][u8LoopCntr][INDEX_TABLE_DB_COL] == s8Val)
			break;
		else
			continue;
	}
	return u8LoopCntr;
}

/*To be like*/
/*
dB		Index	Decimal	Hex
-7		0		457		1C9
-8		1		407		197
-10		2		323		143
-100	3		0		0
-100	4		0		0
-100	5		0		0
-100	6		0		0
-100	7		0		0
*/
/* Function name "round" clashes with function defined in math.h when building in VS2015 */
long our_round(double x) {
	assert(x >= LONG_MIN-0.5);
    assert(x <= LONG_MAX+0.5);
    if (x >= 0)
		return (long) (x+0.5);
	return (long) (x-0.5);
}
void update_db_table(void)
{
	uint8 u8LoopCntr/*Table Counter*/, loopCntr2/*Channel Counter*/;
	uint8 indexOfRates=0, indexOfDB=0, index=MAX_NUMBER_OF_DB_ELEMENTS;

	printf(">Building tables...\r\n");

	for(loopCntr2=0; loopCntr2<MAX_NUMBER_OF_CHANNELS; loopCntr2++)
	{
		for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++)
		{
			index = is_value_in_array(loopCntr2, gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL]);
			if(index >= MAX_NUMBER_OF_DB_ELEMENTS)//Not Found
			{
				gas16Db_table[loopCntr2][indexOfDB][INDEX_TABLE_DB_COL] = gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL];
				gas16Db_table[loopCntr2][indexOfDB][INDEX_TABLE_VALUE_COL] = calc_gain_value(gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL]);
				gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL] = indexOfDB;
				indexOfDB++;
			}
			else//Found
			{
				gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL] = index;
			}
		}
		indexOfRates=0;
		indexOfDB=0;
//#define TEST
#ifdef TEST
		printf("CH %d:\r\n",loopCntr2);
		for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++)
		{
			gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL] = our_round(gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL]);
			printf("%f %x\r\n",gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_DB_COL],(uint32)gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL]);
		}
		printf("-------------------\r\n");
		for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++)
		{
			printf("%f %04d\r\n",gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL],gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL]);
		}
		printf("-------------------\r\n");
#endif
	}

	printf("Done\r\n");
	return;
}
#ifdef NMC1003A0_HP_MULT_GAIN
void print_tables(uint8 u8pwr, uint8 u8table_idx)
#else
void print_tables(uint8 u8pwr)
#endif
{
//#define PRINT1	//Prtint format of table 2 (Step2 in Excel sheet)
//#define PRINT2	//Prtint format of table 1 (Step1 in Excel sheet)
#define PRINT3	//Prtint format of table 3 (Step3 in Excel sheet)
#if defined(PRINT1) || defined(PRINT2) || defined(PRINT3)
	uint32 *ptr = NULL;
	uint8 u8LoopCntr, loopCntr2;
#endif
#ifdef PRINT1
	printf(" _________ _________ _________ _________ _________ _________ _________\r\n");
	printf("|Chann 01 |Chann 02 |Chann 03 |Chann 04 |Chann 05 |Chann 06 |Chann 07 |\r\n");
	printf("|Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |\r\n");
	printf("|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|\r\n");

	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++){
		for(loopCntr2=0; loopCntr2<7; loopCntr2++) {
			printf("|%f | %f ", gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL],
				gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL]);
		}
		printf("|\n");
	}
	printf(" _________ _________ _________ _________ _________ _________ _________\r\n");
	printf("|Chann 08 |Chann 09 |Chann 10 |Chann 11 |Chann 12 |Chann 13 |Chann 14 |\r\n");
	printf("|Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |Dgain|Id |\r\n");
	printf("|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|\r\n");

	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_RATES; u8LoopCntr++){
		for(loopCntr2=7; loopCntr2<MAX_NUMBER_OF_CHANNELS; loopCntr2++) {
			printf("|%4d | %1d ", gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_IN_DB_COL],
				gf8InputTableGains[loopCntr2][u8LoopCntr][RATES_TABLE_GAIN_INDEX_COL]);
		}
		printf("|\n");
	}
	printf("|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|_____|___|\r\n");
#endif
#ifdef PRINT2
	printf("\r\n\r\n");
	printf(" _____________ _____________ _____________ _____________ _____________\r\n");
	printf("| Channel 01  | Channel 02  | Channel 03  | Channel 04  | Channel 05  |\r\n");
	printf("|dB   |Id|val |dB   |Id|val |dB   |Id|val |dB   |Id|val |dB   |Id|val |\r\n");
	printf("|_____|__|____|_____|__|____|_____|__|____|_____|__|____|_____|__|____|\r\n");
	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++){
		for(loopCntr2=0; loopCntr2<5; loopCntr2++) {
			printf("|%4d | %1d|%04X", gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_DB_COL],
				u8LoopCntr, gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL]);
		}
		printf("|\n");
	}
	printf(" _____________ _____________ _____________ _____________ _____________\r\n");
	printf("| Channel 06  | Channel 07  | Channel 08  | Channel 09  | Channel 10  |\r\n");
	printf("|dB   |Id|val |dB   |Id|val |dB   |Id|val |dB   |Id|val |dB   |Id|val |\r\n");
	printf("|_____|__|____|_____|__|____|_____|__|____|_____|__|____|_____|__|____|\r\n");
	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++){
		for(loopCntr2=5; loopCntr2<10; loopCntr2++) {
			printf("|%4d | %1d|%04X", gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_DB_COL],
				u8LoopCntr, gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL]);
		}
		printf("|\n");
	}
	printf(" _____________ _____________ _____________ _____________ \r\n");
	printf("| Channel 11  | Channel 12  | Channel 13  | Channel 14  |\r\n");
	printf("|dB   |Id|val |dB   |Id|val |dB   |Id|val |dB   |Id|val |\r\n");
	printf("|_____|__|____|_____|__|____|_____|__|____|_____|__|____|\r\n");
	for(u8LoopCntr=0; u8LoopCntr<MAX_NUMBER_OF_DB_ELEMENTS; u8LoopCntr++){
		for(loopCntr2=10; loopCntr2<MAX_NUMBER_OF_CHANNELS; loopCntr2++) {
			printf("|%4d | %1d|%04X", gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_DB_COL],
				u8LoopCntr, gas16Db_table[loopCntr2][u8LoopCntr][INDEX_TABLE_VALUE_COL]);
		}
		printf("|\n");
	}
	printf("|_____|__|____|_____|__|____|_____|__|____|_____|__|____|\r\n");
#endif
#ifdef PRINT3
#ifdef NMC1003A0_HP_MULT_GAIN
	printf(" ____________________________________________________________________________\n");
	printf("|                               Table - %d                                    |\n", u8table_idx+1);
#endif
	printf(" ______ _________ _________ _________ _________ _________ _________ _________\r\n");
	printf("|CH/REG|");
	for(loopCntr2=0;loopCntr2<TX_GAIN_NUM_OF_REGISTERS; loopCntr2++)
	{
		printf("%08X |",gau32RegAddr[loopCntr2]);
	}
	printf("\r\n");
	printf("|______|_________|_________|_________|_________|_________|_________|_________|\r\n");

	for(loopCntr2=0; loopCntr2<MAX_NUMBER_OF_CHANNELS; loopCntr2++)
	{
		printf("| %02d   |", loopCntr2+1);
#ifdef NMC1003A0_HP_MULT_GAIN
		if (u8pwr == HIGH)
		{
			ptr =(uint32 *)(&gatstrGainStruct.HpGain[u8table_idx].strHpGain[loopCntr2].reg_40);
		}
#else
		if(u8pwr == HIGH)
		{
			ptr =(uint32 *)(&gatstrGainStruct.strHpGain[loopCntr2].reg_40);
		}
		else if(u8pwr == LP_VBAT1)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[0].strlPGain[loopCntr2].reg_40;
		}
		else if(u8pwr == LP_VBAT2)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[1].strlPGain[loopCntr2].reg_40;
		}
		else if(u8pwr == LP_VBAT3)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[2].strlPGain[loopCntr2].reg_40;
		}
#endif //NMC1003A0_HP_MULT_GAIN
		for(u8LoopCntr=0; u8LoopCntr<TX_GAIN_NUM_OF_REGISTERS; u8LoopCntr++)
		{
			printf("%08X |", *ptr);
			ptr++;
		}
		printf("\n");
	}
	printf("|______|_________|_________|_________|_________|_________|_________|_________|\r\n");
#endif
	return;
}

/*Registers values like:*/
/*
Register 0x1240	019701C9
Register 0x1244	00000143
Register 0x1248	00000000
Register 0x124c	00000000

Register 0x1250	11110000
Register 0x1254	22221111
Register 0x1258	00002222
*/
#ifdef NMC1003A0_HP_MULT_GAIN
void calculate_registers(uint8 u8pwr, uint8 u8table_idx)
#else
void calculate_registers(uint8 u8pwr)
#endif
{
	uint8 u8LoopCntr, loopCntr2;
	uint32 *ptr = NULL;
	for(loopCntr2=0; loopCntr2<MAX_NUMBER_OF_CHANNELS; loopCntr2++)
	{
#ifdef NMC1003A0_HP_MULT_GAIN
		if (u8pwr == HIGH)
		{
			ptr =(uint32 *)&gatstrGainStruct.HpGain[u8table_idx].strHpGain[loopCntr2].reg_40;
		}
#else
		if(u8pwr == HIGH)
		{
			ptr =(uint32 *)&gatstrGainStruct.strHpGain[loopCntr2].reg_40;
		}
		if(u8pwr == LP_VBAT1)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[0].strlPGain[loopCntr2].reg_40;
		}
		if(u8pwr == LP_VBAT2)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[1].strlPGain[loopCntr2].reg_40;
		}
		if(u8pwr == LP_VBAT3)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[2].strlPGain[loopCntr2].reg_40;
		}
#endif
		for(u8LoopCntr=0; u8LoopCntr<(MAX_NUMBER_OF_DB_ELEMENTS/2); u8LoopCntr++)
		{
			*ptr = (uint32)(gas16Db_table[loopCntr2][u8LoopCntr*2][INDEX_TABLE_VALUE_COL]+
				(gas16Db_table[loopCntr2][(u8LoopCntr*2)+1][INDEX_TABLE_VALUE_COL] * pow(2,16)));
			ptr++;
		}
#ifdef NMC1003A0_HP_MULT_GAIN
		if (u8pwr == HIGH)
		{
			ptr = (uint32 *)&gatstrGainStruct.HpGain[u8table_idx].strHpGain[loopCntr2].reg_50;
		}
#else
		if(u8pwr == HIGH)
		{
			ptr = (uint32 *)&gatstrGainStruct.strHpGain[loopCntr2].reg_50;
		}
		if(u8pwr == LP_VBAT1)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[0].strlPGain[loopCntr2].reg_50;
		}
		if(u8pwr == LP_VBAT2)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[1].strlPGain[loopCntr2].reg_50;
		}
		if(u8pwr == LP_VBAT3)
		{
			ptr =(uint32 *)&gatstrGainStruct.strLPGainVbat[2].strlPGain[loopCntr2].reg_50;
		}
#endif
		for(u8LoopCntr=4; u8LoopCntr<TX_GAIN_NUM_OF_REGISTERS; u8LoopCntr++)
		{
			uint8 u8LoopCntrIn, max_loop = 28;
			*ptr = 0;
			if(u8LoopCntr+1 == TX_GAIN_NUM_OF_REGISTERS)
			{
				max_loop = 12;
			}
			for(u8LoopCntrIn=0; u8LoopCntrIn <= max_loop; u8LoopCntrIn+=4)
			{
				*ptr += (uint32)
					(gf8InputTableGains[loopCntr2][((u8LoopCntr-4)*8)+(u8LoopCntrIn/4)][RATES_TABLE_GAIN_INDEX_COL] *
					pow(2,u8LoopCntrIn));
			}
			ptr++;
		}
	}
/*
	gatstrGainSettingValue.u32RegValue[4] = 0;
	for(u8LoopCntr=0; u8LoopCntr<=28; u8LoopCntr+=4)
	{
		gatstrGainSettingValue.u32RegValue[4] += (gf8InputTableGains[u8LoopCntr/4][RATES_TABLE_GAIN_INDEX_COL] * (uint32)pow(2,u8LoopCntr));
	}

	gatstrGainSettingValue.u32RegValue[5] = 0;
	for(u8LoopCntr=0; u8LoopCntr<=28; u8LoopCntr+=4)
	{
		gatstrGainSettingValue.u32RegValue[5] += (gf8InputTableGains[8+(u8LoopCntr/4)][RATES_TABLE_GAIN_INDEX_COL] * (uint32)pow(2,u8LoopCntr));
	}

	gatstrGainSettingValue.u32RegValue[6] = 0;
	for(u8LoopCntr=0; u8LoopCntr<=12; u8LoopCntr+=4)
	{
		gatstrGainSettingValue.u32RegValue[6] += (gf8InputTableGains[16+(u8LoopCntr/4)][RATES_TABLE_GAIN_INDEX_COL] * (uint32)pow(2,u8LoopCntr));
	}
*/
	return;
}

#ifdef NMC1003A0_HP_MULT_GAIN
sint8 update_gain_struct(tstrGain_Struct *ptstrGainSettingValue, uint8 *data, uint8 u8pwr, uint8 u8table_idx)
#else
sint8 update_gain_struct(tstrGain_Struct *ptstrGainSettingValue, uint8 *data,uint8 u8pwr)
#endif
{
	sint8 s8Ret =  M2M_SUCCESS;

	s8Ret = parse_gains_rates_table(data);
	if(M2M_SUCCESS == s8Ret)
	{

		update_db_table();
#ifdef NMC1003A0_HP_MULT_GAIN
		calculate_registers(u8pwr, u8table_idx);
		print_tables(u8pwr, u8table_idx);
#else
		calculate_registers(u8pwr);
		print_tables(u8pwr);
#endif

#if 0
		ptrReg = strstr(data, GAIN_REG_PREFIX);
		printf(" _________ ___________\r\n");
		printf("|Register | Value     |\r\n");
		printf("|_________|___________|\r\n");
		while(NULL != ptrReg)
		{
			ptrReg +=strlen(GAIN_REG_PREFIX);
			ptstrGainSettingValue->apRegAddr[index] = strtol(ptrReg, NULL, 16);
			printf("|0x%06X ",ptstrGainSettingValue->apRegAddr[index]);
			ptrReg = strstr(ptrReg, ",");
			ptrReg++;
			ptstrGainSettingValue->u32RegValue[index] = strtol(ptrReg, NULL, 16);
			printf("| 0x%08X|\n",ptstrGainSettingValue->u32RegValue[index]);
			ptrReg = strstr(ptrReg, GAIN_REG_PREFIX);
		}
		printf("|_________|___________|\r\n");
#endif
	}
	return s8Ret;
}
uint8	u8ImageData[M2M_CONFIG_SECT_TOTAL_SZ]={0};
#ifdef NMC1003A0_HP_MULT_GAIN
sint8 update_table(char* image_path, uint8 u8pwr, uint8 u8table_idx)
#else
sint8 update_table(char* image_path,uint8 u8pwr)
#endif
{
	sint8	s8Ret = M2M_SUCCESS;
	uint32	u32ImageSz = 0;
	FILE	*pf = NULL;

	uint8	*pu8DataDup = NULL;
	if(image_path == NULL)
	{
		s8Ret = M2M_ERR_FAIL;
		goto _END;
	}
	pf = fopen(image_path,"r");

	if(NULL == pf) {
		M2M_PRINT("[ERR]Unable to open file:\n\t\"%s\"\r\n",image_path);
		s8Ret = M2M_ERR_FAIL;
		return s8Ret;
	} else {
		/**/
	}
	M2M_PRINT("Setting file has been opened:\n\t\"%s\"\r\n",image_path);

	fseek(pf, 0L, SEEK_END);
	u32ImageSz = ftell(pf);
	fseek(pf, 0L, SEEK_SET);

	fread(u8ImageData, u32ImageSz, 1, pf);

	pu8DataDup = _strdup(u8ImageData);

	if(M2M_SUCCESS != validate_data((const uint8 *)u8ImageData)) {
		M2M_PRINT("Invalid data.\r\n");
		s8Ret = M2M_ERR_FAIL;
		goto _END;
	}

	M2M_PRINT("Initializing values ....\r\n");
#ifdef NMC1003A0_HP_MULT_GAIN
	init_gain_struct(&gatstrGainStruct, u8pwr, u8table_idx);
#else
	init_gain_struct(&gatstrGainStruct,u8pwr);
#endif
	pu8DataDup = strstr(pu8DataDup, "\n");
	pu8DataDup++;//to avoid first character which is "\n"
#ifdef NMC1003A0_HP_MULT_GAIN
	s8Ret = update_gain_struct(&gatstrGainStruct, pu8DataDup, u8pwr, u8table_idx);
#else
	s8Ret = update_gain_struct(&gatstrGainStruct, pu8DataDup,u8pwr);
#endif
_END:
	return s8Ret;
}

sint8 burn_gain_settings(uint8* vflash)
{
	sint8	s8Ret = M2M_SUCCESS;
	uint8 u8LoopCntr = 0;
	int i = 0;
	sint32 s32IndUpdateTable = 0;

	memset(&gatstrGainStruct, 0, sizeof(gatstrGainStruct));
#ifdef NMC1003A0_HP_MULT_GAIN
	// Update the active index in the gain structure
	gatstrGainStruct.GainHdr.HpActiveGTbIndx = gu32GTbIndx;
	M2M_PRINT("Active gain table index = %d\n", gatstrGainStruct.GainHdr.HpActiveGTbIndx);
#endif
	for (i = 0; i < 4; i++)
	{
		if(gentry[i].s8ValidEntry == TRUE)
		{
#ifdef NMC1003A0_HP_MULT_GAIN
			s8Ret = update_table(gentry[i].image_path, gentry[i].u8pwr, i);
#else
			s8Ret = update_table(gentry[i].image_path,gentry[i].u8pwr);
#endif
			if(M2M_SUCCESS != s8Ret)
			{
				goto _END;
			}
		}
	}

	memset(u8ImageData, 0xFF, sizeof(u8ImageData));

	if(!skipProgramming)
	{
		M2M_PRINT(">Reading data...\r\n");
		s8Ret = programmer_read(u8ImageData, M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ);
		if(M2M_SUCCESS != s8Ret)
		{
			M2M_PRINT("[ERR]Error\r\n");
			s8Ret = M2M_ERR_FAIL;
			goto _END;
		} else {
			/**/
		}
		M2M_PRINT("Done.\r\n");
	}

	memcpy((uint8 *)&u8ImageData[M2M_PLL_FLASH_SZ], (uint8 *)&gatstrGainStruct, sizeof(tstrGain_Struct));
	spitBinary("fs_gain_table",(uint8 *)&u8ImageData[M2M_PLL_FLASH_SZ],M2M_GAIN_FLASH_SZ);

	if(!skipProgramming)
	{
		s8Ret=programmer_erase(M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ, vflash);
		if(M2M_SUCCESS != s8Ret) {
			M2M_PRINT("[ERR]Error\r\n");
			s8Ret = M2M_ERR_FAIL;
			goto _END;
		} else {
			/**/
		}
		s8Ret=programmer_write(u8ImageData, M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ, vflash);
		if(M2M_SUCCESS != s8Ret) {
			M2M_PRINT("[ERR]Error\r\n");
			s8Ret = M2M_ERR_FAIL;
			goto _END;
		} else {
			/**/
		}
	#ifdef ENABLE_VERIFICATION
		M2M_PRINT(">Verifying...\r\n");
		{
			uint8	u8ImageDataVerify[FLASH_SECTOR_SZ]={0};
			if(M2M_SUCCESS != programmer_read(u8ImageDataVerify, M2M_PLL_FLASH_OFFSET, M2M_CONFIG_SECT_TOTAL_SZ))
			{
				M2M_PRINT("[ERR]Error while reading.\r\n");
				s8Ret = M2M_ERR_FAIL;
				goto _END;
			}

			for(i=0; i<FLASH_SECTOR_SZ; i++)
			{
				if(u8ImageDataVerify[i] != u8ImageData[i]){
					break;
				}
			}
			if(i<FLASH_SECTOR_SZ) {
				M2M_PRINT("[ERR]Error\r\n");
			} else {
				printf("Done\r\n");
			}
		}
	#endif
	}

_END:
	return s8Ret;
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
static sint8 checkArguments(char * argv[],uint8 argc, uint8* portNum,uint32* chipId, char ** vflash_path)
{
	sint8 ret = M2M_SUCCESS;
	uint8 u8LoopCntr = 1;
	uint32 i = 0;
	uint8 portHasValue = 0, assignNewPath = 0;
	*portNum = 0;

	if(1 >= argc)
	{
		ret = M2M_ERR_FAIL;
		goto ERR;
	}


	memset(gentry,0,4*sizeof(entry));

	for(;u8LoopCntr<argc;u8LoopCntr++)
	{
#ifdef NMC1003A0_HP_MULT_GAIN
		if (strstr(argv[u8LoopCntr], "-table"))
		{
			// Read the no of tables
			u8LoopCntr++;
			sscanf(argv[u8LoopCntr], "%d", &gu32NoOfTables);

			if (!(gu32NoOfTables >= 1 && gu32NoOfTables <= 4)) {
				ret = M2M_ERR_FAIL;
				goto ERR;
			}
			// Read the tables
			if(argc-u8LoopCntr > 1) {
				while (i < gu32NoOfTables) {
					u8LoopCntr++;
					//memset(image_path, 0, sizeof(image_path));
					gentry[i].image_path = argv[u8LoopCntr];
					gentry[i].u8pwr = HIGH;
					gentry[i].s8ValidEntry = TRUE;
					i++;
				}
				continue;
			}
		}
		if (strstr(argv[u8LoopCntr], "-index"))
		{
			u8LoopCntr++;
			sscanf(argv[u8LoopCntr], "%d", &gu32GTbIndx);
			if (!(gu32GTbIndx >= 1 && gu32GTbIndx <= 4)) {
				ret = M2M_ERR_FAIL;
				goto ERR;
			}
			continue;
		}

#else
		if(strstr(argv[u8LoopCntr],"-hp"))
		{
			if(argc-u8LoopCntr > 1) {
				u8LoopCntr++;
				//memset(image_path, 0, sizeof(image_path));
				gentry[i].image_path = argv[u8LoopCntr];
				gentry[i].u8pwr = HIGH;
				gentry[i].s8ValidEntry = TRUE;
				i++;
				continue;
			}
		}
		if(strstr(argv[u8LoopCntr],"-lpvbat1"))
		{
			if(argc-u8LoopCntr > 1) {
				u8LoopCntr++;
				//memset(image_path, 0, sizeof(image_path));
				gentry[i].image_path = argv[u8LoopCntr];
				gentry[i].u8pwr = LP_VBAT1;
				gentry[i].s8ValidEntry = TRUE;
				i++;
				continue;
			}
		}
		if(strstr(argv[u8LoopCntr],"-lpvbat2"))
		{
			if(argc-u8LoopCntr > 1) {
				u8LoopCntr++;
				//memset(image_path, 0, sizeof(image_path));
				gentry[i].image_path = argv[u8LoopCntr];
				gentry[i].u8pwr = LP_VBAT2;
				gentry[i].s8ValidEntry = TRUE;
				i++;
				continue;
			}
		}
		if(strstr(argv[u8LoopCntr],"-lpvbat3"))
		{
			if(argc-u8LoopCntr > 1) {
				u8LoopCntr++;
				//memset(image_path, 0, sizeof(image_path));
				gentry[i].image_path = argv[u8LoopCntr];
				gentry[i].u8pwr = LP_VBAT3;
				gentry[i].s8ValidEntry = TRUE;
				i++;
				continue;
			}
		}
#endif // NMC1003A0_HP_MULT_GAIN
		if(strstr(argv[u8LoopCntr],"-no_wait"))
		{
			SET_NO_WAITE_BIT(ret);
			continue;
		}
		if(strstr(argv[u8LoopCntr],"-B"))
		{
			SET_BREAK_BIT(ret);
			continue;
		}
		if(strstr(argv[u8LoopCntr],"-vflash_path"))
		{
			if(argc-u8LoopCntr > 1) {
				*vflash_path =  argv[++u8LoopCntr];
				M2M_PRINT("Virtual Flash Path %s\n",*vflash_path);
			}
			continue;
		}
		if(strstr(argv[u8LoopCntr],"-port"))
		{
			if(argc-u8LoopCntr > 1)
			{
				*portNum = atoi(argv[++u8LoopCntr]);
				portHasValue = 1;
			}
			continue;
		}
		if(strstr(argv[u8LoopCntr],"-skip_programming"))
		{
			skipProgramming = 1;
			continue;
		}
		if(strstr(argv[u8LoopCntr],"-rev"))
		{
			if(*argv[u8LoopCntr+1] == 'A')
			{
				*chipId = 0x1502b1;
				continue;
			}
			if(*argv[u8LoopCntr+1] == 'B')
			{
				*chipId = 0x1503a0;
				continue;
			}
		}
	}
	if(i==0)
		ret = M2M_ERR_FAIL;
#ifdef NMC1003A0_HP_MULT_GAIN
	if ((HP_DEFAULT_INVALID_GAIN_IDX == gu32GTbIndx) || (gu32GTbIndx > gu32NoOfTables))
		ret = M2M_ERR_FAIL;
#endif
ERR:
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
	char * vflash_path = NULL;
	uint8* vflash = NULL;

	sint8 commands_val = 0;
	nm_bsp_init();
	print_nmi();
	commands_val = checkArguments(argv,argc,&ret,&chipId,&vflash_path);

	if(M2M_ERR_FAIL == commands_val)
	{
#ifdef NMC1003A0_HP_MULT_GAIN
		printf("usage: gain_builder [-table <no_of_tables> [<img_path1> <img_path2> <img_path3> <img_path4>]] [-index <gain_table_index_to_use>(1-4)] [-no_wait] [-port]\n");
#else
		printf("usage: gain_builder [-hp <img_path>] [-lpvbat1 <img_path>] [-lpvbat2 <img_path>] [-lpvbat3 <img_path>] [-vflash_path <vflash_path>] [-no_wait] [-port]\n");
#endif
		return commands_val;
	}
	if(!skipProgramming)
	{
		M2M_PRINT(">>Init Programmer\n");
		ret = programmer_init(&ret,BREAK(commands_val));
		if(ret != M2M_SUCCESS)
		{
			M2M_PRINT("(ERR)Failed To intilize programmer\n");
			goto END;
		}
        dump_flash("Before_gb.bin");

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
	}

#ifdef PROFILING
	{
	uint32 u32T1=GetTickCount();
#endif

	if(M2M_SUCCESS == (ret = burn_gain_settings(vflash))) {
		M2M_PRINT("\nTX Gain values have been downloaded successfully.\r\n");
	} else {
		M2M_PRINT("\n[ERR]TX Gain values have been failed to be downloaded.\r\n");
		goto END;
	}

	if(!skipProgramming)
	{
        dump_flash("After_gb.bin");

        if(vflash)
        {
            M2M_PRINT("Saving vflash to %s\n", vflash_path);
            FILE *fp;
            uint32 sz = programmer_get_flash_size();
            fp = fopen(vflash_path,"wb");
            if(fp != NULL)
            {
                fwrite(vflash,1,sz,fp);
                fclose(fp);
            }
        }

		programmer_deinit();
	}

#ifdef PROFILING
	M2M_PRINT("\n>>This task finished after %5.2f sec\n\r",(GetTickCount() - u32T1)/1000.0);
	}
#endif
END:
	if(!NO_WAIT(commands_val))
		system("pause");

	return ret;
}

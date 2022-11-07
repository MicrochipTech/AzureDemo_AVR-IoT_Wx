/**
*  @file		main.c				
*  @brief		This module contains M2M driver test application
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0	
*/
#include "./App_Donwloader/app_setup.h"
#include "driver/include/m2m_svnrev.h"

/**
* CMD MACROS 
*/
/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE 
*/
#define NO_WAIT_BIT			(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define ARGS_ERR_BIT		(0x40)	/*!< Positiion of arguments_error bit in the byte. */
#define BREAK_BIT		    (0x08)	/*!< Positiion of break bit in the byte. */
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
#define BREAK(x)		    (x & BREAK_BIT)

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
	M2M_PRINT("*  >In Circuit Programmer for SPI Flash<   *\n\r");
	M2M_PRINT("*    Required Board NMC1500                *\n\r");
	M2M_PRINT("*        Owner:  NewPortMediaInc           *\n\r");
	M2M_PRINT("********************************************\n\r");
	M2M_PRINT("SVN REV %u SVN BR %s\n\r",SVN_REVISION,SVN_REL_URL);
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
static uint8 checkArguments(char * argv[],uint8 argc, uint8* portNum,uint8 ** file3A)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 1;
	uint8 portHasValue = 0;
	*portNum = 0;
	if(1 >= argc) goto ERR;
	for(;loopCntr<argc;loopCntr++)
	{
		if(strstr(argv[loopCntr],"-no_wait"))
		{
			SET_NO_WAITE_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-port"))
		{
			if(argc-loopCntr > 1)
			{
				*portNum = atoi(argv[++loopCntr]);
				portHasValue = 1;
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-B"))
		{
			SET_BREAK_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-app3A0_path"))
		{
			if(argc-loopCntr > 1)
			{
				*file3A = argv[++loopCntr];
			}
			continue;
		}
	}
ERR:
	return ret;
}

int main(int argc, char* argv[])
{
	uint8 *app3A0_path = NULL;
	uint8 *file = NULL;
	sint8 ret = M2M_SUCCESS;
	uint8 commands_val = 0;
#ifdef PROFILING
	uint32 u32T1=0;
#endif
	nm_bsp_init();
	print_nmi();
	commands_val = checkArguments(argv,argc,&ret,&app3A0_path);
	if(ARGS_ERR(commands_val)) 
	{
		M2M_ERR("\n>>Invalid arguments\n");
		goto END;
	}
	M2M_PRINT(">>Init Flash\n");
	ret = programmer_init(&ret,BREAK(commands_val));
#ifdef PROFILING
	u32T1=GetTickCount();
#endif
	if(ret != M2M_SUCCESS)
	{
		goto END;
	}

	if(REV(GET_CHIPID()) == REV_3A0)
	{
		file = app3A0_path;
	}
	else
	{
		M2M_PRINT(">>Unsupported variant\n");
		goto ERR;
	}
	

	ret = spi_flash_download_app(file);
	if(ret != M2M_SUCCESS)
	{
		goto ERR;
	}

ERR:
	programmer_deinit();
#ifdef PROFILING
	M2M_PRINT("\n>>This task finished after %5.2f sec\n\r",(GetTickCount() - u32T1)/1000.0);
#endif
END:
	if(!NO_WAIT(commands_val))
		system("pause");
	return ret;	
}

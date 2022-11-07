/*!
@file		app_main.c

@brief		This module contains user Application related functions

@author		M. Abdelmawla
@date		24 MAY 2013
@version	1.0
*/

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#include "nmi_uart.h"
#include <machine/cpu.h>
#include <machine/memsize.h>
#include "driver/source/m2m_hif.h"


#define MAX_CODE_SIZE	(56*1024)
#define MAX_DATA_SIZE   (8*1024)

STACK_SIZE(0);
HEAP_SIZE(0);

PROGRAM_MEMORY_SIZE(MAX_CODE_SIZE);
DATA_MEMORY_SIZE(MAX_DATA_SIZE);

extern unsigned _data_memory_size[];
extern unsigned _program_memory_size[];

extern sint8 app_start(void);

int app_init(tstrFirmLibIn *in) __attribute__((section (".init"), used, externally_visible));
int app_init(tstrFirmLibIn *in)
{
	uint32 u32Sz = (uint32)_data_memory_size + (uint32)_program_memory_size;

	(*((tstrAppInit**)APP_INP_APIS_PTR))->app_firm_load(&gstrFirmLibOut);

	if((*((tstrAppInit**)APP_INP_APIS_PTR))->app_set_code_size(u32Sz) != M2M_SUCCESS)
	{
		M2M_PRINT("Failed To allocate memory\n");
		/*The system will use the default memory = current code size*/
	}

	M2M_PRINT("Cortus APP_INIT\n");

	in->app_handle_resp = hif_Resp_handler;
	in->app_m2m_start = app_start;

	DISABLE_LOGS;
	ENABLE_APP_LOGS;
	ENABLE_ALL_LOGS;
	ENABLE_FIRMWARE_LOGS;
	return 0;
}


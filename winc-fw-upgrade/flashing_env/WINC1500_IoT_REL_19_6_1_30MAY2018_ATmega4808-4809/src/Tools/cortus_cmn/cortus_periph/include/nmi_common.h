/*
 * nmi_common.h
 *
 *  Created on: Dec 17, 2013
 *      Author: mmahfouz
 */

#ifndef NMI_COMMON_H_
#define NMI_COMMON_H_

#include "common/include/nm_common.h"
#include "nmi_gpio.h"


#define BSP_MIN(x,y) ((x)>(y)?(y):(x))

#define WIFI_PERIPH_BASE		0x30000000
#define WIFI_PINMUX_SEL_0		(WIFI_PERIPH_BASE+0x1408)
#define WIFI_PINMUX_SEL_1		(WIFI_PERIPH_BASE+0x140c)
#define WIFI_PINMUX_SEL_2		(WIFI_PERIPH_BASE+0x1410)
#define WIFI_PINMUX_SEL_3		(WIFI_PERIPH_BASE+0x15C0)
#define WIFI_PULL_ENABLE		(WIFI_PERIPH_BASE+0x142C)
#define WIFI_IC_BASE			0x40000300


typedef enum{
	NM_SUCCESS 		= ((sint8) 0),
	NM_TIMER_FAIL 	= ((sint8) -1),
	NM_GPIO_FAIL 	= ((sint8) -2),
	MM_I2C_FAIL 	= ((sint8) -3),
	NM_SPI_FAIL 	= ((sint8) -4),
	NM_UART_FAIL 	= ((sint8) -5),
	NM_OS_FAIL 	= ((sint8) -6)
}enuErrorCode;




/**@brief Macro for calling error handler function if supplied error code any other than NM_SUCCESS.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        if (ERR_CODE != NM_SUCCESS)                		    \
        {                                                   \
            goto APP_ERR;					                \
        }                                                   \
    } while (0)
/*!
*  @struct 		_Ic
*  @brief		Interrupt control Structure
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
typedef struct _Ic {

	/*
	 global interrupt enable register:
	 bit 0 - global interrupt enable,
	 bit [8:15] - current interrupt request,
	 bit [16:23] - current interrupt priority,
	 bit [24:31] - previous interrupt priority
	 */
	volatile unsigned int glb_ctl;
	volatile unsigned int reserve1;
	volatile unsigned int reserve2;
	volatile unsigned int reserve3;
	volatile unsigned int reserve4; /* 0x10 */
	volatile unsigned int reserve5;
	volatile unsigned int reserve6;
	volatile unsigned int reserve7;
	volatile unsigned int reserve8;
	volatile unsigned int reserve9;
	/*
	 Gpio register:
	 bit 0 - enable,
	 bit [8:15] - priority,
	 */
	/*28*/volatile unsigned int gpio;

	/*
	 Uart register:
	 bit 0 - enable,
	 bit [8:15] - priority,
	 */
	volatile unsigned int uart_tx;

	/*
	 Uart register:
	 bit 0 - enable,
	 bit [8:15] - priority,
	 */
	volatile unsigned int uart_rx;
	volatile unsigned int reserved10;
	volatile unsigned int reserved11;
	volatile unsigned int reserved12;
	volatile unsigned int reserved13;
	volatile unsigned int reserved14;

	/*
	 Spi register:
	 bit 0 - enable,
	 bit [8:15] - priority,
	 */
	volatile unsigned int spi;
	volatile unsigned int reserved15;
	volatile unsigned int reserved16;
	volatile unsigned int reserved17;

	/*
	 i2c master register:
	 bit 0 - enable,
	 bit [8:15] - priority,
	 */
	volatile unsigned int i2c_master;
	/*23
			sleep timer register:
				bit 0 - enable,
				bit [8:15] - priority,
		*/
		volatile unsigned int sleep_timer;
		/*24
			vmm register:
				bit 0 - enable,
				bit [8:15] - priority,
		*/
		volatile unsigned int vmm;

		/*25
			host vmm register:
				bit 0 - enable,
				bit [8:15] - priority,
		*/
		volatile unsigned int host_vmm;
		/*26
		 *
		 */
		volatile unsigned int reserved26;
		/*27
		 *
		 */
		volatile unsigned int reserved27;
		/*28
		 *
		 */
		volatile unsigned int reserved28;
		/*29
		 *
		 */
		volatile unsigned int reserved29;
		/*30
		 *
		 */
		volatile unsigned int reserved30;
		/*31
		 *
		 */
		volatile unsigned int reserved31;
		/*32
		 *
		 */
		volatile unsigned int reserved32;
		/*33
		 *
		 */
		volatile unsigned int reserved33;
		/*34
		 *
		 */
		volatile unsigned int reserved34;
		/*35
		 *
		 */
		volatile unsigned int shortBeacon;
		/*36
		 *
		 */
		volatile unsigned int reserved36;
		/*37
		 *
		 */
		volatile unsigned int reserved37;
		/*38
		 *
		 */
		volatile unsigned int reserved38;
		/*39
		 *
		 */
		volatile unsigned int reserved39;
		/*40
		 *
		 */
		volatile unsigned int reserved40;
		/*41
		 *
		 */
		volatile unsigned int reserved41;
		/*42
		 *
		 */
		volatile unsigned int reserved42;
		/*43
		 *
		 */
		volatile unsigned int reserved43;
		/*44
		 *
		 */
		volatile unsigned int slp_timer1;
		/*45
		 *
		 */
		volatile unsigned int slp_timer2;
		/*46
		 *
		 */
		volatile unsigned int reserved46;
		/*47
		 *
		 */
		volatile unsigned int aes;
		/*48
		 *
		 */
		volatile unsigned int bigint;
		/*49
		 *
		 */
		volatile unsigned int sha;
		/*50
		 *
		 */
		volatile unsigned int reserved50;
		/*51
		 *
		 */
		volatile unsigned int reserved51;
		/*52
		 *
		 */
		volatile unsigned int reserved52;
		/*53
		 *
		 */
		volatile unsigned int reserved53;
		/*54
		 *
		 */
		volatile unsigned int reserved54;
		/*55
		 *
		 */
		volatile unsigned int reserved55;
		/*56
		 *
		 */
		volatile unsigned int reserved56;
		/*57
		 *
		 */
		volatile unsigned int reserved57;
		/*58
		 *
		 */
		volatile unsigned int reserved58;
		/*59
		 *
		 */
		volatile unsigned int reserved59;
		/*60
		 *
		 */
		volatile unsigned int reserved60;
		/*61
		 *
		 */
		volatile unsigned int dmac;
		/*62
			uart2 tx register:
				bit 0 - enable,
				bit [8:15] - priority,
		*/
		volatile unsigned int uart2_tx; /*62*/
		/*63
			uart2 Rx register:
				bit 0 - enable,
				bit [8:15] - priority,
		*/

		volatile unsigned int uart2_rx; /*63*/

} Ic;

#endif /* NMI_COMMON_H_ */

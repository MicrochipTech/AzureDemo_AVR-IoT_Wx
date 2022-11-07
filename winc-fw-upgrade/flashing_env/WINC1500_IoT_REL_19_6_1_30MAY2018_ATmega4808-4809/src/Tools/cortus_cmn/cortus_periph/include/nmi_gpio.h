/**
*  @file		nmi_gpio.h
*  @brief		This module contains gpio APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifndef __NMI_GPIO_H__
#define __NMI_GPIO_H__
/**
 * INCLUDE
 */
#include "nmi_common.h"
/**
 * MACROS
 */
 /*!
@enum	\
	tenuGpio

@brief
	suported gpio numbers 

*/
typedef enum {
	GPIO_0_HOST_WAKEUP,
	GPIO_1_RTC_CLK,
	GPIO_2_IRQN,
	GPIO_3,
	GPIO_4,
	GPIO_5,
	GPIO_6,
	GPIO_7_SD_CLK,
	SD_DAT0_SPI_TXD,
	SD_DAT2_SPI_SSN,
	SD_DAT1_SPI_RXD,
	SD_CMD_SPI_SCK,
	GPIO_8_SD_DAT3,
	GPIO_11,
	GPIO_12,
	GPIO_13,	
	I2C_SCL,
	I2C_SDA,
	GPIO_14,
	GPIO_15,
	GPIO_16,
	GPIO_17,
	GPIO_18,
	GPIO_21,	
	GPIO_22,
	GPIO_23,
	GPIO_24,
	GPIO_MAX,
} tenuGpio;

/*!
@enum	\
	enuEdgeTrigger

@brief
	edge trigger 

*/
typedef enum{
	FAILLING_EDGE = 0,
	RAISING_EDGE = 1,
}enuEdgeTrigger;
/*!
@enum	\
	tenuPullupMask

@brief
	Bitwise-ORed flags for use in nm_gpio_pullup_ctrl.

*/
typedef enum {
	PULLUP_GPIO_0_HOST_WAKEUP    = (1ul << 0),
	PULLUP_GPIO_1_RTC_CLK        = (1ul << 1),
	PULLUP_GPIO_2_IRQN           = (1ul << 2),
	PULLUP_GPIO_3          		 = (1ul << 3),
	PULLUP_GPIO_4          		 = (1ul << 4),
	PULLUP_GPIO_5                = (1ul << 5),
	PULLUP_GPIO_8_SD_DAT3        = (1ul << 6),
	PULLUP_SD_DAT2_SPI_RXD       = (1ul << 7),
	PULLUP_SD_DAT1_SPI_SSN       = (1ul << 8),
	PULLUP_SD_CMD_SPI_SCK        = (1ul << 9),
	PULLUP_SD_DAT0_SPI_TXD      = (1ul << 10),
	PULLUP_GPIO_6               = (1ul << 11),
	PULLUP_GPIO_7_SD_CLK        = (1ul << 12),
	PULLUP_I2C_SCL         		= (1ul << 13),
	PULLUP_I2C_SDA         		= (1ul << 14),
	PULLUP_GPIO_11         		= (1ul << 15),
	PULLUP_GPIO_12        		= (1ul << 16),
	PULLUP_GPIO_13        		= (1ul << 17),
	PULLUP_GPIO_14        		= (1ul << 18),
	PULLUP_GPIO_15         		= (1ul << 19),
	PULLUP_GPIO_16         		= (1ul << 20),
	PULLUP_GPIO_17         		= (1ul << 21),
	PULLUP_GPIO_18         		= (1ul << 22),
	PULLUP_GPIO_19         		= (1ul << 23),
	PULLUP_GPIO_20         = (1ul << 24),
	PULLUP_GPIO_21         = (1ul << 25),
	PULLUP_GPIO_22         = (1ul << 26),
	PULLUP_GPIO_23         = (1ul << 27),
	PULLUP_GPIO_24         = (1ul << 28),
} tenuPullupMask;

/*!
@enum	\
	tenuWakeMask

@brief
	suported gpios for wakeup, used in nm_gpio_wakeup_XXX

*/
typedef enum {
	WAKE_GPIO_19,
	WAKE_GPIO_20,
	WAKE_GPIO_21,
	WAKE_GPIO_22,
	WAKE_GPIO_23,
	WAKE_GPIO_24,
	WAKE_GPIO_11,
	WAKE_GPIO_12,
	WAKE_GPIO_13,
	WAKE_GPIO_14,
	WAKE_GPIO_15,
	WAKE_GPIO_16,
	WAKE_GPIO_17,
	WAKE_GPIO_18,
	WAKE_GPIO_3,
	WAKE_GPIO_4,
	WAKE_GPIO_5,
	WAKE_GPIO_6,
	WAKE_IRQN,
	WAKE_SD_DAT1_SPI_SSN,
	WAKE_RTC_CLK,
	WAKE_SD_CLK,
	WAKE_I2C_SDA,
	WAKE_SD_DAT2_SPI_RXD,
	WAKE_SD_CMD_SPI_SCK,
} tenuWakeMask;

/**
 * STRUCTURE
 */
/*!
*  @brief		Interrupt states
*/
typedef enum{
	DISABLED,
	ENABLED
}tenuInteruptCtrl;
/*!
*  @struct 		tpfNmIsr
*  @brief		Call back function for gpio isr.
*/
typedef void (*tpfNmIsr)(void);
/*!
*  @enum 		tenuInteruptType
*  @brief		Interrupt type {low level or high level}
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/

typedef enum{
	LOW_LEVEL,
	HIGH_LEVEL
}tenuInteruptType;

/*!
*  @fn			sint8 nm_gpio_init(void);
*  @details		Initialize gpio module
*  @return		if NM_SUCCESS, or any error
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_gpio_init(void);
/*!
*	@fn		nm_bsp_register_isr
*	@brief	Register interrupt service routine
*	@param[IN]	u8GpioNum
*				Gpio number {GPIO_0,..}
*	@param[IN]	pfIsr
*				Pointer to ISR handler
*	@param[IN]	enuInt
*				Interrupt type {low level or high level}
*	@sa		tpfNmBspIsr ,tenuInteruptType
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0
*/
sint8 nm_gpio_register_isr(uint8 u8GpioNum,tpfNmBspIsr pfIsr,tenuInteruptType enuInt);

/*!
*	@fn		nm_bsp_interrupt_ctrl
*	@brief	Enable/Disable interrupts
*	@param[IN]	u8GpioNum
*				Gpio number {GPIO_0,..}
*	@param[IN]	u8Enable
				'0' disable interrupts. '1' enable interrupts
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
void nm_gpio_interrupt_ctrl(uint8 u8GpioNum,uint8 u8Enable);

/*!
*  @fn			void nm_gpio_output_ctrl(uint8 u8GpioNum,uint8 u8State)
*  @details		Control gpio output
*  @param[in]	u8GpioNum: Gpio number {GPIO_0,..}
*  @param[in]	u8State: '0' {OFF,LOW}  '1' {HIGH,ON}
*  @return		NONE
*  @sa			nm_gpio_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_gpio_output_ctrl(uint8 u8GpioNum,uint8 u8State);
/*!
*  @brief		get output gpio state
*  @param[in]	u8GpioNum: Gpio number {GPIO_0,..}
*  @return		 '0' {OFF,LOW}  '1' {HIGH,ON}
*  @sa			nm_gpio_init,nm_gpio_output_ctrl
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
uint8 nm_gpio_get_out_state(uint8 u8GpioNum);
/*!
*  @brief		get input gpio state
*  @param[in]	u8GpioNum: Gpio number {GPIO_0,..}
*  @return		 '0' {OFF,LOW}  '1' {HIGH,ON}
*  @sa			nm_gpio_init,nm_gpio_register_isr
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
uint8 nm_gpio_get_in_state(uint8 u8GpioNum);
/*!
*  @brief	
*  @param[in] u32GpioPullMask: tenuPullupMask set any gpio pull from that enum
*  @param[in] enable: 1 to enable the pull up 0 to disable the pull up
*  @return		M2M_SUCCESS if fail return -ve number
*  @sa			tenuPullupMask
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_gpio_pullup_ctrl(uint32 u32GpioPullMask, uint8 enable);
/*
*	@fn		sint8 nm_gpio_clear_mux(uint8 u8GpioNum)
*   @param[in] u8GpioNum: used from that enum tenuGpio
*	@brief 	clear gpio mux , it will enabel the gpio mux by default
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_clear_mux(uint8 u8GpioNum);
/*
*	@fn sint8 nm_gpio_set_mux(uint8 u8GpioNum,uint32 u32mux);
*	@brief set gpio mux tenuGpio,(review the pads excel for more informations) 
*   @param[in] u8GpioNum: used from that enum tenuGpio
*   @param[in] u32mux: (review the pads excel for more informations) 
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_set_mux(uint8 u8GpioNum,uint32 u32mux);
/*
*	@fn		uint32 nm_gpio_get_mux(uint8 u8GpioNum)
*   @param[in] u8GpioNum: used from that enum tenuGpio
*	@brief 	get gpio mux
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
uint32 nm_gpio_get_mux(uint8 u8GpioNum);
/*
*	@fn sint8 nm_gpio_wakeup_ctrl(uint8 u8GpioWakeMux,uint8 u8Enable)
*	@brief		it enable the wakeup on couple of gpios in tenuWakeMask
*   @param[in] u8GpioWakeMux: used from that enum tenuWakeMask
*   @param[in] u8Enable: 1 to enable the waekup on that gpio/ 0 to disable
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_ctrl(uint8 u8GpioWakeMux,uint8 u8Enable);
/*
*	@fn sint8 nm_gpio_wakeup_pol(uint8 u8GpioWakeMux,uint8 u8EdgeTriger)
*	@brief control the polarity of wakeup enuEdgeTrigger edge 	FAILLING_EDGE = 0,RAISING_EDGE = 1,
*   @param[in] u8GpioWakeMux: used from that enum tenuWakeMask
*   @param[in] u8EdgeTriger: FAILLING_EDGE = 0,RAISING_EDGE = 1,
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_pol(uint8 u8GpioWakeMux,uint8 u8EdgeTriger);
/*
*	@fn uint8 nm_gpio_wakeup_sts(uint8 u8GpioWakeMux)
*	@brief Read the gpio status "1" means it cause wakeup and need to be cleared to let the sysytem goto sleep
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
uint8 nm_gpio_wakeup_sts(uint8 u8GpioWakeMux);
/*
*	@fn uint8 nm_gpio_wakeup_clr_irq(uint8 u8GpioWakeMux)
*	@brief clear the gpio status to let the system goto sleep
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_clr_irq(uint8 u8GpioWakeMux);


#endif /*__NMI_GPIO_H__*/

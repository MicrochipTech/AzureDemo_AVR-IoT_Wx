
/**
*  @file		nmi_gpio.c
*  @brief		This module contains gpio APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/

/**
 * INCLUDE
 */
#include "nmi_gpio.h"
#include "driver/source/nmbus.h"

#define GPIO_WAKEUP_TRIG_EN		(WIFI_PERIPH_BASE + 0x1590)
#define GPIO_WAKEUP_TRIG_POL	(WIFI_PERIPH_BASE + 0x1594)
#define GPIO_WAKEUP_TRIG_STATS	(WIFI_PERIPH_BASE + 0x1598)

/**
 *
 * STRUCTURE
 */
typedef struct
{
	tpfNmBspIsr gpfIsr;
}
tstrNmIsr;

typedef struct _Gpio
{
	volatile unsigned int out;
	volatile unsigned int in;
	volatile unsigned int dir;
	volatile unsigned int old_in;	/* this is used to generate interrupt */
	volatile unsigned int mask;
} Gpio;

/*!
*  @brief		mux registers
*/
typedef enum{

	GPIO_MUX_0 = 0,
	GPIO_MUX_1 = SD_DAT0_SPI_TXD,
	GPIO_MUX_2 = I2C_SCL,
	GPIO_MUX_3 = GPIO_22,
}enuGpioMux;
/**
 *
 * MACROS
 */
#define WIFI_GPIO_BASE			0x40000100

/**
 *
 * GLOBAL VAR
 */
static tstrNmIsr gstrNmIsr[GPIO_MAX];

/*
*	@fn		gpio_isr_handler
*	@brief	interrupt service routine
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
static void gpio_isr_handler(void)
{
	uint8 i = 0;
	Gpio *gio = (Gpio *) WIFI_GPIO_BASE;
	unsigned pc,sta/*,sta2*/;

	asm ("mov %0, rtt" : "=r"(pc) : /*no inputs*/);
	sta = (gio->mask)&0xfff;

	while (i < GPIO_MAX) {
		if ((sta >> i) & 0x1) {
			if (((gio->in >> i) & 0x1) != ((gio->old_in >> i) & 0x1)) {
				if (gstrNmIsr[i].gpfIsr) {
					gstrNmIsr[i].gpfIsr();
				}
			}
		}
		i++;
	}
}

/*!
*  @fn			sint8 nm_gpio_init(void);
*  @details		Initialize gpio module
*  @return		if NM_SUCCESS, or any error
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_gpio_init(void)
{
	int os_ret;
	sint8 ret = NM_SUCCESS;
	Ic *ic = (Ic *) WIFI_IC_BASE;
	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	os_ret = app_os_interrupt_register(10, gpio_isr_handler);
	if (OS_SUCCESS != os_ret) {
		ret = NM_OS_FAIL;
	}
	m2m_memset((uint8*) &gstrNmIsr, 0, sizeof(gstrNmIsr));
	ic->gpio = 1;
	gio->mask = 0;
	return ret;
}
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
sint8 nm_gpio_register_isr(uint8 u8GpioNum,tpfNmBspIsr pfIsr,tenuInteruptType enuInt)
{
	sint8 ret = NM_SUCCESS;
	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	ret = nm_gpio_clear_mux(u8GpioNum);
	if (ret == NM_SUCCESS) {
		gio->dir &= ~(1 << u8GpioNum);
		if (enuInt == HIGH_LEVEL) {
			gio->old_in &= ~(1UL << u8GpioNum);
		} else {
			gio->old_in |= (1UL << u8GpioNum);
		}
		gio->mask &= ~(1UL << u8GpioNum);

		gstrNmIsr[u8GpioNum].gpfIsr = pfIsr;

	}
	return ret;
}
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
void nm_gpio_interrupt_ctrl(uint8 u8GpioNum,uint8 u8Enable)
{

	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	if (u8Enable) {
		gio->mask |= (1UL << u8GpioNum);
	} else {
		gio->mask &= ~(1UL << u8GpioNum);
	}
}
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
sint8 nm_gpio_output_ctrl(uint8 u8GpioNum,uint8 u8State)
{
	sint8 ret = NM_SUCCESS;
	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	ret = nm_gpio_clear_mux(u8GpioNum);
	if (ret == NM_SUCCESS) {
		gio->dir |= (1UL << u8GpioNum);
		if (u8State) {
			gio->out |= (1UL << u8GpioNum);
		} else {
			gio->out &= ~(1UL << u8GpioNum);
		}
	}
	return ret;
}

/*!
*  @brief		get output gpio state
*  @param[in]	u8GpioNum: Gpio number {GPIO_0,..}
*  @return		 '0' {OFF,LOW}  '1' {HIGH,ON}
*  @sa			nm_gpio_init,nm_gpio_output_ctrl
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
uint8 nm_gpio_get_out_state(uint8 u8GpioNum)
{
	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	uint32 val = 0;
	val = gio->out;
	return (val >> u8GpioNum)&0x1;
}
/*!
*  @brief		get input gpio state
*  @param[in]	u8GpioNum: Gpio number {GPIO_0,..}
*  @return		 '0' {OFF,LOW}  '1' {HIGH,ON}
*  @sa			nm_gpio_init,nm_gpio_register_isr
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
uint8 nm_gpio_get_in_state(uint8 u8GpioNum)
{
	Gpio *gio = (Gpio *) (WIFI_GPIO_BASE);
	uint32 val = 0;
	val = gio->in;
	return (val >> u8GpioNum)&0x1;
}
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
sint8 nm_gpio_pullup_ctrl(uint32 u32GpioPullMask, uint8 enable)
{
	volatile unsigned int* pullEn = (volatile unsigned int*)(WIFI_PULL_ENABLE);
	if(enable) {
		*pullEn &= ~u32GpioPullMask;
		} else {
		*pullEn |= u32GpioPullMask;
	}
	return M2M_SUCCESS;

}


/*
*	@fn		sint8 nm_gpio_clear_mux(uint8 u8GpioNum)
*   @param[in] u8GpioNum: used from that enum tenuGpio
*	@brief 	clear gpio mux , it will enabel the gpio mux by default
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_clear_mux(uint8 u8GpioNum)
{
	sint8 ret = NM_SUCCESS;
	volatile unsigned int *pin_mux =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_0);
	volatile unsigned int *pin_mux3 =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_3);
	if ((u8GpioNum >= GPIO_MUX_0)&&(u8GpioNum < GPIO_MUX_1)) {
		*pin_mux &= ~(0x7 << (u8GpioNum * 4));
	} else if ((u8GpioNum >= GPIO_MUX_1) && (u8GpioNum < GPIO_MUX_2)) {
		*(pin_mux + 1) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_1) * 4));
	} else if ((u8GpioNum >= GPIO_MUX_2) && (u8GpioNum < GPIO_MUX_3)) {
		*(pin_mux + 2) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_2) * 4));
	} else if (u8GpioNum >= GPIO_MUX_3) {
		*(pin_mux3) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_3) * 4));
	} else {
		ret = NM_UART_FAIL;
	}
	return ret;
}
/*
*	@fn		uint32 nm_gpio_get_mux(uint8 u8GpioNum)
*   @param[in] u8GpioNum: used from that enum tenuGpio
*	@brief 	get gpio mux
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
uint32 nm_gpio_get_mux(uint8 u8GpioNum)
{
	uint32 val32 = 0;
	volatile unsigned int *pin_mux =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_0);
	volatile unsigned int *pin_mux3 =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_3);
	if ((u8GpioNum >= GPIO_MUX_0)&&(u8GpioNum < GPIO_MUX_1)) {
		val32 = ((*pin_mux) >> (u8GpioNum * 4))&0xf;
	} else if ((u8GpioNum >= GPIO_MUX_1) && (u8GpioNum < GPIO_MUX_2)) {
		val32 = ((*(pin_mux + 1)) >> ((u8GpioNum - GPIO_MUX_1) * 4))&0xf;
	} else if ((u8GpioNum >= GPIO_MUX_2) && (u8GpioNum < GPIO_MUX_3)) {
		val32 = ((*(pin_mux + 2)) >> ((u8GpioNum - GPIO_MUX_2) * 4))&0xf;
	} else if (u8GpioNum >= GPIO_MUX_3) {
		val32 = ((*(pin_mux3)) >> ((u8GpioNum - GPIO_MUX_3) * 4))&0xf;
	}
	return val32;
}
/*
*	@fn
*	@brief set gpio mux tenuGpio,(review the pads excel for more informations) 
*   @param[in] u8GpioNum: used from that enum tenuGpio
*   @param[in] u32mux: (review the pads excel for more informations) 
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_set_mux(uint8 u8GpioNum,uint32 u32mux)
{
	sint8 ret = NM_SUCCESS;
	volatile unsigned int *pin_mux =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_0);
	volatile unsigned int *pin_mux3 =
			(volatile unsigned int *) (WIFI_PINMUX_SEL_3);
	if ((u8GpioNum >= GPIO_MUX_0)&&(u8GpioNum < GPIO_MUX_1)) {
		*pin_mux &= ~(0x7 << (u8GpioNum * 4));
		*pin_mux |= (u32mux << (u8GpioNum * 4));
	} else if ((u8GpioNum >= GPIO_MUX_1) && (u8GpioNum < GPIO_MUX_2)) {
		*(pin_mux + 1) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_1) * 4));
		*(pin_mux + 1) |= (u32mux << ((u8GpioNum - GPIO_MUX_1) * 4));
	} else if ((u8GpioNum >= GPIO_MUX_2) && (u8GpioNum < GPIO_MUX_3)) {
		*(pin_mux + 2) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_2) * 4));
		*(pin_mux + 2) |= (u32mux << ((u8GpioNum - GPIO_MUX_2) * 4));
	} else if (u8GpioNum >= GPIO_MUX_3) {
		*(pin_mux3) &= ~(0x7 << ((u8GpioNum - GPIO_MUX_3) * 4));
		*(pin_mux3) |= (u32mux << ((u8GpioNum - GPIO_MUX_3) * 4));
	} else {
		ret = NM_UART_FAIL;
	}
	return ret;
}
/*
*	@fn sint8 nm_gpio_wakeup_ctrl(uint8 u8GpioWakeMux,uint8 u8Enable)
*	@brief		it enable the wakeup on couple of gpios in tenuWakeMask
*   @param[in] u8GpioWakeMux: used from that enum tenuWakeMask
*   @param[in] u8Enable: 1 to enable the waekup on that gpio/ 0 to disable
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_ctrl(uint8 u8GpioWakeMux,uint8 u8Enable)
{
#ifdef NMC1003A0
	sint8 ret = NM_SUCCESS;
	volatile unsigned int *trig_en = (volatile unsigned int *) (GPIO_WAKEUP_TRIG_EN);

	if (u8Enable) {
		*trig_en |= (1 << u8GpioWakeMux);
	} else {
		*trig_en &= ~(1 << u8GpioWakeMux);
	}
	return ret;
#else
	return NM_GPIO_FAIL;
#endif
}
/*
*	@fn sint8 nm_gpio_wakeup_pol(uint8 u8GpioWakeMux,uint8 u8EdgeTriger)
*	@brief control the polarity of wakeup enuEdgeTrigger edge 	FAILLING_EDGE = 0,RAISING_EDGE = 1,
*   @param[in] u8GpioWakeMux: used from that enum tenuWakeMask
*   @param[in] u8EdgeTriger: FAILLING_EDGE = 0,RAISING_EDGE = 1,
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_pol(uint8 u8GpioWakeMux,uint8 u8EdgeTriger)
{
#ifdef NMC1003A0
	sint8 ret = NM_SUCCESS;
	volatile unsigned int *trig_pol = (volatile unsigned int *) (GPIO_WAKEUP_TRIG_POL);

	if (u8EdgeTriger) {
		*trig_pol |= (1 << u8GpioWakeMux);
	} else {
		*trig_pol &= ~(1 << u8GpioWakeMux);
	}

	return ret;
#else
	return NM_GPIO_FAIL;
#endif
}
/*
*	@fn uint8 nm_gpio_wakeup_sts(uint8 u8GpioWakeMux)
*	@brief Read the gpio status "1" means it cause wakeup and need to be cleared to let the sysytem goto sleep
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
uint8 nm_gpio_wakeup_sts(uint8 u8GpioWakeMux)
{
#ifdef NMC1003A0
	volatile unsigned int *trig_stats = (volatile unsigned int *) (GPIO_WAKEUP_TRIG_STATS);
	uint32 val = *trig_stats;

	return (uint8)((val >> u8GpioWakeMux) & 0x1);
#else
	return 0;
#endif
}
/*
*	@fn uint8 nm_gpio_wakeup_clr_irq(uint8 u8GpioWakeMux)
*	@brief clear the gpio status to let the system goto sleep
*	@return NM_sucess if it cleared
*	@author	M.S.M
*	@date	30 DEC 2013
*	@version	1.0
*/
sint8 nm_gpio_wakeup_clr_irq(uint8 u8GpioWakeMux)
{
	sint8 ret = NM_GPIO_FAIL;
#ifdef NMC1003A0
	volatile unsigned int *trig_stats = (volatile unsigned int *) (GPIO_WAKEUP_TRIG_STATS);
	volatile unsigned int *trig_en = (volatile unsigned int *) (GPIO_WAKEUP_TRIG_EN);

	uint32 val = *trig_stats;

	if(((val >> u8GpioWakeMux) & 0x1))
	{
		*trig_en &= ~(1 << u8GpioWakeMux);
		*trig_en |= (1 << u8GpioWakeMux);
		ret = NM_SUCCESS;
	}
#endif
	return ret;
}


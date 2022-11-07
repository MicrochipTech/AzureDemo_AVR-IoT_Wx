/**
*  @file		nmi_btn.c
*  @brief		This module contains btn APIs implementation
*  @author		M.S.M
*  @date		10 JULY 2012
*  @version		1.0
*/
/**
 * INCLUDE
 */
#include "nmi_btn.h"
#include "nmi_gpio.h"

/**
 *
 * MACROS
 */
#define TICK_RES_MS				(40)
#define SHORT_PRESS_DEBOUNCE	(120/TICK_RES_MS)	/*100 ms*/
#define LONG_PRESS_DEBOUNCE		(2000/TICK_RES_MS)	/*2 sec*/

#define PRESS_SEL(x,m1,m2,m3)		((x>SHORT_PRESS_DEBOUNCE)?((x>LONG_PRESS_DEBOUNCE)?(m3):(m2)):(m1))

#define BTN_TICK   (20)
#define BTN_DEBOUNCE (25)


#if (0 == SHORT_PRESS_DEBOUNCE)
#undef SHORT_PRESS_DEBOUNCE
#define SHORT_PRESS_DEBOUNCE 1
#endif

#define SW1_GPIO GPIO_0_HOST_WAKEUP
#define SW2_GPIO

/**
 *
 * Global variables
 */

static uint16 gu16Btn1Cnt;
static tpfNmBspBtnPress gpfBtns;
static uint8 gu8BtnIfg;
static tstrOsTimer gstrTimerBtn;
static uint8 gu8BTactive;

/**
 *
 * STATIC FUNCTIONS
 */

/**
 *
 * Timer tick
 */
static void btn_poll(void *p)
{
	if (gu8BtnIfg & SW1) {
		gu16Btn1Cnt++;

		if (gu16Btn1Cnt >= SHORT_PRESS_DEBOUNCE) {
			if (gu16Btn1Cnt >= LONG_PRESS_DEBOUNCE) {
				if (!nm_gpio_get_in_state(SW1_GPIO)) {
					gpfBtns(SW1, LONG_PRESS); /* long press callback */
					gu16Btn1Cnt = 0;
				}
			}
			else
			{
				if (!nm_gpio_get_in_state(SW1_GPIO)) {
					gpfBtns(SW1, SHORT_PRESS); /* Short press callback */
					gu16Btn1Cnt = 0;
				}
			}
		}
	}
	M2M_DBG("%s %d %d\n",PRESS_SEL(gu16Btn1Cnt,"NA","S","L"),gu16Btn1Cnt,gu8BtnIfg);
	if (!gu16Btn1Cnt) {
		gu8BtnIfg &= ~SW1;
		app_os_timer_stop(&gstrTimerBtn);
		gu8BTactive = 0;
		nm_gpio_interrupt_ctrl(SW1_GPIO, (uint8) ENABLED);
	}
}
/*
 * BTN ISR
 * */
static void btn1_isr(void)
{
	int ret = 0;
	M2M_DBG("Btn_INT\n");
	gu8BtnIfg |=SW1;
	nm_gpio_interrupt_ctrl(SW1_GPIO, (uint8) DISABLED);
	if (!gu8BTactive) {
		gu8BTactive = 1;
		ret = app_os_timer_start(&gstrTimerBtn, "App_btn", btn_poll,
				(TICK_RES_MS/OS_TICK_RES), 1, NULL, 1);
		if (ret) {
			M2M_ERR("Can't start timer ret %d\n", ret);
			gu8BTactive = 0;
		}
	}

}
/*!
*  @fn			void nm_btn_init(tpfNmBspBtnPress pfBtnCb);
*  @brief
*  @details		Intilize btn module
*  @pfBtnCb[in]	pointer to btn call back
*  @return		NM_SUCESES if succeed, else fail.
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_btn_init(tpfNmBspBtnPress pfBtnCb)
{
	sint8 ret = NM_SUCCESS;
	gpfBtns = pfBtnCb;
	gu8BtnIfg = 0;
	gu16Btn1Cnt = 0;
	gu8BTactive = 0;
	m2m_memset((uint8*) &gstrTimerBtn, 0, sizeof(gstrTimerBtn));
	ret = nm_gpio_init();
	APP_ERROR_CHECK(ret);
	ret = nm_gpio_register_isr(SW1_GPIO,btn1_isr,HIGH_LEVEL);
	APP_ERROR_CHECK(ret);
	nm_gpio_interrupt_ctrl(SW1_GPIO,(uint8)ENABLED);


APP_ERR:
	return ret;
}

/*!
*  @fn			void nm_btn_deinit(void);
*  @details		Deinit btn module
*  @return		None
*  @sa			nm_btn_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
void nm_btn_deinit(void)
{
	gpfBtns = NULL;
	gu8BtnIfg = 0;
	gu16Btn1Cnt = 0;
	gu8BTactive = 0;
	nm_gpio_interrupt_ctrl(SW1_GPIO,(uint8)DISABLED);
	nm_gpio_register_isr(SW1_GPIO,NULL,LOW_LEVEL);
}


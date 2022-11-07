/**
*  @file		nmi_uart.c
*  @brief		This module contains i2c APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifdef NMI_USE_UART
/**
 * INCLUDE
 */
#include "nmi_uart.h"
/**
 * STRUCTURE
 */
typedef struct _Uart {
	/* Transmit data fifo */
	volatile unsigned tx_data;

	/* Tranmsmit status
	 5:    cts              1 means tx is clear to send
	 4:    empty            1 if fifo is entirely empty
	 3:    3/4 empty        1 if at least 3/4 of fifo is empty
	 2:    1/2 empty        1 if at least 1/2 of fifo is empty
	 1:    1/4 empty        1 if at least 1/4 of fifo is empty
	 0:    !full            1 if fifo is not full */
	volatile unsigned tx_status;

	/* Transmit interrupt mask */
	volatile unsigned tx_mask;

	unsigned __fill0x0c;

	/* Receive data fifo */
	volatile unsigned rx_data;

	/* Receive status
	 7:    framing error    1 if missing stop bit
	 6:    overrun          1 if fifo overrun has occured
	 :       -
	 3:    3/4 full         1 if fifo at least 3/4 full
	 2:    1/2 full         1 if fifo at least 1/2 full
	 1:    1/4 full         1 if fifo at least 1/4 full
	 0:    !empty           1 if some data is available */
	volatile unsigned rx_status;

	/* Receive interrupt mask */
	volatile unsigned rx_mask;

	/* Timeout value */
	volatile unsigned rx_timeout;

	/*
	 * The following registers are only present in the full version of the
	 * uart implementation.
	 */

	/* UART configuration register (default 0)
	 5:     cts_en          cts enable for TX
	 4:     nstop   	0 => 1 stop bit, 1 => 2 stop bits
	 [3:2]:     parity          parity mode
	 1:     parity_en       enable parity
	 0:     nbits           0 => 8 bit data, 1 => 7 bits data */
	volatile unsigned config;

	/* parity mode = parity check mode
	 0:   even
	 1:   odd
	 2:   space
	 3:   mark */

	/* Clock divider, this is a 16 bits register with
	 13 bits for the integer part, 3 bits for the fractional part.
	 e.g. 32768Hz clock needs to be divided by 3 3/8 for 9600 (9709) bauds
	 The register is 3 3/8 per default. i.e. 0x001b == 3*8+3 */
	volatile unsigned divider;

	/* Clock source selection
		1002B0		1003A0
	 0: 1.25 Mhz,	6.5M
	 1: 2.5 Mhz,	13M
	 2: 5 Mhz,		26M
	 3: 10 Mhz.		52M
	 */
	volatile unsigned selclk;

} Uart;
/**
 * MACROS
 */
#define WIFI_UART1_BASE 			(0x40000000)
#define WIFI_UART2_BASE 			(0x40000a00)
#define WIFI_MISC_CTRL				(WIFI_PERIPH_BASE+0x1428)

#ifdef NMC1003A0
#define UART_MAX_CLCOK				(52000000UL)
#else
#define UART_MAX_CLCOK				(10000000UL)
#endif

static void delay(uint32 count)
{
    uint32 i = 0;
    volatile uint32 j = 0;

    for(i = 0; i < count; i++)
         j += count;
}
/*!
*  @fn			nm_uart_init
*  @brief		Initialize Uart module
*  @param[in]	enCom: Uart port 
*  @param[in]	pstrUartconfig: Connect UART TX to any gpio {UART_TX_GPIO7,...}/buard rate/flow conntrol
*  @return		nm_success if successes or else if fail
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_init(enUartCom enCom,tstrUartConfig *pstrUartconfig)
{
	sint8 ret = NM_SUCCESS;
    uint16 integerpart = 0;
	uint8 fractionalpart = 0;
	uint32 diff = 0;
	uint8 i = 0;
	Uart *uart;
	if(enCom == UART1){
		uart = (Uart *) WIFI_UART1_BASE;

		switch (pstrUartconfig->u8TXGpioPin) {
			case UART1_TX_GPIO0:
			case UART1_TX_GPIO1:
			case UART1_TX_GPIO7:
			case UART1_TX_GPIO5:{
				nm_gpio_set_mux(pstrUartconfig->u8TXGpioPin,0x3UL);
			}
			break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
		switch (pstrUartconfig->u8RxGpioPin) {
			case UART1_RX_GPIO2:
			case UART1_RX_GPIO3: {
				nm_gpio_set_mux(pstrUartconfig->u8RxGpioPin,0x3UL);
			}
				break;
			case UART1_RX_GPIO8: {
				nm_gpio_set_mux(UART1_RX_GPIO8,0x3UL);
			}
				break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
	}
#ifdef NMC1003A0
	else if (enCom == UART2){

		uart = (Uart *) WIFI_UART2_BASE;

		switch (pstrUartconfig->u8TXGpioPin) {
			case UART2_TX_SPI_RXD: {
				volatile unsigned *misc_ctrl = (volatile unsigned*) WIFI_MISC_CTRL;
				/*enable sdio pins mux*/
				*misc_ctrl |= (1ul << 29);
				nm_gpio_set_mux(UART2_TX_SPI_RXD,0x4UL);
			}
				break;
			case UART2_TX_GPIO6: {
				nm_gpio_set_mux(UART2_TX_GPIO6,0x3UL);
			}
				break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
		switch (pstrUartconfig->u8RxGpioPin) {
			case UART2_RX_SPI_TXD: {
				volatile unsigned *misc_ctrl = (volatile unsigned*) WIFI_MISC_CTRL;
				/*enable sdio pins mux*/
				*misc_ctrl |= (1ul << 29);
				nm_gpio_set_mux(UART2_RX_SPI_TXD,0x4UL);
			}
				break;
			case UART2_RX_GPIO4: {
				nm_gpio_set_mux(UART2_RX_GPIO4,0x3UL);
			}
				break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
		switch (pstrUartconfig->u8RtsGpioPin) {
			case UART2_RTS_SPI_SSN: {
				volatile unsigned *misc_ctrl = (volatile unsigned*) WIFI_MISC_CTRL;
				/*enable sdio pins mux*/
				*misc_ctrl |= (1ul << 29);
				nm_gpio_set_mux(UART2_RTS_SPI_SSN,0x4UL);
			}
				break;
			case UART2_RTS_GPIO3: {
				nm_gpio_set_mux(UART2_RTS_GPIO3,0x5UL);
			}
				break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
		switch (pstrUartconfig->u8CtsGpioPin) {
			case UART2_CTS_SPI_SCK: {
				volatile unsigned *misc_ctrl = (volatile unsigned*) WIFI_MISC_CTRL;
				/*enable sdio pins mux*/
				*misc_ctrl |= (1ul << 29);
				nm_gpio_set_mux(UART2_CTS_SPI_SCK,0x4UL);
			}
				break;
			case UART2_CTS_GPIO5: {
				nm_gpio_set_mux(UART2_CTS_GPIO5,0x5UL);
			}
				break;
			default: {
				ret = NM_UART_FAIL;
				goto ERR;
			}
				break;
		}
	}
#endif
	else{
		ret = NM_UART_FAIL;
		goto ERR;
	}
	integerpart = UART_MAX_CLCOK / pstrUartconfig->u32BaudRate;
	diff = UART_MAX_CLCOK - (pstrUartconfig->u32BaudRate * integerpart);
	while (diff > (pstrUartconfig->u32BaudRate / 16)) {
		i++;
		diff -= (pstrUartconfig->u32BaudRate / 16);
	}
	fractionalpart = (i + 1) / 2;
	/**
	 program the uart baud rate (115200)
	 **/
	uart->selclk = 3;
	uart->divider = ((uint32) integerpart << 3)
			| ((uint32) fractionalpart << 0);
	uart->rx_mask = 0x01;

#ifndef NMC1003A0
	if(app_os_sch_get_clk_src())
	{
		/*Ruining on Xo = 26 so the divider should be multiplied by 26/40*/
		uart->divider = ((uart->divider*26)/40);
	}
#endif

	if (pstrUartconfig->u8EnFlowctrl) {
		uart->config = 0x20;
	} else {
		uart->config = 0x0;
	}

ERR:
	return ret;
}
/*!
*  @fn			sint8 nm_uart_register_rx_isr(enUartCom enCom,tpfNmBspIsr pfIsr)
*  @brief		Register rx isr handle
*  @param[in]	pfIsr: rx isr handle
*  @param[in]	enCom: UART COM port
*  @return		nm_success if successes or else if fail
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_register_rx_isr(enUartCom enCom,tpfNmBspIsr pfIsr)
{
	sint8 ret = NM_UART_FAIL;
	Ic *ic = (Ic *) WIFI_IC_BASE;

	if (pfIsr != NULL) {
		if (enCom == UART1) {
			app_os_interrupt_register(12, pfIsr);
			ic->uart_rx = 1;
			ret = NM_SUCCESS;
		} else if (enCom == UART2) {
			app_os_interrupt_register(63, pfIsr);
			ic->uart2_rx = 1;
			ret = NM_SUCCESS;
		}
	}
	return ret;
}
/*!
*  @fn			sint8 nm_uart_register_tx_isr(enUartCom enCom,tpfNmBspIsr pfIsr)
*  @brief		Register tx isr handle
*  @param[in]	pfIsr: tx isr handle
*  @param[in]	enCom: UART COM port
*  @return		nm_success if successes or else if fail
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_register_tx_isr(enUartCom enCom,tpfNmBspIsr pfIsr)
{
	sint8 ret = NM_UART_FAIL;
	Ic *ic = (Ic *) WIFI_IC_BASE;

	if (pfIsr != NULL) {
		if (enCom == UART1) {
			app_os_interrupt_register(11, pfIsr);
			ic->uart_tx = 1;
			ret = NM_SUCCESS;
		} else if (enCom == UART2) {
			app_os_interrupt_register(62, pfIsr);
			ic->uart2_tx = 1;
			ret = NM_SUCCESS;
		}
	}
	return ret;
}
/*!
*  @fn			sint8 nm_uart_send(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz)
*  @brief		Uart recv buffer {blocking function}
*  @param[in]	pu8Buf: uart buffer pointer
*  @param[in]	u16Sz: buffer size
*  @param[in]	enCom: UART COM port
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_send(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz)
{
	Uart *uart;
	uint32 i;
	sint8 ret = NM_SUCCESS;
	if (enCom == UART1) {
		uart = (Uart *) WIFI_UART1_BASE;
	} else if (enCom == UART2) {
		uart = (Uart *) WIFI_UART2_BASE;
	} else {
		ret = NM_UART_FAIL;
		goto ERR;
	}

	for (i = 0; i < u16Sz; i++) {
		while (!(uart->tx_status & 1))
			;
		uart->tx_data = pu8Buf[i];
	}
	ERR: return ret;

}

/*!
*  @fn			sint8 nm_uart_recv(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz);
*  @brief		Uart recv buffer {blocking function}
*  @param[out]	pu8Buf: uart buffer pointer
*  @param[in]	u16Sz: buffer size
*  @param[in]	enCom: UART COM port
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_recv(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz)
{
	Uart *uart;
	sint8 ret = NM_SUCCESS;
	uint32 i, sts;
	if (enCom == UART1) {
		uart = (Uart *) WIFI_UART1_BASE;
	} else if (enCom == UART2) {
		uart = (Uart *) WIFI_UART2_BASE;
	} else {
		ret = NM_UART_FAIL;
		goto ERR;
	}

	for (i = 0; i < u16Sz; i++) {
		do
		{
			sts = uart->rx_status;
			if(sts & NBIT7) return NM_UART_FAIL;

		} while(!(sts & 1));

		pu8Buf[i] = uart->rx_data;
	}
ERR:
	return ret;
}
/*!
*  @fn			sint8 nm_uart_flush(enUartCom enCom)
*  @brief		flush all the UART Buffer
*  @param[in]	enCom: UART COM port
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_flush(enUartCom enCom)
{
	Uart *uart;
	sint8 ret = NM_SUCCESS;
	if (enCom == UART1) {
		uart = (Uart *) WIFI_UART1_BASE;
	} else if (enCom == UART2) {
		uart = (Uart *) WIFI_UART2_BASE;
	} else {
		ret = NM_UART_FAIL;
		goto ERR;
	}
	while (!(uart->tx_status & NBIT4));
	/*
	 * Use divider/2 value as a delay. Required delay for 1 charachter is
	 * propotional to the divider value.
	 */
#ifdef NMC1003A0
	delay(uart->divider >> 4);
#else
	delay(uart->divider >> 1);
#endif

ERR:
	return ret;
}
#endif /*NMI_USE_UART*/


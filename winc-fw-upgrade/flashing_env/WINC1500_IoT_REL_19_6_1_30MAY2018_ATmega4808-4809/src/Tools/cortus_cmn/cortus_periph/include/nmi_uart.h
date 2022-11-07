/**
*  @file		nmi_uart.h
*  @brief		This module contains uart APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifndef __NMI_UART_H__
#define __NMI_UART_H__
/**
 * INCLUDE
 */
#include "nmi_common.h"
#include "nmi_gpio.h"

/**
 * MACROS
 */
#define ENABLE_ALL_LOGS				app_logs_ctrl(LOGS_FIRMWARE|LOGS_M2M|LOGS_APP);
#define ENABLE_M2M_LOGS				app_logs_ctrl(LOGS_M2M);
#define ENABLE_FIRMWARE_LOGS		app_logs_ctrl(LOGS_FIRMWARE);
#define ENABLE_APP_LOGS				app_logs_ctrl(LOGS_APP);
#define DISABLE_LOGS				app_logs_ctrl(LOGS_DISABLED);
/*!
@enum	\
	enUartCom

@brief
	Supported uart on 1003A0, UART1 suported only on B0

*/
typedef enum {
	UART1,
	UART2,
}enUartCom;
/*!
@enum	\
	tenuUart1TxMuxOpt

@brief
	UART1 supported TX gpio mux

*/
typedef enum {
	UART1_TX_GPIO0 = GPIO_0_HOST_WAKEUP,
	UART1_TX_GPIO1 = GPIO_1_RTC_CLK,
	UART1_TX_GPIO5 = GPIO_5,
	UART1_TX_GPIO7 = GPIO_7_SD_CLK,
} tenuUart1TxMuxOpt;
/*!
@enum	\
	tenuUart1RxMuxOpt

@brief
	UART1 supported RX gpio mux

*/
typedef enum {
	UART1_RX_GPIO2 = GPIO_2_IRQN,
	UART1_RX_GPIO3 = GPIO_3,
	UART1_RX_GPIO8 = GPIO_8_SD_DAT3,
} tenuUart1RxMuxOpt;
/*!
@enum	\
	tenuUart2TxMuxOpt

@brief
	UART2 supported TX gpio mux

*/
typedef enum {
	UART2_TX_SPI_RXD = SD_DAT1_SPI_RXD,
	UART2_TX_GPIO6 = GPIO_6,
} tenuUart2TxMuxOpt;
/*!
@enum	\
	tenuUart2RxMuxOpt

@brief
	UART2 supported RX gpio mux

*/
typedef enum {
	UART2_RX_SPI_TXD = SD_DAT0_SPI_TXD,
	UART2_RX_GPIO4 = GPIO_4,
} tenuUart2RxMuxOpt;
/*!
@enum	\
	tenuUart2RtsMuxOpt

@brief
	UART2 supported RTS gpio mux

*/
typedef enum {
	UART2_RTS_SPI_SSN = SD_DAT2_SPI_SSN,
	UART2_RTS_GPIO3 = GPIO_3,
} tenuUart2RtsMuxOpt;
/*!
@enum	\
	tenuUart2CtsMuxOpt

@brief
	UART2 supported CTS gpio mux

*/
typedef enum {
	UART2_CTS_SPI_SCK = SD_CMD_SPI_SCK,
	UART2_CTS_GPIO5 = GPIO_5,
} tenuUart2CtsMuxOpt;
/*!
@struct	\
	tstrUartConfig

@brief
	uart config parm

*/
typedef struct{
	uint8 u8TXGpioPin;
	uint8 u8RxGpioPin;
	uint8 u8RtsGpioPin;
	/**
	RTS only supported on uart2
	*/
	uint8 u8CtsGpioPin;
		/**
	CTS only supported on uart2
	*/
	uint8 u8EnFlowctrl;
	/**
	FLOW CTRL supported on uart2
	*/
	uint32 u32BaudRate;
}tstrUartConfig;

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


sint8 nm_uart_init(enUartCom enCom,tstrUartConfig *strUartconfig);
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
sint8 nm_uart_register_tx_isr(enUartCom enCom,tpfNmBspIsr pfIsr);
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
sint8 nm_uart_register_rx_isr(enUartCom enCom,tpfNmBspIsr pfIsr);
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
sint8 nm_uart_send(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz);
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
sint8 nm_uart_recv(enUartCom enCom,uint8 *pu8Buf, uint16 u16Sz);
/*!
*  @fn			sint8 nm_uart_flush(enUartCom enCom)
*  @brief		flush all the UART Buffer
*  @param[in]	enCom: UART COM port
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_uart_flush(enUartCom enCom);

#endif /*__NMI_UART_H__*/

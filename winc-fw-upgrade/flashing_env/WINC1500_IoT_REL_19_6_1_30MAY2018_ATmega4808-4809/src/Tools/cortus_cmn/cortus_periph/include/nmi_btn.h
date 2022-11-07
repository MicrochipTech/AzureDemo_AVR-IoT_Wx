/**
*  @file		nmi_btn.h
*  @brief		This module contains NMC1000 Win32 bsp APIs declarations 
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0	
*/
#ifndef __NMI_BTN_H__
#define __NMI_BTN_H__

/**
 * INCLUDE
 */
//#include "m2m_fw_iface.h"
#include "common/include/nm_common.h"
/**
 * MACROS
 */

#define SW1		1
#define SW2		2

#define LONG_PRESS 1		/*!<return type Long btn press */
#define SHORT_PRESS 0		/*!<return type short btn press*/
/**
 * STRUCTURE
 */
/*!
*  @struct 		tpfNmBspBtnPress
*  @brief		call back function for btn intilization
*  @details		it contain {u8Btn: btn number{SW1,SW2},u8Type:{LONG_PRESS,SHORT_PRESS}}
*  @sa			nm_btn_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
typedef void (*tpfNmBspBtnPress)(uint8 u8Btn, uint8 u8Type);

/*!
*  @fn			void nm_btn_init(tpfNmBspBtnPress pfBtnCb);
*  @brief
*  @details		Execute function test 1 in details
*  @pfBtnCb[in]	pointer to btn call back
*  @return		NM_SUCESES if succeed, else fail.
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_btn_init(tpfNmBspBtnPress pfBtnCb);
/*!
*  @fn			void nm_btn_deinit(void);
*  @details		Deinit btn module
*  @return		None
*  @sa			nm_btn_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
void nm_btn_deinit(void);

#endif	/*__NM_BSP_APS3_CORTUS_H__ */

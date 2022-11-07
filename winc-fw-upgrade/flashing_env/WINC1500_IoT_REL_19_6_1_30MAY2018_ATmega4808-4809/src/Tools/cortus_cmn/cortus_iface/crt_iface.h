/**
 *  @file		m2m_fw_iface.h
 *  @brief		This module contains declarations of all needed APIs from M2M FW
 *  @author		M. Abdelmawla
 *  @date		24 MAY 2013
 *  @version		1.0
 */
#ifndef _M2M_FW_IFACE_H_
#define _M2M_FW_IFACE_H_


#include "crt_fw_cmn.h"


extern tstrFirmLibOut gstrFirmLibOut;

/************************************
 * 		M2M Related APIs			*
 ***********************************/
#define app_m2m_alloc_spacket				gstrFirmLibOut.app_m2m_alloc_spacket
#define	app_m2m_alloc_cpacket 				gstrFirmLibOut.app_m2m_alloc_cpacket
#define app_m2m_free_packet					gstrFirmLibOut.app_m2m_free_packet
#define app_get_num_free_packet_buffers 	gstrFirmLibOut.app_get_num_free_packet_buffers
/************************************
 * 		CLIB Related APIs			*
 ***********************************/
#define app_printf							gstrFirmLibOut.app_printf
#define app_logs_ctrl						gstrFirmLibOut.app_logs_ctrl
/************************************
 * 		OS Related APIs				*
 ***********************************/
#define app_os_sch_task_create 				gstrFirmLibOut.app_os_sch_task_create
#define app_os_sch_task_sleep				gstrFirmLibOut.app_os_sch_task_sleep
#define app_os_sem_init						gstrFirmLibOut.app_os_sem_init
#define app_os_sem_up						gstrFirmLibOut.app_os_sem_up
#define app_os_sem_down						gstrFirmLibOut.app_os_sem_down
#define app_os_timer_start					gstrFirmLibOut.app_os_timer_start

#define app_os_timer_stop 					gstrFirmLibOut.app_os_timer_stop
#define app_os_interrupt_register 			gstrFirmLibOut.app_os_interrupt_register
#define app_os_timer_get_time			gstrFirmLibOut.app_os_timer_get_time
#define app_os_sch_get_clk_src			gstrFirmLibOut.app_os_sch_get_clk_src

/************************************
 * 		SPI Flash Related APIs		*
 ***********************************/
#define app_spi_flash_read					gstrFirmLibOut.app_spi_flash_read
#define app_spi_flash_write					gstrFirmLibOut.app_spi_flash_write
#define app_spi_flash_sector_erase			gstrFirmLibOut.app_spi_flash_sector_erase
#define app_spi_flash_erase					gstrFirmLibOut.app_spi_flash_erase
#define app_spi_flash_rdid					gstrFirmLibOut.app_spi_flash_rdid
#define app_spi_flash_get_size_inbyte		gstrFirmLibOut.app_spi_flash_get_size_inbyte
/************************************
 * 		Request  Related APIs		*
 ***********************************/
#define app_post_req						gstrFirmLibOut.app_post_req

/************************************
 * 		critical_section  Related APIs*
 ***********************************/

#define app_critical_section_start			gstrFirmLibOut.app_critical_section_start
#define app_critical_section_end			gstrFirmLibOut.app_critical_section_end


#define CONF_WINC_PRINTF(args...) 	app_printf(LOGS_APP,0,args)
#define CONF_WINC_DEBUG				1

#define NM_BSP_PRINTF			CONF_WINC_PRINTF

#endif /*_M2M_FW_IFACE_H_*/


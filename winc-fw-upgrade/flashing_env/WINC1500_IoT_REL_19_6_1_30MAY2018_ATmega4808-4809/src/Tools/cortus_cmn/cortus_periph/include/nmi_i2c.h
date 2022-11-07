/**
*  @file		nmi_i2c.h
*  @brief		This module contains i2c APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifndef __NMI_I2C_H__
#define __NMI_I2C_H__

#include "nmi_common.h"
/*!
*	@fn		nm_i2c_init
*	@brief	Initialize i2c
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M.S.M
*/
sint8 nm_i2c_init(uint8 u8I2cpage);
/*!
*	@fn		nm_i2c_deinit
*	@brief	deInitialize I2C
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_i2c_deinit(void);
/*!
*
*	@fn		nm_eeprom_read
*	@brief	Read from i2c flash
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_eeprom_write(uint32 u32Addr, uint8 *pu8buf,uint32 u32sz);
/*!
*	@fn		nm_eeprom_write
*	@brief	Write in i2c flash
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
uint8 nm_eeprom_read(uint32 u32Addr, uint8 *pu8buf,uint16 u16Sz);
#endif /*__NMI_I2C_H__*/

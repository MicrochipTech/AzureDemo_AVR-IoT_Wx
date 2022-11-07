/**
*  @file		nmi_spi.h
*  @brief		This module contains spi APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifndef __NMI_SPI_H__
#define __NMI_SPI_H__
/**
 * INCLUDE
 */
#include "nmi_common.h"

/**
 *
 * MACROS
 */

/*!Clock phase & polarity settings. */
#define CPHA_0_CPOL_0 0
#define CPHA_0_CPOL_1 1
#define CPHA_1_CPOL_0 2
#define CPHA_1_CPOL_1 3
/*!
*  @fn			sint8 nm_spi_init(uint32 u32Rate,uint8 u8Chpa_cpol);
*  @brief		initialize Spi interface
*  @param[in]	u8Rate: 1,2 spi rate
*  @param[in]	u8Chpa_cpol: Clock phase & polarity settings {CPHA_0_CPOL_0,..}
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/

sint8 nm_spi_init(uint32 u32Rate,uint8 u8Chpa_cpol);
/*!
*  @fn			 nm_spi_deinit
*  @brief		deinitialize Spi interface
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/

void nm_spi_deinit(void);
/*!
*  @brief		Spi flash read
*  @param[out]	pu8buff : Buffer pointer
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure.
*  @sa			nm_spi_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_spi_read(uint8 *pu8buff,uint16 u16sz);
/*!
*  @brief		SPI Flash write
*  @param[in]	pu8buff : Buffer pointer
*  @param[in]	u16sz : buffer size
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure.
*  @sa			nm_spi_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_spi_write(uint8 *pu8buff,uint16 u16sz);

#endif /*__NMI_SPI_H__*/

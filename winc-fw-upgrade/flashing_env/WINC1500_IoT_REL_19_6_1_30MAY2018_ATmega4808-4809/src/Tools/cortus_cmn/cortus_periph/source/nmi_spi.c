

/**
*  @file		nmi_spi.c
*  @brief		This module contains spi APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifdef NMI_USE_SPI
/**
 * INCLUDE
 */
#include "nmi_spi.h"
/**
 * STRUCTURE
 */
typedef struct
{
	/*0x0*/unsigned spi_ctrl;
	/*0x4*/unsigned spi_mstslv_dma_addr;
	/*0x8*/unsigned spi_mstslv_dma_count;
	/*0xc*/unsigned spi_slv_dma_addr;
	/*0x10*/unsigned spi_slv_dma_count;
	/*0x14*/unsigned spi_mst_fcw;
	/*0x18*/unsigned spi_mst_protocol_cmd;
	/*0x1c*/unsigned spi_mst_protocol_addr;
	/*0x20*/unsigned spi_tx_mode;
	/*0x24*/unsigned spi_protocol_config;
	/*0x28*/unsigned spi_protocol_timing;
	/*0x2c*/unsigned spi_intr_ctrl;
	/*0x30*/unsigned spi_intr_status;
	/*0x34*/unsigned spi_mst_timing_ctrl;
	/*0x38*/unsigned spi_mstslv_data_start_ctrl;
	/*0x3c*/unsigned __fill0x3c;
	/*0x40*/unsigned spi_dma_addr_ctrl1;
	/*0x44*/unsigned spi_dma_addr_ctrl2;
	/*0x48*/unsigned spi_misc_ctrl;
} spi_t;
/**
 * MACROS
 */

#define SPI_MST_BASE			0x3000E800UL
#define SPI_MAX_RATE			(80*1000UL) /*KHZ*/

#define MEM_IRAM_BASE_COR	0x80000000ul
#define MEM_IRAM_BASE_AHB	0x60000ul
#define MEM_IRAM_SZ			(1024ul * 128)
#define MEM_IRAM_END_AHB	(MEM_IRAM_BASE_AHB+MEM_IRAM_SZ)
#define MEM_IRAM_END_COR	(MEM_IRAM_BASE_COR+MEM_IRAM_SZ)

#define MEM_DRAM_BASE_COR	0x0ul
#define MEM_DRAM_BASE_AHB	0x30000ul
#define MEM_DRAM_SZ			(1024ul * 64)
#define MEM_DRAM_END_AHB	(MEM_DRAM_BASE_AHB+MEM_DRAM_SZ)
#define MEM_DRAM_END_COR	(MEM_DRAM_BASE_COR+MEM_DRAM_SZ)

#define MEM_SPKT_BASE_COR	0x60000000ul
#define MEM_SPKT_BASE_AHB	0xd0000ul
#define MEM_SPKT_SZ			(1024ul * 128)
#define MEM_SPKT_END_AHB	(MEM_SPKT_BASE_AHB+MEM_SPKT_SZ)
#define MEM_SPKT_END_COR	(MEM_SPKT_BASE_COR+MEM_SPKT_SZ)
/**
 *
 *
 */
uint32 gu32Rate = 0; /*KHZ*/
/**
 *
 *
 *
 */

static unsigned long get_ahb_adr_from_cor_adr(unsigned long adr)
{
	if ((adr >= MEM_SPKT_BASE_COR) && (adr < MEM_SPKT_END_COR)) /* packet memory */
	{
		adr = (adr - MEM_SPKT_BASE_COR) + MEM_SPKT_BASE_AHB;
	} else if ((adr >= MEM_DRAM_BASE_COR) && (adr < MEM_DRAM_END_COR)) /* DRAM */
	{
		adr = (adr - MEM_DRAM_BASE_COR ) + MEM_DRAM_BASE_AHB;
	} else if ((adr >= MEM_IRAM_BASE_COR) && (adr < MEM_IRAM_END_COR)) /* IRAM */
	{
		adr = (adr - MEM_IRAM_BASE_COR ) + MEM_IRAM_BASE_AHB;
	} else {
		//adr += 0x30000000;
	}
	return adr;
}
static void udelay(uint32 udelay)
{
	/*if count = 0xFFF time will be ~1 msec*/
    uint32 i = 0;
    volatile uint32 j = 0,count  = 0;
    count = ((udelay*4095)/1000) + 10;
    /*need to be tested using logic scope*/

    for(i = 0; i < count; i++)
       j += count;
}
/*!
*  @brief		Spi flash read
*  @param[out]	pu8buff : Buffer pointer
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure.
*  @sa			nm_spi_init
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/
sint8 nm_spi_read(uint8 *pu8buff,uint16 u16sz)
{
	sint8 ret = NM_SUCCESS;
	volatile spi_t *m = (volatile spi_t *) (SPI_MST_BASE);

	if((pu8buff != NULL) &&(u16sz!=0))
	{
		/* Set DMA address */
		m->spi_mstslv_dma_addr = get_ahb_adr_from_cor_adr((unsigned long)pu8buff);
		/* Set DMA count */
		m->spi_mstslv_dma_count = u16sz;
		/* Start */
		m->spi_tx_mode |= 2;/*Receive*/
		/* Wait for DMA done */
		while((m->spi_tx_mode & 0x1) != 0);
		udelay(((11 * 1000 * 1000)/ (gu32Rate * 1000))+1);
	}
	else
	{
		ret = NM_SPI_FAIL;
	}
	return ret;

}
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

sint8 nm_spi_write(uint8 *pu8buff,uint16 u16sz)
{
	sint8 ret = NM_SUCCESS;
	volatile spi_t *m = (volatile spi_t *) (SPI_MST_BASE);
	if ((pu8buff != NULL) && (u16sz != 0))
	{
		/* Set DMA address to shared packet memory address */
		m->spi_mstslv_dma_addr = get_ahb_adr_from_cor_adr((unsigned long)pu8buff);
		/* Set DMA count */
		m->spi_mstslv_dma_count = u16sz;
		/* Start */
		m->spi_tx_mode |= 1;/*send*/
		/* Wait for DMA done */
		while ((m->spi_tx_mode & 0x1) != 0);
		udelay(((11 * 1000 * 1000)/ (gu32Rate * 1000))+1);

	}
	else
	{
		ret = NM_SPI_FAIL;
	}
	return ret;

}

/*!
*  @fn			sint8 nm_spi_init(uint8 u8Rate,uint8 u8Chpa_cpol);
*  @brief		initialize Spi interface
*  @param[in]	u8Rate: 1000,2000 spi rate in khz
*  @param[in]	u8Chpa_cpol: Clock phase & polarity settings {CPHA_0_CPOL_0,..}
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/

sint8 nm_spi_init(uint32 u32Rate,uint8 u8Chpa_cpol)
{
	sint8 ret = NM_SUCCESS;
	volatile spi_t *m = (volatile spi_t *) (SPI_MST_BASE);
	/* Set the divisor */
	/*sys_clk*(mst_fcw_reg+1)/1024==rate(MHZ)*/
	if(u32Rate>SPI_MAX_RATE)
	{
		ret = NM_SPI_FAIL;
		goto ERR;
	}
	m->spi_mst_fcw =/*0x20;*/((u32Rate*1024UL)/SPI_MAX_RATE)-1;
	/* Set SPI mode  */
	m->spi_ctrl |= 0x2ul;
	m->spi_ctrl |=(1<<15);
	m->spi_ctrl &=~(3<<3);
	m->spi_ctrl |=(u8Chpa_cpol<<3);

	/*enable spi*/
	m->spi_ctrl |= 1;
	/* Disable protocol mode and use generic mode */
	m->spi_protocol_config &= ~(0x1ul);
	/* Disable protocol mode and use generic mode */
	m->spi_protocol_timing =0;
ERR:
	return ret;
}

/*!
*  @fn			 nm_spi_deinit
*  @brief		deinitialize Spi interface
*  @return		NM_SUCCESS in case of success and NM_SPI_FAIL in case of failure
*  @author		M.S.M
*  @date		19 DEC 2013
*  @version		1.0 Description
*/

void nm_spi_deinit(void)
{

}


#endif

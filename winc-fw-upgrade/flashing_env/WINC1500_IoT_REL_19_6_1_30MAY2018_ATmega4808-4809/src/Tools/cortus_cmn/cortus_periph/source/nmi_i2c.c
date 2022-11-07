/**
*  @file		nmi_i2c.c
*  @brief		This module contains i2c APIs declarations
*  @author		M.S.M
*  @date		17 DEC 2013
*  @version		1.0
*/
#ifdef NMI_USE_I2C

#error "I2C MUXED with spi flash interface, can't be used in that Revision"
/**
 * INCLUDE
 */
#include "nmi_i2c.h"

/**
 *
 * STRUCTURE
 */

typedef struct
{
	/*0x0*/unsigned ic_con;
	/*0x4*/unsigned ic_tar;
	/*0x8*/unsigned ic_sar;
	/*0xc*/unsigned ic_hs_maddr;
	/*0x10*/unsigned ic_data_cmd;
	/*0x14*/unsigned ic_ss_scl_hcnt;
	/*0x18*/unsigned ic_ss_scl_lcnt;
	/*0x1c*/unsigned ic_fs_scl_hcnt;
	/*0x20*/unsigned ic_fs_scl_lcnt;
	/*0x24*/unsigned ic_hs_scl_hcnt;
	/*0x28*/unsigned ic_hs_scl_lcnt;
	/*0x2c*/unsigned ic_intr_stat;
	/*0x30*/unsigned ic_intr_mask;
	/*0x34*/unsigned ic_raw_intr_stat;
	/*0x38*/unsigned ic_rx_tl;
	/*0x3c*/unsigned ic_tx_tl;
	/*0x40*/unsigned ic_clr_intr;
	/*0x44*/unsigned ic_clr_rx_under;
	/*0x48*/unsigned ic_clr_rx_over;
	/*0x4c*/unsigned ic_clr_tx_over;
	/*0x50*/unsigned ic_clr_rd_req;
	/*0x54*/unsigned ic_clr_tx_abrt;
	/*0x58*/unsigned ic_clr_rx_done;
	/*0x5c*/unsigned ic_clr_activity;
	/*0x60*/unsigned ic_clr_stop_det;
	/*0x64*/unsigned ic_clr_start_det;
	/*0x68*/unsigned ic_clr_gen_call;
	/*0x6c*/unsigned ic_enable;
	/*0x70*/unsigned ic_status;
	/*0x74*/unsigned ic_txflr;
	/*0x78*/unsigned ic_rxflr;
	/*0x7c*/unsigned ic_sda_hold;
	/*0x80*/unsigned ic_tx_abrt_source;
} i2c_t;

/**
 *
 * MACROS
 */
#define WIFI_I2C (WIFI_PERIPH_BASE + 0x2200) 	/*I2C address*/
#define I2C_ADD (0xA0 >> 1)
#define PAGE_SIZE (256)							/*I2C pageing*/
#define BSP_MIN(x,y) ((x)>(y)?(y):(x))
/*I2C errors*/
#define I2C_SUCCESS 			  ((sint8)0)
#define ERR_I2C_LARGE_ADDRESS 	  ((sint8)-1)
#define ERR_I2C_TX_ABRT 		  ((sint8)-2)
#define ERR_I2C_OVER_SIZE 		  ((sint8)-3)

/***
 *
 *GLOBAL VAR
 *
 */
static uint8 u8Page = 0;
/***
 *
 *STATIC FUNCTIONS
 *
 */
static void delay(unsigned long del)
{
	unsigned long i, j;
	volatile uint32 val;
	for (i = 0; i < del; i++)
		for (j = 0; j < del; j++)
		{
			val = *((volatile unsigned *) 0x3000108c);
		}
}
static void check_tx_over(volatile i2c_t *x)
{
	volatile uint32 clr;
	while (((x->ic_raw_intr_stat) & 0x008))/*check tx over*/
	{
		clr = x->ic_clr_tx_over;
		delay(10);
		clr = x->ic_clr_tx_over;
	}
	while (!((x->ic_raw_intr_stat) & 0x010));/*tx empty*/
}
static void check_bus(volatile i2c_t *x)
{
	uint32 stat;
	do
	{
		stat = x->ic_status;
		if ((((stat >> 5) & 0x1) == 0) && (((stat >> 2) & 0x1) == 1))/*mst active tx completely empty*/
			break;
	} while (1);
}
static void check_stop_detect(volatile i2c_t *x)
{
	volatile uint32 clr;
	while (!((x->ic_intr_stat) & 0x200))
		;
	clr = x->ic_clr_stop_det;
}
static uint8 check_tx_abrt(volatile i2c_t *x)
{
	uint32 stat;
	stat = x->ic_raw_intr_stat;
	if ((stat >> 6) & 0x1)
	{ /* tx abort */
		return 1;
	}
	return 0;
}
static sint8 eeprom_write_PP(uint32 u32Addr, uint8 *pu8buff, uint32 u32Sz)
{

	volatile i2c_t *m = (volatile i2c_t *) (WIFI_I2C);
	/*write must be =< 256byte page size*/
	if (u32Sz > PAGE_SIZE)
	{
		return ERR_I2C_OVER_SIZE;
	}
	/*4 memory slot 00 01 10 11*/
	if ((u32Addr >> 16) > 0x3)
	{
		//*((volatile unsigned *) 0x3000108c) = 0xee;
		return ERR_I2C_LARGE_ADDRESS;
	}
	/*re-init to change i2c target address*/
	if (u8Page != (u32Addr >> 16))
	{
		nm_i2c_init((u32Addr >> 16));
	}
	/*check fifo empty and master idle */
	check_bus(m);
	m->ic_data_cmd = ((u32Addr & 0xff00) >> 8);
	m->ic_data_cmd = (u32Addr & 0xff);
	/*check No ACk ,tx errors */
	if (check_tx_abrt(m))
	{
		return ERR_I2C_TX_ABRT;
	}
	/*check buffer not full "blocking function till first slot empty"*/
	check_tx_over(m);
	while (u32Sz > 0)
	{
		m->ic_data_cmd = *pu8buff; /* Restart, Read */
		pu8buff++;
		u32Sz--;
		if (check_tx_abrt(m))
		{
			return ERR_I2C_TX_ABRT;
		}
		check_tx_over(m);
	}
	/*check stop detected "blocking function till detect"*/
	check_stop_detect(m);
	/*delay needed after write operation */
	/*if removed the i2c flash don't respond */
	delay(1000);
	return I2C_SUCCESS;

}
/*
*	@fn		nm_i2c_init
*	@brief	Initialize i2c
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author	M.S.M
*/

sint8 nm_i2c_init(uint8 u8I2cpage)
{
	volatile unsigned *pin = (volatile unsigned *) (WIFI_PINMUX_SEL_0);
	volatile i2c_t *m = (volatile i2c_t *) (WIFI_I2C);
	/**
	 i2c pin mux
	 **/
	*pin &= ~((0x7 << 16) | (0x7 << 24));
	*pin |= (0x2 << 16) | (0x2 << 24);

	/**
	 disable i2c
	 **/
	m->ic_enable = 0;
	/**
	 speed
	 **/
	m->ic_fs_scl_hcnt = 0x8a;/*400khz*/
	m->ic_fs_scl_lcnt = 0x96;
	/**
	 con
	 **/
	m->ic_con = 0x65;
	/*1100101*/
	/* disable slave mode */
	/* re-start */
	/* 7-bits address */
	/* 400k */
	/* master enable */

	/**
	 tar
	 **/
	m->ic_tar = (I2C_ADD | (u8I2cpage & 0x3));
	u8Page = u8I2cpage;

	/**
	 mask all
	 **/
	m->ic_intr_mask = 0xaff;/*stop det &tx over&rx full*/

	/**
	 set threshold
	 **/
	m->ic_rx_tl = 7;
	m->ic_tx_tl = 7;
	/**
	 enable i2c
	 **/
	m->ic_enable = 1;

	return I2C_SUCCESS;
}
/*
*	@fn		nm_i2c_deinit
*	@brief	deInitialize I2C
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_i2c_deinit(void)
{
	//volatile unsigned *pin = (volatile unsigned *) (WIFI_PIN_MUX0);
	volatile i2c_t *m = (volatile i2c_t *) (WIFI_I2C);

	/**
	 enable i2c
	 **/
	m->ic_enable = 0;
	return I2C_SUCCESS;
}
/*
*	@fn		nm_eeprom_read
*	@brief	Read from i2c flash
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
uint8 nm_eeprom_read(uint32 u32Addr, uint8 *pu8buf,uint16 u16Sz)
{
	volatile i2c_t *m = (volatile i2c_t *) (WIFI_I2C);
	uint32 stat;
	uint32 sizetx, sizerx;
	volatile uint32 clr;
	sizetx = u16Sz;
	sizerx = sizetx;
	if ((u32Addr >> 16) > 0x3)
	{
		return I2C_ERR_LARGE_ADDRESS;
	}
	if (u8Page != (u32Addr >> 16))
	{
		nm_i2c_init((u32Addr >> 16));
	}
	check_bus(m);
	m->ic_data_cmd = ((u32Addr & 0xff00) >> 8);
	m->ic_data_cmd = (u32Addr & 0xff);
	m->ic_data_cmd = 0x100; /* Restart, Read */
	/*check for ack received and tx errors*/
	if (check_tx_abrt(m))
	{
		return ERR_I2C_TX_ABRT;
	}
	sizetx--;
	do
	{
		stat = m->ic_status;
		if ((stat >> 3) & 0x1)
		{ /* RFNE */
			*pu8buf = m->ic_data_cmd;
			pu8buf++;
			sizerx--;
			if (sizerx == 0)
				break;
		}
		if (sizetx != 0)
		{
			if (((m->ic_raw_intr_stat) & 0x010)
					&& (!((m->ic_raw_intr_stat) & 0x008)))/*not full empty*/
			{
				m->ic_data_cmd = 0x100; /* Restart, Read */
				sizetx--;
			}
			else
			{
				/*clear Tx over*/
				clr = m->ic_clr_tx_over;
			}
		}
		/*check for ack received and tx errors*/
		if (check_tx_abrt(m))
		{
			return ERR_I2C_TX_ABRT;
		}
	} while (1);
	/*check stop detect "blocking fun"*/
	check_stop_detect(m);

	return I2C_SUCCESS;
}
/*
*	@fn		nm_eeprom_write
*	@brief	Write in i2c flash
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_eeprom_write(uint32 u32Addr, uint8 *pu8buf,uint32 u32Sz)
{
	uint32  u32wsz;
	uint32  u32off;
	sint8 ret= I2C_SUCCESS;

	u32off = u32Addr % PAGE_SIZE;
	if (u32off)/*first part of data in the address page*/
	{
		u32wsz = PAGE_SIZE - u32off;
		ret = eeprom_write_PP(u32Addr, pu8buf, BSP_MIN(u32Sz, u32wsz));
		if (ret != I2C_SUCCESS)
		{
			return ret;
		}
		if (u32Sz < u32wsz) return ret;
		/*return data less than the remaining bytes in page*/
		pu8buf += u32wsz;
		u32Addr += u32wsz;
		u32Sz -= u32wsz;
	}
	do
	{
		u32wsz = BSP_MIN(u32Sz, PAGE_SIZE);
		/*write complete page or the remaining data*/
		ret = eeprom_write_PP(u32Addr, pu8buf, u32wsz);
		if (ret != I2C_SUCCESS)
		{
			return ret;
		}
		pu8buf += u32wsz;
		u32Addr += u32wsz;
		u32Sz -= u32wsz;
	} while (u32Sz > 0);
	return ret;
}
#endif


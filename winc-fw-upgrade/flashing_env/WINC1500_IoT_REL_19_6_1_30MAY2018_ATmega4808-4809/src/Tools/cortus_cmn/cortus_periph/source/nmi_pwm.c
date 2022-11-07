#include "nmi_pwm.h"
#include "driver\source\nmbus.h"


void pwm_init(uint8 clkdiv)
{
	uint32  reg = 0;
	/**
	 * You must also select the PWM as the output.  Do this by writing the following register bit
		0x107C[3] = 1
	 *
	 */
	reg = nm_read_reg(0x107c);
	reg |= NBIT3;
	nm_write_reg(0x107c,reg);

	/**
	 * GPIO_4  0x1408[18:16] = 0x1
	 */
	reg = nm_read_reg(0x1408);
	reg &= ~(0x7UL<<16);
	reg |= (1UL<<16);
	nm_write_reg(0x1408,reg);

	/*
	 * The PWM clock must also be enabled.  Do this by writing the following register bit
		0x1008[1] = 1
	 *
	 */
	reg = nm_read_reg(0x1008);
	reg |= NBIT1;
	nm_write_reg(0x1008,reg);
	/*
	 *
	 *  pwm_en[0]		 			PWM enable bit - 1 to activate PWM function
		output_polarity[1]	 		1 to inverse the polarity
		agcdata_fmt[2]	 			AGC data format
		sample_method[3]	 		0 - Samples agcdata at >= 1024 cycles and does not lost precision.
									1 - Samples at PWM period but will lose LSBs if < 1024
		use_old_pwm[4]	 			0 - Use new PWM
									1 - Use old PWM
		pwm_period[8:5]	 		    programmable PWM update period
		agcdata_in[18:9]			agc value from AGC
		use_agcupdate[19]	 		Use agcupdate
		agcupdate[20]	 	    	agcupdate
	 *
	 */
	reg = nm_read_reg(0x1064);
	reg |= NBIT0;
	reg |= (clkdiv<<5);
	nm_write_reg(0x1064,reg);

}

void pwm_set_duty_cycle(uint32 val)
{
	uint32 reg = 0;
	if(val > 1022) return;


	if(val >= 511)
	{
		reg = nm_read_reg(0x1064);
		reg &= ~ (0x3FF << 9);
		reg |= ((val - 511) << 9);
		reg |= NBIT1;
		nm_write_reg(0x1064,reg);
	}
	else
	{
		reg = nm_read_reg(0x1064);
		reg &= ~ (0x3FF << 9);
		reg |= ((511 - val) << 9);
		reg &= ~NBIT1;
		nm_write_reg(0x1064,reg);
	}

}

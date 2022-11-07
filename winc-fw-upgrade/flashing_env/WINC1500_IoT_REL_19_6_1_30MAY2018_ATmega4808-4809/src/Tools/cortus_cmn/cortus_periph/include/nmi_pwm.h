#ifndef _PWM_H_
#define _PWM_H_
#include "nmi_common.h"


void pwm_init(uint8 clkdiv);
void pwm_set_duty_cycle(uint32 val);


#endif /* _EFUSE_H_ */

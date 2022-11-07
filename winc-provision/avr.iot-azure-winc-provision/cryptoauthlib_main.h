/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#ifndef CRYPTOAUTHLIB_MAIN_H
#define CRYPTOAUTHLIB_MAIN_H

#include <stddef.h>
#include "cryptoauthlib/lib/atca_device.h"
#include "cryptoauthlib/lib/basic/atca_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initialize Cryptoauthlib Stack
 */
void cryptoauthlib_init(void);
extern ATCAIfaceCfg cfg_ateccx08a_i2c_custom;
extern uint8_t cryptoDeviceInitialized;

#ifdef __cplusplus
}
#endif

#endif /* CRYPTOAUTHLIB_MAIN_H */
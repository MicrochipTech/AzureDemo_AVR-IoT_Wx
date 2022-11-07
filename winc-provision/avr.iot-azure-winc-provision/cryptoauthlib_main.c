/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */

#include "cryptoauthlib_main.h"
#include <atmel_start.h>

struct atca_command  _gmyCommand;
struct atca_iface    _gmyIface;
struct atca_device   _gMyDevice = {&_gmyCommand, &_gmyIface};

ATCAIfaceCfg cfg_ateccx08a_i2c_custom = {
    .iface_type             = ATCA_I2C_IFACE,
    .devtype                = ATECC608A,
    .atcai2c.slave_address  = 0xB0,
    .atcai2c.bus            = 2,
    .atcai2c.baud           = 100000,
    .wake_delay             = 1500,
    .rx_retries             = 20
};

uint8_t cryptoDeviceInitialized = true;

void cryptoauthlib_init(void)
{
    uint8_t rv;
    
    atcab_init_device(&_gMyDevice);
    
    rv = atcab_init(&cfg_ateccx08a_i2c_custom);
    if (rv != ATCA_SUCCESS)
    {
        cryptoDeviceInitialized = false;
    }
    
    //atcab_lock_data_slot(0);
}

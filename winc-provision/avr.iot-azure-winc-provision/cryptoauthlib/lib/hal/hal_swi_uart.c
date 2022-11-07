/**
 * \file
 * \brief ATCA Hardware abstraction layer for SWI over UART drivers.
 *
 * \copyright (c) 2017 Microchip Technology Inc. and its subsidiaries.
 *            You may use this software and any derivatives exclusively with
 *            Microchip products.
 *
 * \page License
 *
 * (c) 2017 Microchip Technology Inc. and its subsidiaries. You may use this
 * software and any derivatives exclusively with Microchip products.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIPS TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
 * TERMS.
 */

#include <string.h>
#include <stdio.h>
#include "atca_hal.h"
#include "hal_swi_uart.h"
#include "atca_device.h"
#include "atca_execution.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 * using SWI Interface.
 *
   @{ */

/** \brief logical to physical bus mapping structure */
ATCASWIMaster_t *swi_hal_data[MAX_SWI_BUSES];   // map logical, 0-based bus number to index
int swi_bus_ref_ct = 0;                         // total in-use count across buses

#ifdef DEBUG_HAL
static void print_array(uint8_t *data, uint32_t data_size)
{
    //  printf("%.4x\r\n", data_size);

    uint32_t n;

    for (n = 0; n < data_size; n++)
    {
        printf("%.2x ", data[n]);
        if (((n + 1) % 16) == 0)
        {
            printf("\r\n");
            if ((n + 1) != data_size)
            {
                printf("         ");
            }
        }
    }
    if (data_size % 16 != 0)
    {
        printf("\r\n");
    }
}
#endif


/** \brief discover swi buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] swi_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 * \return ATCA_SUCCESS
 */

ATCA_STATUS hal_swi_discover_buses(int swi_buses[], int max_buses)
{
    swi_uart_discover_buses(swi_buses, max_buses);

    return ATCA_SUCCESS;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in] bus_num - logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg[] - pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] *found - number of devices found on this bus
 * \return ATCA_SUCCESS
 */

ATCA_STATUS hal_swi_discover_devices(int bus_num, ATCAIfaceCfg cfg[], int *found)
{
    ATCAIfaceCfg *head = cfg;
    ATCADevice device;

#ifdef ATCA_INTERFACE_V2
    struct atca_device disc_device;
    struct atca_command disc_command;
    struct atca_iface disc_iface;
#endif
    ATCAPacket packet;
    ATCA_STATUS status;
    uint8_t revs608[][4] = { { 0x00, 0x00, 0x60, 0x01 }, { 0x00, 0x00, 0x60, 0x02 } };
    uint8_t revs508[][4] = { { 0x00, 0x00, 0x50, 0x00 } };
    uint8_t revs108[][4] = { { 0x80, 0x00, 0x10, 0x01 } };
    uint8_t revs204[][4] = { { 0x00, 0x02, 0x00, 0x08 }, { 0x00, 0x02, 0x00, 0x09 }, { 0x00, 0x04, 0x05, 0x00 } };
    unsigned int i;

    /** \brief default configuration, to be reused during discovery process */
    ATCAIfaceCfg discoverCfg = {
        .iface_type     = ATCA_SWI_IFACE,
        .devtype        = ATECC508A,
        .atcaswi.bus    = bus_num,
        .wake_delay     = 800,
        .rx_retries     = 3
    };

    ATCAHAL_t hal;

    if (bus_num < 0)
    {
        return ATCA_COMM_FAIL;
    }

    hal_swi_init(&hal, &discoverCfg);
#ifdef ATCA_INTERFACE_V2
    device = newATCADevice_v2(&discoverCfg, &disc_device, &disc_command, &disc_iface);
#else
    device = newATCADevice(&discoverCfg);
#endif


    memset(packet.data, 0x00, sizeof(packet.data));

    // build an info command
    packet.param1 = INFO_MODE_REVISION;
    packet.param2 = 0;
    atInfo(device->mCommands, &packet);
    if ((status = atca_execute_command(&packet, device)) != ATCA_SUCCESS)
    {
        return status;
    }

    // determine device type from common info and dev rev response byte strings... start with unknown
    discoverCfg.devtype = ATCA_DEV_UNKNOWN;

    for (i = 0; i < (int)sizeof(revs608) / 4; i++)
    {
        if (memcmp(&packet.data[1], &revs608[i], 4) == 0)
        {
            discoverCfg.devtype = ATECC608A;
            break;
        }
    }

    for (i = 0; i < sizeof(revs508) / 4; i++)
    {
        if (memcmp(&packet.data[1], &revs508[i], 4) == 0)
        {
            discoverCfg.devtype = ATECC508A;
            break;
        }
    }

    for (i = 0; i < sizeof(revs204) / 4; i++)
    {
        if (memcmp(&packet.data[1], &revs204[i], 4) == 0)
        {
            discoverCfg.devtype = ATSHA204A;
            break;
        }
    }

    for (i = 0; i < sizeof(revs108) / 4; i++)
    {
        if (memcmp(&packet.data[1], &revs108[i], 4) == 0)
        {
            discoverCfg.devtype = ATECC108A;
            break;
        }
    }

    if (discoverCfg.devtype != ATCA_DEV_UNKNOWN)
    {
        // now the device type is known, so update the caller's cfg array element with it
        (*found)++;
        memcpy( (uint8_t*)head, (uint8_t*)&discoverCfg, sizeof(ATCAIfaceCfg));
        head->devtype = discoverCfg.devtype;
        head++;
    }

    atca_delay_ms(15);

    deleteATCADevice(&device);

    return ATCA_SUCCESS;
}

/** \brief hal_swi_init manages requests to initialize a physical interface.  it manages use counts so when an interface
 * has released the physical layer, it will disable the interface for some other use.
 * You can have multiple ATCAIFace instances using the same bus, and you can have multiple ATCAIFace instances on
 * multiple swi buses, so hal_swi_init manages these things and ATCAIFace is abstracted from the physical details.
 */

/** \brief initialize an SWI interface using given config
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_init(void *hal, ATCAIfaceCfg *cfg)
{
    int bus = cfg->atcaswi.bus;   // 0-based logical bus number
    ATCAHAL_t *phal = (ATCAHAL_t*)hal;

    if (swi_bus_ref_ct == 0)       // power up state, no swi buses will have been used
    {
        for (int i = 0; i < MAX_SWI_BUSES; i++)
        {
            swi_hal_data[i] = NULL;
        }
        swi_bus_ref_ct++;  // total across buses become 1
    }

    if (bus >= 0 && bus < MAX_SWI_BUSES)
    {
        // if this is the first time this bus and interface has been created, do the physical work of enabling it
        if (swi_hal_data[bus] == NULL)
        {
            swi_hal_data[bus] = malloc(sizeof(ATCASWIMaster_t) );
            // store this for use during the release phase
            swi_hal_data[bus]->bus_index = bus;
            // initialize  UART module for SWI interface
            swi_uart_init(swi_hal_data[bus]);
        }
        else
        {
            // otherwise, another interface already initialized the bus, any different
            // cfg parameters will be ignored...first one to initialize this sets the configuration
        }
        phal->hal_data = swi_hal_data[bus];

        return ATCA_SUCCESS;
    }
    return ATCA_COMM_FAIL;
}

/** \brief HAL implementation of SWI post init
 * \param[in] iface  instance
 * \return ATCA_SUCCESS
 */

ATCA_STATUS hal_swi_post_init(ATCAIface iface)
{
    return ATCA_SUCCESS;
}

/** \brief HAL implementation of SWI send one byte over UART
 * \param[in] iface  instance
 * \param[in] data   bytes to send
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_send_flag(ATCAIface iface, uint8_t data)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcaswi.bus;
    uint8_t bit_mask, bit_data;

    for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1)
    {
        // Send one byte that represent one bit, 0x7F for one or 0x7D for zero
        // The LSB (least significant bit) is sent first.
        bit_data = (bit_mask & data) ? 0x7F : 0x7D;
        status |= swi_uart_send_byte(swi_hal_data[bus], bit_data);

    }
    if (status != ATCA_SUCCESS)
    {
        return ATCA_COMM_FAIL;
    }
    else
    {
        return ATCA_SUCCESS;
    }
}

/** \brief HAL implementation of SWI send command over UART
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_send(ATCAIface iface, uint8_t *txdata, int txlength)
{

#ifdef DEBUG_HAL
    printf("hal_swi_send()\r\n");

    printf("\r\nCommand Packet (size:0x%.4x)\r\n", txlength);
    printf("Count  : %.2x\r\n", txdata[1]);
    printf("Opcode : %.2x\r\n", txdata[2]);
    printf("Param1 : %.2x\r\n", txdata[3]);
    printf("Param2 : "); print_array(&txdata[4], 2);
    if (txdata[1] > 7)
    {
        printf("Data   : "); print_array(&txdata[6], txdata[1] - 7);
    }
    printf("CRC    : "); print_array(&txdata[txdata[1] - 1], 2);
    printf("\r\n");
#endif
    ATCA_STATUS status = ATCA_SUCCESS;
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcaswi.bus;
    uint8_t i, bit_mask, bit_data;

    //Skip the Word Address data as SWI doesn't use it
    txdata++;

    status = hal_swi_send_flag(iface, SWI_FLAG_CMD);
    if (status == ATCA_SUCCESS)
    {
        for (i = 0; i < txlength; i++)
        {
            for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1)
            {
                // Send one byte that represent one bit, 0x7F for one or 0x7D for zero
                // The LSB (least significant bit) is sent first.
                bit_data = (bit_mask & *txdata) ? 0x7F : 0x7D;
                status = swi_uart_send_byte(swi_hal_data[bus], bit_data);
                if (status != ATCA_SUCCESS)
                {
                    return ATCA_COMM_FAIL;
                }
            }
            txdata++;
        }
        return ATCA_SUCCESS;
    }
    return ATCA_COMM_FAIL;
}

/** \brief HAL implementation of SWI receive function over UART
 * \param[in]    iface     Device to interact with.
 * \param[out]   rxdata    Data received will be returned here.
 * \param[inout] rxlength  As input, the size of the rxdata buffer.
 *                         As output, the number of bytes received.
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_swi_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
#ifdef DEBUG_HAL
    printf("hal_swi_receive()\r\n");
#endif
    ATCA_STATUS status = !ATCA_SUCCESS;
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcaswi.bus;
    int retries = cfg->rx_retries;
    uint8_t bit_mask, *head_buff, bit_data, count;
    uint8_t i = 0;
    uint16_t rxdata_max_size = *rxlength;

    *rxlength = 0;
    if (rxdata_max_size < 1)
    {
        return ATCA_SMALL_BUFFER;
    }

    head_buff = rxdata;

    while (retries-- > 0 && status != ATCA_SUCCESS)
    {
        if ((status = hal_swi_send_flag(iface, SWI_FLAG_TX)) == ATCA_SUCCESS)
        {
            swi_uart_mode(swi_hal_data[bus], RECEIVE_MODE);

            atca_delay_us(RX_DELAY); // Must be configured to sync with response from device
        }

        *head_buff = 0x00;
        for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1)
        {
            bit_data = 0;
            status = swi_uart_receive_byte(swi_hal_data[bus], &bit_data);
            if (status != ATCA_SUCCESS)
            {
                break;
            }
            // Sometimes bit data from device is stretched
            // When the device sends a "one" bit, it is read as 0x7E or 0x7F.
            // When the device sends a "zero" bit, it is read as 0x7A, 0x7B, or 7D.
            if ((bit_data ^ 0x7F) < 2)
            {
                // Received "one" bit.
                *head_buff |= bit_mask;
            }
        }
    }
    if (status != ATCA_SUCCESS)
    {
        // Set SWI to transmit mode.
        swi_uart_mode(swi_hal_data[bus], TRANSMIT_MODE);
        atca_delay_us(TX_DELAY);
        return status;
    }
    if (*head_buff < ATCA_RSP_SIZE_MIN)
    {
        // Set SWI to transmit mode.
        swi_uart_mode(swi_hal_data[bus], TRANSMIT_MODE);
        atca_delay_us(TX_DELAY);
        return ATCA_INVALID_SIZE;
    }
    if (*head_buff > rxdata_max_size)
    {
        // Set SWI to transmit mode.
        swi_uart_mode(swi_hal_data[bus], TRANSMIT_MODE);
        atca_delay_us(TX_DELAY);
        return ATCA_SMALL_BUFFER;
    }

    count = (*head_buff) - 1;
    head_buff++;

    for (i = 0; i < count; i++)
    {
        *head_buff = 0x00;
        for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1)
        {
            bit_data = 0;
            status = swi_uart_receive_byte(swi_hal_data[bus], &bit_data);
            if (status != ATCA_SUCCESS)
            {
                // Set SWI to transmit mode.
                swi_uart_mode(swi_hal_data[bus], TRANSMIT_MODE);
                atca_delay_us(TX_DELAY);
                return status;
            }
            // Sometimes bit data from device is stretched
            // When the device sends a "one" bit, it is read as 0x7E or 0x7F.
            // When the device sends a "zero" bit, it is read as 0x7A, 0x7B, or 7D.
            if ((bit_data ^ 0x7F) < 2)
            {
                // Received "one" bit.
                *head_buff |= bit_mask;
            }
        }
        head_buff++;
    }

    *rxlength = rxdata[0];

    // Set SWI to transmit mode.
    swi_uart_mode(swi_hal_data[bus], TRANSMIT_MODE);
    atca_delay_us(TX_DELAY);

#ifdef DEBUG_HAL
    printf("\r\nResponse Packet (size:0x%.4x)\r\n", *rxlength);
    printf("Count  : %.2x\r\n", rxdata[0]);
    if (rxdata[0] > 3)
    {
        printf("Data   : "); print_array(&rxdata[1], rxdata[0] - 3);
        printf("CRC    : "); print_array(&rxdata[rxdata[0] - 2], 2);
    }
    printf("\r\n");
#endif

    return status;
}


/** \brief wake up CryptoAuth device using SWI interface
 * \param[in] iface  interface to logical device to wakeup
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_wake(ATCAIface iface)
{
#ifdef DEBUG_HAL
    printf("hal_swi_wake()\r\n");
#endif
    ATCA_STATUS status = ATCA_COMM_FAIL;
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcaswi.bus;
    int retries = cfg->rx_retries;
    uint16_t datalength = 4;
    uint8_t data[4] = { 0x00, 0x00, 0x00, 0x00 }, expected[4] = { 0x04, 0x11, 0x33, 0x43 };

    while ((status != ATCA_SUCCESS) && (retries >= 0x00))
    {
        retries--;
        // Change baudrate to 115200 to get low signal more than 60us
        swi_uart_setbaud(swi_hal_data[bus], 115200);
        // Send byte 0x00
        status = swi_uart_send_byte(swi_hal_data[bus], SWI_WAKE_TOKEN);
        // Change baudrate back to 230400
        swi_uart_setbaud(swi_hal_data[bus], 230400);
    }

    if (!status)
    {
        atca_delay_us(cfg->wake_delay);  // wait tWHI + tWLO which is configured based on device type and configuration structure
        status = hal_swi_receive(iface, data, &datalength);
    }

    if ((retries == 0x00) && (status != ATCA_SUCCESS) )
    {
        return ATCA_TIMEOUT;
    }

    if (memcmp(data, expected, 4) == 0)
    {
        return ATCA_SUCCESS;
    }

    return ATCA_COMM_FAIL;
}

/** \brief idle CryptoAuth device using SWI interface
 * \param[in] iface  interface to logical device to idle
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_idle(ATCAIface iface)
{
    return hal_swi_send_flag(iface, SWI_FLAG_IDLE);
}

/** \brief sleep CryptoAuth device using SWI interface
 * \param[in] iface  interface to logical device to sleep
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_sleep(ATCAIface iface)
{
    return hal_swi_send_flag(iface, SWI_FLAG_SLEEP);
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_swi_release(void *hal_data)
{
    ATCASWIMaster_t *hal = (ATCASWIMaster_t*)hal_data;

    swi_bus_ref_ct--;  // track total SWI instances

    // if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
    if (hal && swi_hal_data[hal->bus_index] != NULL)
    {
        swi_uart_deinit(swi_hal_data[hal->bus_index]);
        free(swi_hal_data[hal->bus_index]);
        swi_hal_data[hal->bus_index] = NULL;
    }
    return ATCA_SUCCESS;
}

/** @} */

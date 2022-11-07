/**
 * \file
 * \brief  Microchip CryptoAuth device object
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

#include <stdlib.h>
#include "atca_device.h"

/** \defgroup device ATCADevice (atca_)
 * \brief ATCADevice object - composite of command and interface objects
   @{ */


/** \brief constructor for a Microchip CryptoAuth device
 * \param[in] cfg  pointer to an interface configuration object
 * \return Reference to a new ATCADevice on success. NULL on failure.
 */
ATCADevice newATCADevice(ATCAIfaceCfg *cfg)
{
    ATCADevice ca_dev = NULL;
    if (cfg == NULL)
    {
        return NULL;
    }

    ca_dev = (ATCADevice)malloc(sizeof(struct atca_device));
    if (ca_dev == NULL)
    {
        return NULL;
    }
    ca_dev->mCommands = newATCACommand(cfg->devtype);
    ca_dev->mIface    = newATCAIface(cfg);

    if (ca_dev->mCommands == NULL || ca_dev->mIface == NULL)
    {
        if (ca_dev->mCommands)
        {
            deleteATCACommand(&ca_dev->mCommands);
        }
        if (ca_dev->mIface)
        {
            deleteATCAIface(&ca_dev->mIface);
        }

        free(ca_dev);
        ca_dev = NULL;
    }

    return ca_dev;
}

    
/** \brief initializer for an Microchip CryptoAuth device
 * \param[in] cfg  pointer to an interface configuration object
 * \param[in] ca_dev  pre-allocated structure to be initialized
 * \return reference to a new ATCADevice
 */
ATCADevice initATCADevice(ATCAIfaceCfg *cfg, ATCADevice ca_dev)
{
	ATCACommand cmdResult;
	ATCAIface   ifaceResult;
	
	// Ensure all members have allocated ram first
	if (cfg == NULL || ca_dev == NULL || ca_dev->mCommands == NULL || ca_dev->mIface == NULL)
	{
		return NULL;
	}

	cmdResult	= initATCACommand(cfg->devtype,ca_dev->mCommands);
	ifaceResult = initATCAIface(cfg,ca_dev->mIface);

	if (cmdResult == NULL || ifaceResult == NULL)
	{
		return  NULL;
	}

	return ca_dev;
}

/** \brief returns a reference to the ATCACommand object for the device
 * \param[in] dev  reference to a device
 * \return reference to the ATCACommand object for the device
 */
ATCACommand atGetCommands(ATCADevice dev)
{
    return dev->mCommands;
}

/** \brief returns a reference to the ATCAIface interface object for the device
 * \param[in] dev  reference to a device
 * \return reference to the ATCAIface object for the device
 */
ATCAIface atGetIFace(ATCADevice dev)
{
    return dev->mIface;
}

/** \brief destructor for a device NULLs reference after object is freed
 * \param[in] ca_dev  pointer to a reference to a device
 */
void deleteATCADevice(ATCADevice *ca_dev)   // destructor
{
    struct atca_device *dev = *ca_dev;

    if (*ca_dev)
    {
        deleteATCACommand(&dev->mCommands);
        deleteATCAIface(&dev->mIface);
#ifndef ATCA_INTERFACE_V2
        free(*ca_dev);
#endif
    }

    *ca_dev = NULL;
}

/** @} */

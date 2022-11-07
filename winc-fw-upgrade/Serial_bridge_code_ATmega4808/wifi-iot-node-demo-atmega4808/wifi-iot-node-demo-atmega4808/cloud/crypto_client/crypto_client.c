/*
    \file   crypto_client.c

    \brief  Crypto Client source file.

    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include "cryptoauthlib/lib/tls/atcatls.h"
#include <stdio.h>
#include "crypto_client.h"
#include "../core/core.h"
#include "../cryptoauthlib/lib/jwt/atca_jwt.h"

const uint8_t public_key_x509_header[] = { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
	0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
	0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
0x42, 0x00, 0x04 };

uint8_t g_serial_number[ATCA_SERIAL_NUM_SIZE];

/** \brief custom configuration for an ECCx08A device */
ATCAIfaceCfg cfg_ateccx08a_i2c_custom = {
	.iface_type             = ATCA_I2C_IFACE,
	.devtype                = ATECC608A,
	.atcai2c.slave_address  = 0xB0,
	.atcai2c.bus            = 2,
	.atcai2c.baud           = 100000,
	.wake_delay             = 1500,
	.rx_retries             = 20
};

uint8_t CRYPTO_CLIENT_createJWT(char* buf, size_t buflen, uint32_t ts)
{
	int rv = -1;

	if(buf && buflen)
	{
		atca_jwt_t jwt;
		
		//uint32_t ts = time_utils_get_utc();

		rv = atcab_init(&cfg_ateccx08a_i2c_custom);
		if(ATCA_SUCCESS != rv)
		{
			return rv;
		}

		/* Build the JWT */
		rv = atca_jwt_init(&jwt, buf, buflen);
		if(ATCA_SUCCESS != rv)
		{
			return rv;
		}

		if(ATCA_SUCCESS != (rv = atca_jwt_add_claim_numeric(&jwt, "iat", ts)))
		{
			return rv;
		}

		if(ATCA_SUCCESS != (rv = atca_jwt_add_claim_numeric(&jwt, "exp", ts + 60*20)))
		{
			return rv;
		}

		if(ATCA_SUCCESS != (rv = atca_jwt_add_claim_string(&jwt, "aud", project_id)))
		{
			return rv;
		}

		rv = atca_jwt_finalize(&jwt, 0);

		atcab_release();
	}
	return rv;
}

uint8_t CRYPTO_CLIENT_printPublicKey()
{
	uint8_t buf[128];
	uint8_t * tmp;
	size_t buf_len = sizeof(buf);
	ATCA_STATUS rv = ATCA_SUCCESS;
	
	rv = atcab_init(&cfg_ateccx08a_i2c_custom);
	if(ATCA_SUCCESS != rv)
	{
		return rv;
	}
	
	/* Calculate where the raw data will fit into the buffer */
	tmp = buf + sizeof(buf) - ATCA_PUB_KEY_SIZE - sizeof(public_key_x509_header);
	
	/* Copy the header */
	memcpy(tmp, public_key_x509_header, sizeof(public_key_x509_header));
	
	/* Get public key without private key generation */
	rv = atcab_get_pubkey(0, tmp + sizeof(public_key_x509_header));
	
	//    atcab_release();
	
	if (ATCA_SUCCESS != rv ) {
		return rv;
	}
	
	/* Convert to base 64 */
	rv = atcab_base64encode(tmp, ATCA_PUB_KEY_SIZE + sizeof(public_key_x509_header), (char*) buf, &buf_len);
	
	if(ATCA_SUCCESS != rv)
	{
		return rv;
	}
	
	/* Add a null terminator */
	buf[buf_len] = 0;
	
	/* Print out the key */
	printf("-----BEGIN PUBLIC KEY-----\r\n%s\r\n-----END PUBLIC KEY-----\r\n", buf);
	
	return rv;
}

uint8_t CRYPTO_CLIENT_printSerialNumber(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	
	status = atcab_init(&cfg_ateccx08a_i2c_custom);
	
	if(ATCA_SUCCESS != status)
	{
		return status;
	}
	
	status = atcatls_get_sn(g_serial_number);
	
	if (status == ATCA_SUCCESS)
	{
		
		printf("-----BEGIN SERIAL NUMBER-----\r\n");
		
		for (uint8_t i = 0; i < ATCA_SERIAL_NUM_SIZE; i++)
		{
			printf("%X", g_serial_number[i]);
		}
		
		printf("\r\n-----END SERIAL NUMBER-----\r\n");

	}
	else{
		printf("Get serial number failed\n\r");
		asm volatile("nop");
	}
	
	return status;
}

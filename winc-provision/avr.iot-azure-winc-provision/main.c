/*
    \file   main.c

    \brief  Main source file.

    (c) 2019 Microchip Technology Inc. and its subsidiaries.

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

#include <interrupt_avr8.h>
#include <string.h>
#include <stdlib.h>
#include "atmel_start.h"
#include "winc_certs_functions.h"
#include "convertions.h"
#include "winc/driver/include/m2m_wifi.h"
#include "cert_def_3_device_csr.h"
#include "tls/atcatls.h"

#define ATCA_SERIAL_NUM_SIZE	(9)
#define COMMON_NAME_SIZE        (ATCA_SERIAL_NUM_SIZE * 2)
#define COMMON_NAME_START_POS   (43)
#define MAX_MESSAGE_SIZE		1200
#define CSR_BUFFER_LENGTH		1500
#define ARGUMENT_DELIMITER		':'
#define RECEIVE_CA_PUB_KEY		"caPubKey"
#define RECEIVE_CA_CERT			"caCert"
#define RECEIVE_DEVICE_CERT		"deviceCert"
#define RECEIVE_DEVICE_NAME		"deviceName"
#define TRANSFERT_TO_WINC		"transferCertificates"
#define GENERATE_CSR			"genCsr"
#define MESSAGE_EXECUTED		"Done.\0"

static void addCommonNameToCSR(void)
{
	uint8_t g_serial_number[ATCA_SERIAL_NUM_SIZE];
	
	ATCA_STATUS status = atcatls_get_sn(g_serial_number);
	
	char *commonName = g_csr_def_3_device.cert_template + COMMON_NAME_START_POS;
	
	char commonNameAscii[COMMON_NAME_SIZE];
    
    uint8_t index = 0;
	
	for(index = 0; index < ATCA_SERIAL_NUM_SIZE; index ++)
	{
		sprintf(commonNameAscii + 2 * index,"%02X", g_serial_number[index]);
	}
	
	strncpy(commonName, commonNameAscii, COMMON_NAME_SIZE);
}


static void kitComWriteString(char * str)
{
    for(size_t i = 0; i <= strlen(str); i++)
    {
        USART_0_write(str[i]);
    }
}

static char kitComReadChar()
{
    return (char)USART_0_read();
}

static void processMessage(uint8_t *message, uint16_t *message_length)
{
    char *message_method;
    char *argument;
    
    message_method = message;
    argument = strchr(message, ARGUMENT_DELIMITER);
    *argument = NULL;
    argument = argument + 1;

    if (strcmp(message_method, RECEIVE_CA_PUB_KEY) == 0)
    {
        CONVERTIONS_hexToBinary(strlen(argument), argument); 
        signer_ca_public_key_size = 64;
        memcpy(g_signer_1_ca_public_key, &argument[0], 64);
        memcpy(message, MESSAGE_EXECUTED, sizeof(MESSAGE_EXECUTED));
    }
    
    if (strcmp(message_method, RECEIVE_CA_CERT) == 0)
    {
        uint16_t cert_size;  
        cert_size = CONVERTIONS_hexToBinary(strlen(argument), argument);   
        memcpy(signer_cert, argument, cert_size);
        signer_cert_size = cert_size;     
        memcpy(message, MESSAGE_EXECUTED, sizeof(MESSAGE_EXECUTED));
    }
    
    if (strcmp(message_method, RECEIVE_DEVICE_CERT) == 0)
    {
        uint16_t cert_size;     
        cert_size = CONVERTIONS_hexToBinary(strlen(argument), argument);
        memcpy(device_cert, argument, cert_size);
        device_cert_size = cert_size;        
        memcpy(message, MESSAGE_EXECUTED, sizeof(MESSAGE_EXECUTED));
    }
	

    if (strcmp(message_method, TRANSFERT_TO_WINC) == 0)
    {
        cryptoauthlib_init();
        WINC_CERTS_transfer(NULL);
        memcpy(message, MESSAGE_EXECUTED, sizeof(MESSAGE_EXECUTED));
    }
    
    if (strcmp(message_method, GENERATE_CSR) == 0)
    {
        uint8_t *csr_buffer = (uint8_t*) sector_buffer;
        size_t csr_buffer_length = CSR_BUFFER_LENGTH;
        
        atcacert_create_csr(&g_csr_def_3_device, csr_buffer, &csr_buffer_length);
        CONVERTIONS_binaryToHex((uint16_t)csr_buffer_length, csr_buffer);
        csr_buffer[2 * csr_buffer_length] = NULL;
        
        memcpy(message, csr_buffer, 2 * csr_buffer_length + 1);
        
        // Certificate buffer space is also used to generate the CSR 
        // so only initialize it after the CSR was generated 
        WINC_CERTS_initBuffer();
    }
}

int main(void)
{	
    char c;     
	char kitMessage[MAX_MESSAGE_SIZE];
    uint16_t idx = 0;
    tstrWifiInitParam wifi_paramaters;
    
    atmel_start_init();
    Enable_global_interrupt();
    cryptoauthlib_init();
    
    nm_bsp_init();
    // Need to initialize the WIFI for the certificate transfer to work.
    m2m_memset((uint8*)&wifi_paramaters, 0, sizeof(wifi_paramaters));
    m2m_wifi_init(&wifi_paramaters);
	
	addCommonNameToCSR();
	
	asm("nop");
   
	while (1) 
	{
        c = kitComReadChar();	
        if(c != '\0')
        {
            kitMessage[idx++] = c;
        }
        else
        {
            kitMessage[idx++] = '\0';
            
            processMessage((uint8_t*)kitMessage, &idx);
            kitComWriteString(kitMessage);

            idx = 0;    
        }
	}
	
	return 0;
}

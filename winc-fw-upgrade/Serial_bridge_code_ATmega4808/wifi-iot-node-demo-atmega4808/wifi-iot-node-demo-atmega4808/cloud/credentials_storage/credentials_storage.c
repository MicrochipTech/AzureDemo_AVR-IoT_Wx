/*
    \file   credentials_storage.c

    \brief  Credential Storage source file.

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

#include <stdint.h>
#include <avr/eeprom.h>
#include "credentials_storage.h"
#include "../core/core.h"

#define EEPROM_SSID  0
#define EEPROM_PSW   EEPROM_SSID + MAX_WIFI_CREDENTIALS_LENGTH
#define EEPROM_SEC   EEPROM_PSW + MAX_WIFI_CREDENTIALS_LENGTH

/**
 * \brief Read WiFi SSID and password from EEPROM
 *
 * \param ssidbuf     buffer for SSID
 * \param passwordbuf buffer for password
 * \param sec         pointer to security type
 */
void CREDENTIALS_STORAGE_read(char *ssidbuf, char *passwordbuf, char *sec)
{
	uint8_t i = MAX_WIFI_CREDENTIALS_LENGTH;
	uint8_t *addr = EEPROM_SSID;
	while (i--)
	{
		*ssidbuf++ = eeprom_read_byte((uint8_t *)addr++);
	}
	i = MAX_WIFI_CREDENTIALS_LENGTH;
	addr = (uint8_t *)EEPROM_PSW;
	while (i--)
	{
		*passwordbuf++ = eeprom_read_byte((uint8_t *)addr++);
	}
	*sec = eeprom_read_byte((uint8_t *)EEPROM_SEC);
}

/**
 * \brief Store WiFi SSID and password to EEPROM
 *
 * \param ssidbuf     buffer with SSID
 * \param passwordbuf buffer with password
 * \param sec         pointer to security type
 */
void CREDENTIALS_STORAGE_save(char *ssidbuf, char *passwordbuf, char *sec)
{
	uint8_t i = MAX_WIFI_CREDENTIALS_LENGTH;
	uint8_t *addr = EEPROM_SSID;
	while (*ssidbuf && i--)
	{
		eeprom_write_byte((uint8_t *)addr++, *ssidbuf++);
	}
	if (i) {
		while (i--) eeprom_write_byte((uint8_t *)addr++, 0);
	}
	i = MAX_WIFI_CREDENTIALS_LENGTH;
	addr = (uint8_t *)EEPROM_PSW;
	while (*passwordbuf && i--)
	{
		eeprom_write_byte((uint8_t *)addr++, (uint8_t)*passwordbuf++);
	}
	if (i) {
		while (i--) eeprom_write_byte((uint8_t *)addr++, 0);
	}
	eeprom_write_byte((uint8_t *)EEPROM_SEC, (uint8_t)*sec);
}
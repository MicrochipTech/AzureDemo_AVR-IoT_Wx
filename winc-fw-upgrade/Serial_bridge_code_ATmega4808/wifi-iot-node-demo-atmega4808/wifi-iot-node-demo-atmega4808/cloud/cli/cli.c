/*
    \file   cli.c

    \brief  Command Line Interpreter source file.

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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cli.h"
#include "../core/core.h"
#include "../crypto_client/crypto_client.h"
#include "../credentials_storage/credentials_storage.h"
#include "../api.h"
#include "usart_basic.h"

#define NUM_COMMANDS 4

char command[30];
uint8_t i = 0;

char command_reconnect[] = "reconnect";
char command_wifi_auth[] = "wifi";
char command_key[] = "key";
char command_device[] = "device";

void command_received(char *command_text);

void reconnect_cmd(char *notUsed);
void set_wifi_auth(char *ssid_pwd_auth);
void get_public_key(char *notUsed);
void get_device_id(char *notUsed);

struct cmd {
	char *command;
	void (*handler)(char *argument);
};

struct cmd commands[NUM_COMMANDS] = {
	{ command_reconnect, reconnect_cmd},
	{ command_wifi_auth, set_wifi_auth },
	{ command_key,       get_public_key },
	{ command_device,    get_device_id },
};

void CLI_commandReceive() {
	if(USART_0_is_rx_ready()) {
		char c = USART_0_read();
		printf("%c", c);
		if( c != '\n' && c != 0 && c!='\r' ){
			command[i++] = c;
			} else if(c == '\n') {
			command[i] = '\0';
			command_received(command);
			i = 0;
		}
	}
}

void set_wifi_auth(char *ssid_pwd_auth) {
	char *credentials[4];
	uint8_t i = 0;
	
	char * pch;
	pch = strtok (ssid_pwd_auth, ",");
	while (pch != NULL && i < 4)
	{
		credentials[i++] = pch;
		pch = strtok (NULL, ",");
	}
	
	if(i == 3) {
		if(strlen(credentials[0]) < MAX_WIFI_CREDENTIALS_LENGTH) {
			strcpy(ssid, credentials[0]);
		}
		if(strlen(credentials[1]) < MAX_WIFI_CREDENTIALS_LENGTH) {
			strcpy(pass, credentials[1]);
		}
		if(strlen(credentials[2]) < MAX_WIFI_CREDENTIALS_LENGTH) {
			strcpy(auth_type, credentials[2]);
		}
		CREDENTIALS_STORAGE_save(ssid, pass, auth_type);
		printf("OK\n\r");
	}
}

void reconnect_cmd(char *notUsed) {
	CLOUD_reconnect();
	printf("OK\n\r");
}

void get_public_key(char *notUsed) {
	CRYPTO_CLIENT_printPublicKey();
}

void get_device_id(char *notUsed) {
	CRYPTO_CLIENT_printSerialNumber();
}


void command_received(char *command_text) {
	char *argument = strstr(command_text, " ");
	
	if(argument != NULL) {
		*argument = '\0';
	}
	
	for(uint8_t i = 0; i < NUM_COMMANDS; i++) {
		uint8_t cmp = strcmp(command_text, commands[i].command);
		uint8_t ct_len = strlen(command_text);		
		uint8_t cc_len = strlen(commands[i].command);
		
		if(cmp == 0 && ct_len == cc_len) {
			argument++;
			if(commands[i].handler != NULL) {
				commands[i].handler(argument);
				return;
			}
		}
	}

}

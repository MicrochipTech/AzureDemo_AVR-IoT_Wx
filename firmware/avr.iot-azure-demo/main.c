/* 
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

#include "./config/clock_config.h"
#include <atmel_start.h>
#include "cloud/cloud.h"
#include <sensors_handling.h>
#include <led.h>

#define PUBLISH_INTERVAL    (1000)
#define JSON_SIZE           (35)

absolutetime_t publishADC(void *payload) 
{
    char json[JSON_SIZE];
    
    uint16_t rawTemperature = SENSORS_getTempValue();
    uint16_t light = SENSORS_getLightValue();
    sprintf(json, "{\"L\":%d,\"T\":\"%d.%02d\"}", light, rawTemperature / 100, abs(rawTemperature) % 100);
        
    CLOUD_publish((uint8_t*) json);
    
    LED_YELLOW_toggle_level();

    return PUBLISH_INTERVAL;
}

int main(void)
{
    atmel_start_init();
    ENABLE_INTERRUPTS();
    
    LED_test();
    
    CLOUD_setSendFunction(publishADC);
    CLOUD_setObserver(LED_userHandler);
    CLOUD_startApp();
    
    while (1)
    {
        CLOUD_runTask();           
    }
}
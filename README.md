# Provisioning the Microchip AVR-IoT Wx Development Board for Azure IoT Services

## Introduction

 This document describes how to connect the AVR-IoT Wx Development Board (featuring an 8-bit ATmega4808 AVR MCU, ATECC608A secure element, and ATWINC1510 Wi-Fi module) to Azure IoT Hub. The AVR-IoT Wx Development Board will be provisioned for use with Azure IoT services using self-signed X.509 certificate-based authentication.

## Software Requirements

- Python [3.5.4](https://www.python.org/downloads/release/python-354/)
- Python [3.10.6](https://www.python.org/downloads/release/python-3106/)
- [Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

Note: Make sure Python is functional before proceeding to the instructions.

## Hardware Requirements

- AVR-IoT ([WA](https://www.microchip.com/en-us/development-tool/EV15R70A) or [WG](https://www.microchip.com/en-us/development-tool/AC164160)) Development Board

## Instructions

1.	Create a [free Azure account](https://azure.microsoft.com/en-us/free/?WT.mc_id=A261C142F).
2.	Proceed to the [Azure Portal](https://portal.azure.com/).
3.	Follow the steps from [Create an IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/quickstart-send-telemetry-node#create-an-iot-hub) section to configure an IoT Hub.
4.	Clone this repository (or download ZIP file).
5.	Upgrade the WINC firmware.
	- Go to repository location and browse *“AVR-IoT_Azure_Demo\winc-fw-upgrade\ Serial_bridge_code_ATmega4808\wifi-iot-node-demo-atmega4808”*.
	- Program the wifi-iot-node-demo-atmega4808 project on the AVR-IoT board.
	- Go to repository location and browse *“AVR-IoT_Azure_Demo\winc-fw-upgrade\flashing_env\WINC1500_IoT_REL_19_6_1_30MAY2018_ATmega4808-4809\src”*.
	- Run *download_all_sb_ATmega4808.bat*. The script will have 5 attempts to run successfully. Wait until all of them are done.
* Note: if the WINC firmware upgrade requires another nEDBG firmware, follow the next step and then return to step 5.
6.	 Upgrade the nEDBG firmware.
		1. Go to repository location and browse *“AVR-IoT_Azure_Demo\nedbg-fw-upgrade”*. Copy this path.
		2. Go to Atmel Studio installation folder and browse *“Atmel\Studio\7.0\atbackend\”* (e.g. *“C:\Program Files (x86)\Atmel\Studio\7.0\atbackend\”*).
		3.	Open Command Prompt (cmd) in the folder specified at 6.2 and call *atfw.exe* as in the example below: `atfw.exe -t nedbg -a “=path from step 5.1=\nedbg_fw-1.2.261.zip”`.
* Note: after redoing step 5, there’s no need to do step 6 again.
7.	Install [Azure CLI](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli?view=azure-cli-latest).
8.	Go to repository location and browse *“AVR-IoT_Azure_Demo\winc-provision”*.
9.	Program the *avr.iot-azure-winc-provision* project on the AVR-IoT board.
10.	Go to repository location and browse *“AVR-IoT_Azure_Demo\scripts”*.
11.	Open PowerShell as Administrator.
12. Run `pip install -r .\requirements.txt`.
13.	Run `create_and_verify_certs.ps1`.
14.	Run `create_device_and_provision.ps1`.
15.	Copy the device ID from the console.

![Device Name](./img/DeviceName.PNG)

16.	Go to repository location and browse *“AVR-IoT_Azure_Demo\firmware”*.
17.	Modify the credentials of the network, hub and device.
	1. Open *avr.iot-azure-demo.atsln* using the Microchip Studio IDE.
	2. Browse to *“cloud”* folder and open *“cloud.h”* file.
	3. Modify the network password and ssid.
	4. Modify the COMMON_NAME with the value from step 15.
	5. Go to Azure portal and click on IoT Hub name. Copy the name from Hostname tab.
	6. Modify the HOST_ENDPOINT with the value from step above.
18. Program the *avr.iot-azure-demo* on the AVR-IoT board.
19. Go to Azure Portal and open Cloud Shell.

![Cloud Schell Icon](./img/CloudSchell.PNG)

20. Run `az extension add --name azure-cli-iot-ext`.
21. Run `az iot hub monitor-events --hub-name {MyIoTHubName} --device-id {MyDevice}`.
	Note: replace *{MyIoTHubName}* and *{MyDevice}* with their actual names.
22. For better visualization of data, go to repository location, browse *“AVR-IoT_Azure_Demo\scripts”*  and run  `create_web_app.ps1`.

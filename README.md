# Provisioning the Microchip AVR-IoT Wx Development Board for Azure IoT Services

## Introduction

 This document describes how to connect the AVR-IoT Wx Development Board (featuring an 8-bit ATmega4808 AVR MCU, ATECC608A secure element, and ATWINC1510 Wi-Fi module) to Azure IoT Hub. The AVR-IoT Wx Development Board will be provisioned for use with Azure IoT services using self-signed X.509 certificate-based authentication.

## Software Requirements

- Python [3.5.4](https://www.python.org/downloads/release/python-354/)
- Python [3.10.6](https://www.python.org/downloads/release/python-3106/)
- [Azure CLI](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli?view=azure-cli-latest)
- [Microchip Studio for AVR and SAM Devices](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

Note: Make sure Python is functional before proceeding to the instructions.

## Hardware Requirements

- AVR-IoT ([WA](https://www.microchip.com/en-us/development-tool/EV15R70A) or [WG](https://www.microchip.com/en-us/development-tool/AC164160)) Development Board

## Instructions

1.	If you don't already have one, create a free [Azure account](https://azure.microsoft.com/en-us/free/?WT.mc_id=A261C142F).
2.	Sign into the [Azure Portal](https://portal.azure.com/).
3.	Follow the steps in the section [Create an IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/quickstart-send-telemetry-node#create-an-iot-hub) to use Azure CLI to create your own resource group and IoT Hub within the resource group.
4.	Clone this repository (or download ZIP file).
5.	Upgrade the WINC firmware:
	1. Go to repository location and browse *“AVR-IoT_Azure_Demo\winc-fw-upgrade\ Serial_bridge_code_ATmega4808\wifi-iot-node-demo-atmega4808”*.
	2. Program the *`wifi-iot-node-demo-atmega4808.atsln`* project on the AVR-IoT board.
	3. Go to repository location and browse to *“AVR-IoT_Azure_Demo\winc-fw-upgrade\flashing_env\WINC1500_IoT_REL_19_6_1_30MAY2018_ATmega4808-4809\src”*.
	4. Execute *`download_all_sb_ATmega4808.bat`* in a command line window. The script will have 5 attempts to run successfully. Wait until all of them are done.
* Note: if the WINC firmware upgrade requires another nEDBG firmware, follow the next step and then return to step 5.
6.	 Upgrade the nEDBG firmware:
		1. Go to repository location and browse *“AVR-IoT_Azure_Demo\nedbg-fw-upgrade”*. Copy this path.
		2. Go to the Microchip Studio installation folder and browse to *“Microchip\Studio\7.0\atbackend\”* (e.g. *“C:\Program Files (x86)\Microchip\Studio\7.0\atbackend\”*).
		3. Open a Command Prompt window (cmd) in the folder specified at 6.2 and call *atfw.exe* like in the following example: `atfw.exe -t nedbg -a “=path from step 5.1=\nedbg_fw-1.2.261.zip”`.
* Note: after redoing step 5, there’s no need to do step 6 again.
7.	In the cloned repository, browse to *“AVR-IoT_Azure_Demo\winc-provision”*.
8.	Program the *`avr.iot-azure-winc-provision.atsln`* project onto the AVR-IoT board.
9. Open PowerShell as an Administrator.
10.	Go to repository location and browse *“AVR-IoT_Azure_Demo\scripts”*.
11. Run `pip install -r .\requirements.txt`.
12.	Run `create_and_verify_certs.ps1`.
13.	Run `create_device_and_provision.ps1`.
14.	Copy (save) the device ID from the output of the PowerShell window.

	![Device Name](./img/DeviceName.PNG)

15.	Go to repository location and browse *“AVR-IoT_Azure_Demo\firmware”*.
16.	Modify the credentials of the network, hub and device.
	1. Open the *`avr.iot-azure-demo.atsln`* project using the Microchip Studio IDE.
	2. Browse to the *“cloud”* folder and open the `cloud.h` header file.
	3. Modify the network password and ssid for your wireless Access Point.
	4. Modify the `COMMON_NAME` with the device ID value from step 14.
	5. Go to the Azure portal and click on IoT Hub name. Copy the name from the Hostname tab.
	6. Modify the `HOST_ENDPOINT` definition with the value copied from the Hostname tab.
17. Program the `avr.iot-azure-demo.atsln` project onto the AVR-IoT board.
18. Go to Azure Portal and open Cloud Shell.

	![Cloud Schell Icon](./img/CloudSchell.PNG)

19. Run the command line
    ```shell
	az extension add --name azure-cli-iot-ext
    ```

20. Run the command line
	```shell
	az iot hub monitor-events --hub-name {MyIoTHubName} --device-id {MyDevice}
	```

	Note: replace *{MyIoTHubName}* and *{MyDevice}* with their actual names

21. For better visualization of data, go to repository location, browse *“AVR-IoT_Azure_Demo\scripts”*  and run  `create_web_app.ps1`.

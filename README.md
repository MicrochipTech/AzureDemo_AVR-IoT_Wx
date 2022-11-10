# Provisioning the Microchip AVR-IoT WA/WG Development Board for Azure IoT Services

## Introduction

 This document describes how to connect the AVR-IoT [WA](https://www.microchip.com/en-us/development-tool/EV15R70A) or [WG](https://www.microchip.com/en-us/development-tool/AC164160) Development Board (featuring an 8-bit [ATmega4808](https://www.microchip.com/en-us/product/ATMEGA4808) AVR MCU, [ATECC608A](https://www.microchip.com/en-us/product/atecc608a) secure element, and [ATWINC1510](https://www.microchip.com/en-us/product/ATWINC1510) Wi-Fi network controller module) to Azure IoT Hub. The AVR-IoT WA/WG Development Board will be provisioned for use with Azure IoT services using self-signed X.509 certificate-based authentication.

## Software Installation Requirements

- [Azure CLI](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli?view=azure-cli-latest)

- [Microchip Studio IDE for AVR and SAM Devices](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

- Microchip's [IoT Provisioning Tool](https://www.microchip.com/en-us/solutions/internet-of-things/iot-development-kits/iot-provisioning-tool)

## Hardware Requirements

- AVR-IoT ([WA](https://www.microchip.com/en-us/development-tool/EV15R70A) or [WG](https://www.microchip.com/en-us/development-tool/AC164160)) Development Board

## Instructions

1.	If you don't already have one, create a free [Azure account](https://azure.microsoft.com/en-us/free/?WT.mc_id=A261C142F).

2.	Sign into the [Azure Portal](https://portal.azure.com/).

3.	Follow the steps in the section titled ["Create an IoT Hub"](https://docs.microsoft.com/en-us/azure/iot-hub/quickstart-send-telemetry-node#create-an-iot-hub) to use the Azure CLI to create your own resource group and IoT Hub within the resource group.

4.	Clone this repository using [Git Command Line](https://git-scm.com/downloads) or [GitHub Desktop](https://desktop.github.com). Alternatively, you can download the ZIP file of this repository and extract the ZIP file directly onto your PC's hard drive.

	![GitHub 01](./img/github_01_edit.png)

5.	Connect the AVR-IoT development board to the PC using a standard micro-USB cable. Generate and provision the required X.509 certificates by opening a Command Prompt or PowerShell window. Navigate to the location of the [IoT Provisioning Tool](https://www.microchip.com/en-us/solutions/internet-of-things/iot-development-kits/iot-provisioning-tool) which was previously downloaded (e.g. \iotprovision-bin-2.10.7.12.134\windows64). Execute the following command line:

    ```shell
	.\iotprovision-bin.exe -c azure
    ```

    NOTE: Ignore the error messages at the end regarding 'demo-azure'

6.	The [IoT Provisioning Tool](https://www.microchip.com/en-us/solutions/internet-of-things/iot-development-kits/iot-provisioning-tool) should have created a hidden folder named `.microchip_iot` located in the user's "home" directory (e.g. C:\Users\username). Configure your O.S. to show hidden files & folders, and then confirm that the Root CA, Signer CA, and device certificates were generated in this hidden folder. The device certificate wiil be located in a sub-folder corresponding to the kit's USB serial number (e.g. ATML3203071800001079).

	![CRT 01](./img/crt_01.png)

7. Create copies of the 3 certificates (root-ca.crt, signer-ca.crt, device.crt) and rename them using the *.pem extension (root-ca.pem, signer-ca.pem, device.pem).

	![PEM 01](./img/pem_01.png)

8. In the kit's USB serial number sub-folder, open the `azure-device-id.txt` file and note the device ID that is used for the Common Name field in the device certificate (e.g. sn0123460B75A64D68FE). Save this device ID information for use in a future step as this string will be used to identify your device's connection to the IoT Hub.

9.	Launch the Microchip Studio IDE. A picture of the AVR-IoT development board should appear in its own dedicated window within the IDE - showing that Microchip Studio has successfully connected to the AVR-IoT development board. Do not proceed until the connection to the board has been verified with the appearance of this pop-up window.

	![Kit Window](./img/Kit_Window.png)

10.	Modify the credentials of your wireless network, IoT Hub, and device in the provided example demo project.
	- Open the *`avr.iot-azure-demo.atsln`* project located in the *`AVR-IoT_Azure_Demo\firmware`* folder.
		![Studio Step 01](./img/Studio_Step_01.png)
		![Studio Step 02a](./img/Studio_Step_02a.png)
	- In the Solution Explorer window, expand the *“cloud”* sub-folder and open the `cloud.h` header file by double-clicking on it.
		![Studio Step 02b](./img/Studio_Step_02b.png)
	- Modify the SSID and network password (PSK) for your wireless Access Point.
		![Studio Step 02c](./img/Studio_Step_02c.png)	
	- Modify the `COMMON_NAME` with your specific device ID string which was discovered in a previous step.
		![Studio Step 02d](./img/Studio_Step_02d.png)
	- Go to the [Azure Portal](https://portal.azure.com/) and click on your IoT Hub name.
		![Studio Step 02de](./img/Studio_Step_02de_edit.png)
	- Copy the name shown in the IoT Hub's `Hostname` field by clicking on the icon to the right of the string.
		![Studio Step 02e](./img/Studio_Step_02e_edit.png)
	- Modify (by pasting from the clipboard) the `HOST_ENDPOINT` definition with the value copied from the `Hostname` field.
		![Studio Step 02f](./img/Studio_Step_02f.png)
	- Review and confirm that the 4 required settings were updated correctly for your scenario, and that every value is enclosed in double quotes. The following snippet is an example of one possible scenario:
		![Studio Step 02g](./img/Studio_Step_02g_edit.png)

11. Rebuild and program the solution onto the AVR-IoT development board.

	![Studio Step 03](./img/Studio_Step_03.png)
	![Studio Step 04](./img/Studio_Step_04.png)
	![Studio Step 05](./img/Studio_Step_05_edit.png)
	![Studio Step 06](./img/Studio_Step_06_edit.png)

12. Go back into your [Azure Portal](https://portal.azure.com/) account and click on your IoT Hub. Using the left-hand navigation pane, click on `Devices` and then `+ Add Device`

	![Hub Step 01](./img/Hub_Step_01_edit.png)

13. Enter/paste your Device ID and select `X.509 CA Signed` for `Authentication type`, then hit the `Save` button.

	![Hub Step 02a](./img/Hub_Step_02a_edit.png)	

14. Confirm that your new device has been successfully registered with your IoT Hub.

	![Hub Step 02b](./img/Hub_Step_02b_edit.png)	

15. Using the left-hand navigation pane, click on `Certificates` under `Security settings`, then click on the `+ Add` icon.

	![Hub Step 03](./img/Hub_Step_03_edit.png)

16. Type in any arbitrary Certificate name (e.g. your initials), upload the `signer-ca.pem` file that was generated in a previous step, check the box for "Set certificate status to verified on upload", then click on `Save`. The uploaded signer CA certificate can now be used by your IoT Hub to authenticate devices which have a leaf certificate derived from the signer CA certificate.

	![Hub Step 04](./img/Hub_Step_04_edit.png)
	![Hub Step 05](./img/Hub_Step_05.png)

17. Power cycle the AVR-IoT development board by unplugging and then plugging the micro-USB cable back into the board's USB connector. After a few moments, the Blue (WIFI) and Green (CONN) LEDs should turn constantly ON while the Yellow (DATA) LED blinks - signifying that the board has connected to the IoT Hub and is periodically sending data to the Cloud.

18. In your [Azure Portal](https://portal.azure.com/) account, using the left-hand navigation pane, click on `Devices` under `Device management`, then click on your device name.

	![Hub Step 06](./img/Hub_Step_06_edit.png)	

19. Click on the `Device Twin` icon, then confirm that `"connectionState"` = **"Connected"**.

	![Hub Step 07](./img/Hub_Step_07_edit.png)	
	![Hub Step 08](./img/Hub_Step_08_edit.png)

20. Monitor the telemetry events being sent from the device to your IoT Hub. Open a Command Prompt or PowerShell window and execute the following command line:

	```shell
	az iot hub monitor-events --hub-name {MyIoTHubName} --device-id {MyDeviceID}
	```
	Note: replace *{MyIoTHubName}* and *{MyDeviceID}* with their actual names

21. Each telemetry event has a payload field containing a value ("L") corresponding to the on-board light sensor (lumens) and a second value ("T") corresponding to the on-board temperature sensor (deg C). Increase the ambient light source shining directly on top of the board and observe that the "L" value increases substantially compared to before the additional light source was applied. Similarly, if you are able to change the ambient temperature of the board easily, you can also see the "T" value change accordingly. When finished, hit [CTRL-Z] to exit the monitoring process.

	![Monitor Step 01](./img/Monitor_Step_01_edit.png)

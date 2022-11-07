aps-objcopy -v -S -g -O binary cortus_ota_app.jtag cortus_ota_app.bin
..\..\App_Donwloader\debug_i2c\App_Donwloader.exe -app3A0_path cortus_ota_app.bin
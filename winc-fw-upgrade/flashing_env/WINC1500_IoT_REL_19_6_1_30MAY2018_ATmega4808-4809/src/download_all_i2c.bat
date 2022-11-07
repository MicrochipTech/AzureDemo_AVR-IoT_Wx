@ECHO off
echo This script accepts 4 parameters 
echo  1 serial number of the aardvark pod in use, 0 if only 1
echo  2 rsa key path, none for none or blanks for auto
echo  3 rsa cert path, none for none or blanks for auto
echo  4 ecdsa cert path, none for none or blanks for auto
echo.

set ser=%1
set cer1=%2
set cer2=%3
set cer3=%4

if "x%ser%x" == "xx" set ser=0
if "x%cer1%x" == "xx" set cer1=none
if "x%cer2%x" == "xx" set cer2=none
if "x%cer3%x" == "xx" set cer3=none

:: default everything else
echo calling: download_all.bat I2C D21 3A0 %ser% 0 dev %cer1% %cer2% %cer3%
echo.
download_all.bat I2C D21 3A0 %ser% 0 dev %cer1% %cer2% %cer3%

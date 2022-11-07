@ECHO Off

echo usage:
echo   This program will program a WINC1500Xplained card plugged into a SamD21XplainedPro card, so long as only one is present 
echo   It fills in defaults and calls more specific script files below.
echo.
echo   You have the option to specify certificates, or none, or let the script find defaults.
echo     optional parameter 1	RSA key  path (eg ..\..\..\tls_cert_store\winc_rsa.key) or none
echo     optional parameter 2	RSA cert path (eg ..\..\..\tls_cert_store\winc_rsa.cer) or none
echo       If you include parameter 1 you must include parameter 2
echo     optional parameter 3	ECC Cert path (eg ..\..\..\tls_cert_store\winc_ecdsa.cer) or none
echo       If you only want an ECC cert you must put 'none none' for parameters 1 and 2.
echo.

set varPath=%PROGRAMFILES%

:CheckOS
IF EXIST "%PROGRAMFILES(X86)%" (GOTO 64BIT) ELSE (GOTO RUN)
:64BIT
set varPath=%PROGRAMFILES(X86)%
:RUN


set /A edbgCnt=0
set SN=0
for /f "tokens=1-2" %%i in ('"%varPath%\Atmel\Studio\7.0\atbackend\atprogram.exe" list') do (
	if "%%i" == "edbg" (
		set SN=%%j
		set /A edbgCnt+=1
	)
)	

if %edbgCnt%==0 (
	echo Cannot find and EDBG boards?
	echo see  '"%varPath%\Atmel\Studio\7.0\atbackend\atprogram.exe" list'
	exit /b 1
)

if %edbgCnt% GTR 1 (
	echo This batch file is unsuitable if more than one EDBG based development board is installed, found %edbgCnt%
	echo Use download_all_sb.bat with options
	echo		edbg 
	echo		ATSAMD21J18A 
	echo		Tools\samd21_xplained_pro_serial_bridge\Release\samd21_xplained_pro_serial_bridge.elf 
	echo		SAMD21 
	echo		[3A0]
	echo		serial number of the dev board attached to the board you wish to program - see '"%varPath%\Atmel\Studio\7.0\atbackend\atprogram.exe" list'
	echo		0 
	echo		com port number assigned to the dev board attached to the board you wish to program by the OS
	echo		..\..\..\tls_cert_store\winc_rsa.key 
	echo		..\..\..\tls_cert_store\winc_rsa.cer 
	echo		..\..\..\tls_cert_store\winc_ecdsa.cer
	exit /b 1
)

echo Calling: download_all_sb.bat
echo With:    edbg ATmega4809 Tools\ATmega4809_xplained_pro_serial_bridge\atmega4809_xplained_pro_serial_bridge.elf ATmega 3A0 %SN% 0 0 %*
echo.
call download_all_sb.bat edbg ATmega4809 Tools\ATmega4809_xplained_pro_serial_bridge\atmega4809_xplained_pro_serial_bridge.elf ATmega 3A0 %SN% 0 0 %*


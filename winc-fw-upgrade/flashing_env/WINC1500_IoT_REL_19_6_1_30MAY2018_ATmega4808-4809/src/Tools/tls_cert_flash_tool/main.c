/**
 *
 * \file
 *
 * \brief This module implements TLS Server Certificate Installation.
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver\include\m2m_types.h"
#include "crypto_lib_api.h"
#include "programmer.h"
#include "tls_srv_sec.h"


extern sint8 TlsCertStoreWriteCertChain(char *pcPrivKeyFile, char *pcSrvCertFile, char *pcCADirPath, uint8 *pu8TlsSrvSecBuff, uint32 *pu32SecSz, tenuWriteMode enuMode);

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define STIRCMP(argvPtr, text)	\
	(!_strnicmp((argvPtr), (text), strlen((text))))

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


typedef enum{
	TLS_STORE_INVALID,
	TLS_STORE_FLASH,
	TLS_STORE_FW_IMG
}tenuTLSCertStoreType;

typedef enum{
	CMD_WRITE,
	CMD_READ
}tenuCmd;

typedef struct{
	char					*pcCADir;
	char					*pcFwPath;
	char					*pcPrivKeyFile;
	char					*pcServerCertFile;
	char					*pcProgFw;
	char                    *pcflash_path;
	uint32					req_serial_number;
	uint32					req_serial_port;
	uint8					bIsKeyAvail;
	tenuTLSCertStoreType	enuCertStore;
	uint8					bEraseBeforeWrite;
	uint8					u8PauseAfterFinish;
}tstrWriteOptions;


typedef struct{
	char					*pcOutPath;
	char					*pcFwPath;
	char                    *pcflash_path;
	uint32					req_serial_number;

	uint8					bIsRsa;
	uint8					bIsEcdsa;
	uint8					bIsDumpAll;
	uint8					bPrintPriv;
	uint8					bWriteToFile;
	uint8					bIsListFiles;
	tenuTLSCertStoreType	enuCertStore;
}tstrReadOptions;


typedef struct{
	tenuCmd					enuCmd;
	union{
		tstrReadOptions		strReadOptions;
		tstrWriteOptions	strWriteOptions;
	};
}tstrCmdLineOptions;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBALS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

static uint8 gau8TlsSrvSec[M2M_TLS_SERVER_FLASH_SIZE];

/**************************************************************/
static void printName(void)
{
	printf("******************************************\n\r");
	printf("*   WINC1500 TLS Certificate Flash Tool  *\n\r");
	printf("******************************************\n\r");
}

/**************************************************************/
static void PrintHelp(tenuCmd enuCMD)
{
	if(enuCMD == CMD_WRITE)
	{
		printf("Write X.509 Certificate chain on WINC Device Flash or a given WINC\n");
		printf("firmware image file\n\n");
		printf(" [Usage]: tls_cert_flash_tool.exe write [options]\n");
		printf(" where options are:\n");
		printf(" -key file      Private key in PEM format (RSA Keys only). It MUST NOT be\n");
		printf("                encrypted.\n\n");
		printf(" -nokey         The private key is not present. This is meaningful if a the\n");
		printf("                private key is hidden into a secure hardware. This is the\n");
		printf("                typical case of using ECC508 for ECC secure key storage\n\n");
		printf(" -cert file     X.509 Certificate file in PEM or DER format. The certificate\n");
		printf("                SHALL contain the public key associated with the given\n");
		printf("                private key (If the private key is given).\n\n");
		printf(" -cadir path    [Optional] Path to a folder containing the intermediate CAs\n");
		printf("                and the Root CA of the given certificate.\n\n");
		printf(" -fwimg path    [Optional] Path to the firmware binary image file.\n");
		printf("                If this option is not given, the keys shall be written\n");
		printf("                directly on the WINC Device Flash\n\n");
		printf(" -vflash_path path [Optional] Pass in file with current flash contents, updated after write.\n");
		printf(" -pf_bin        [ignored]\n");
		printf(" -erase         Erase the certificate store before writing. If this option is\n");
		printf("                not given, the new certificate material is appended to the\n");
		printf("                certificate store\n\n");
		printf(" -port          [Optional] If not specified the SW tries to find an EDBG chip connected to a serial port, if there is more than one, specify a port\n");
	    printf(" -aardvark      [ignored]\n");
		printf("  Examples \n");
		printf("    tls_cert_flash_tool.exe Write -key rsa.key -cert rsa.cer -erase\n");
		printf("    tls_cert_flash_tool.exe Write -nokey -cert ecdsa.cer -cadir CADir\n");
		printf("    tls_cert_flash_tool.exe Write -key rsa.key -cert rsa.cer -cadir CADir\n");
		printf("    tls_cert_flash_tool.exe Write -key rsa.key -cert rsa.cer -fwimg m2m_aio_3a0.bin\n");
		printf("\n");
	}
	else if(enuCMD == CMD_READ)
	{
		printf("Read X.509 Certificate chain from WINC Device Flash or a given WINC\n");
		printf("firmware image file\n\n");
		printf(" [Usage]: tls_cert_flash_tool.exe read [options]\n");
		printf(" where options are:\n");
		printf(" -rsa           Print WINC Device RSA certificate(if any)\n\n");
		printf(" -ecdsa         Print WINC Device ECDSA certificate(if any)\n\n");
		printf(" -dir           List all files in the WINC TLS Certificate Store associated\n");
		printf("                with the selected authentication (rsa or ecdsa or both)\n\n");
		printf(" -fwimg path    [Optional] Path to the firmware binary image file.\n");
		printf("                If this option is not given, the certificates shall be read\n");
		printf("                directly from the WINC Device Flash\n\n");
	    printf(" -pf_bin        [ignored]\n");
		printf(" -out path      A path to a directory where the certificates will be saved. This\n");
		printf("                option forces the certificates to be written in files. If this option\n");
		printf("                is not specified, the certificates shall be printed on standard out.\n\n");
		printf(" -all           Dump all certificates in the WINC certificate chain provisioned on WINC\n");
		printf("                (if any) in addition to the WINC Device certificate.\n\n");
		printf(" -privkey       Print the RSA private key (if -rsa option is given) to the standard out.\n");
		printf("                The RSA private dumping is off by default.\n\n");
		printf(" -port          [Optional] If not specified the SW tries to find an EDBG chip connected to a serial port, if there is more than one, specify a port\n");
	    printf(" -aardvark      [ignored]\n");
		printf("  Examples \n");
		printf("    tls_cert_flash_tool.exe read -rsa -privkey -dir\n");
		printf("    tls_cert_flash_tool.exe read -rsa -all\n");
		printf("    tls_cert_flash_tool.exe read -rsa -out C:/Certs/\n");
		printf("    tls_cert_flash_tool.exe read -rsa -ecdsa -dir-fwimg m2m_aio_3a0.bin\n");
		printf("\n");
	}
}
/**************************************************************/
void printUsage(void)
{
	printf("Write/Read X.509 Certificate chain on/from WINC Device Flash or a given WINC\n");
	printf("firmware image file\n\n");
	printf(" [Usage]: tls_cert_flash_tool.exe <CMD> [args]\n");
	printf("  Defined Commands\n");
	printf("   write\n");
	printf("   read\n\n");
	printf("For a specific command help, use <tls_cert_flash_tool.exe <CMD> -help>\n\n");
}

void dump_flash(char * filename)
{
//#define DUMP_FLASH
#ifdef DUMP_FLASH
	M2M_PRINT("Dumping flash to %s\n", filename);
	FILE *fp;
	uint8 * pf;
	uint32 sz = programmer_get_flash_size();
	pf = malloc(sz);
	if(pf != NULL)
	{
		programmer_read(pf,0,sz);
		fp = fopen(filename,"wb");
		if(fp != NULL)
		{
			fwrite(pf,1,sz,fp);
			fclose(fp);
		}
		free(pf);
	}

#endif
}

/**************************************************************/
static int ParseWRITECmdArgs(int argc, char* argv[], tstrWriteOptions *pstrWriteOpt)
{
	int		ret		= 0;
	int		argIdx;
	char	**ppcOptionParam;
	int		isParametricOption;
	uint32	*ppcOptionParamInt;

	pstrWriteOpt->enuCertStore			= TLS_STORE_FLASH;
	pstrWriteOpt->bIsKeyAvail			= 1;
	pstrWriteOpt->bEraseBeforeWrite		= 0;
	pstrWriteOpt->u8PauseAfterFinish	= 1;
	pstrWriteOpt->req_serial_number = 0;

	for(argIdx = 0; argIdx < argc ; )
	{
		isParametricOption = 1;
		ppcOptionParam = NULL;
		ppcOptionParamInt = NULL;

		if(STIRCMP(argv[argIdx], "-key"))
		{
			ppcOptionParam = &pstrWriteOpt->pcPrivKeyFile;
		}
		else if (STIRCMP(argv[argIdx], "-cert"))
		{
			ppcOptionParam = &pstrWriteOpt->pcServerCertFile;
		}
		else if(STIRCMP(argv[argIdx],"-vflash_path"))
		{
			ppcOptionParam = &pstrWriteOpt->pcflash_path;
		}
		else if (STIRCMP(argv[argIdx], "-pf_bin"))
		{
			ppcOptionParam = &pstrWriteOpt->pcProgFw;
		}
		else if(STIRCMP(argv[argIdx], "-cadir"))
		{
			ppcOptionParam = &pstrWriteOpt->pcCADir;
		}
		else if(STIRCMP(argv[argIdx], "-fwimg"))
		{
			ppcOptionParam = &pstrWriteOpt->pcFwPath;
			pstrWriteOpt->enuCertStore = TLS_STORE_FW_IMG;
		}
		else if(STIRCMP(argv[argIdx], "-nokey"))
		{
			pstrWriteOpt->bIsKeyAvail	= 0;
			isParametricOption			= 0;
			ppcOptionParam				= NULL;
		}
		else if(STIRCMP(argv[argIdx], "-erase"))
		{
			pstrWriteOpt->bEraseBeforeWrite	= 1;
			isParametricOption				= 0;
			ppcOptionParam					= NULL;
		}
		else if(STIRCMP(argv[argIdx], "-nowait"))
		{
			pstrWriteOpt->u8PauseAfterFinish	= 0;
			isParametricOption					= 0;
			ppcOptionParam						= NULL;
		}
		else if (STIRCMP(argv[argIdx], "-aardvark"))
		{
			ppcOptionParamInt = &pstrWriteOpt->req_serial_number;
		}
		else if (STIRCMP(argv[argIdx], "-port"))
		{
			ppcOptionParamInt = &pstrWriteOpt->req_serial_number;
		}
		else
		{
			printf("(ERR)Invalid Cmd Line Arg <%s>\n", argv[argIdx]);
			ret = 1;
			break;
		}

		argIdx ++;
		if(isParametricOption)
		{
			if(argIdx >= argc)
			{
				printf("(ERR) Missing Parameter For Option <%s>\n", argv[argIdx - 1]);
				ret = 1;
				break;
			}
			if (ppcOptionParamInt)
			{
				*ppcOptionParamInt = atol(argv[argIdx++]);
			}
			else
			{
				*ppcOptionParam = argv[argIdx++];
			}
		}
	}
	return ret;
}

/**************************************************************/
static int ParseREADCmdArgs(int argc, char* argv[], tstrReadOptions *pstrReadOpt)
{
	int		ret		= 0;
	int		argIdx;
	char	**ppcOptionParam;
	int		isParametricOption;
	uint32	*ppcOptionParamInt;

	pstrReadOpt->enuCertStore	= TLS_STORE_FLASH;

	for(argIdx = 0; argIdx < argc ; )
	{
		isParametricOption = 0;
		ppcOptionParam = NULL;
		ppcOptionParamInt = NULL;

		if(STIRCMP(argv[argIdx], "-rsa"))
		{
			pstrReadOpt->bIsRsa		= 1;
		}
		else if(STIRCMP(argv[argIdx], "-ecdsa"))
		{
			pstrReadOpt->bIsEcdsa	= 1;
		}
		else if(STIRCMP(argv[argIdx], "-dir"))
		{
			pstrReadOpt->bIsListFiles	= 1;
		}
		else if(STIRCMP(argv[argIdx], "-fwimg"))
		{
			isParametricOption = 1;
			ppcOptionParam = &pstrReadOpt->pcFwPath;
			pstrReadOpt->enuCertStore = TLS_STORE_FW_IMG;
		}
		else if (STIRCMP(argv[argIdx], "-pf_bin"))
		{
			//ppcOptionParam = &pstrReadOpt->pcProgFw;
			isParametricOption					= 0;
		}
		else if(STIRCMP(argv[argIdx], "-out"))
		{
			pstrReadOpt->bWriteToFile	= 1;
			isParametricOption			= 1;
			ppcOptionParam				= &pstrReadOpt->pcOutPath;
		}
		else if(STIRCMP(argv[argIdx],"-vflash_path"))
		{
			isParametricOption			= 1;
			ppcOptionParam              = &pstrReadOpt->pcflash_path;
		}
		else if(STIRCMP(argv[argIdx], "-all"))
		{
			pstrReadOpt->bIsDumpAll	= 1;
		}
		else if(STIRCMP(argv[argIdx], "-privkey"))
		{
			pstrReadOpt->bPrintPriv	= 1;
		}
		else if (STIRCMP(argv[argIdx], "-aardvark"))
		{
			//ppcOptionParamInt = &pstrReadOpt->req_serial_number;
			isParametricOption					= 0;
		}
		else if (STIRCMP(argv[argIdx], "-port"))
		{
			ppcOptionParamInt = &pstrReadOpt->req_serial_number;
		}
		else
		{
			printf("(ERR)Invalid Cmd Line Arg <%s>\n", argv[argIdx]);
			ret = 1;
			break;
		}

		argIdx ++;
		if(isParametricOption)
		{
			if(argIdx >= argc)
			{
				printf("(ERR) Missing Parameter For Option <%s>\n", argv[argIdx - 1]);
				ret = 1;
				break;
			}
			if (ppcOptionParamInt)
			{
				*ppcOptionParamInt = atoi(argv[argIdx++]);
			}
			else
			{
                *ppcOptionParam = argv[argIdx ++];
			}
		}
	}
	return ret;
}

/**************************************************************/
int parseCmdLineArgs(int argc, char* argv[], tstrCmdLineOptions *pstrOptList)
{
	int	ret = 1;

	if(pstrOptList == NULL)
		return 1;

	if(argc < 2)
	{
		printUsage();
	}
	else
	{
		/* Defaults.
		*/
		memset(pstrOptList, 0, sizeof(tstrCmdLineOptions));
		ret = 0;

		/*
			First argument is always a command. Current defined commands are:
			1. Write.
			2. Read.
		*/
		if(STIRCMP(argv[1], "write"))
		{
			pstrOptList->enuCmd = CMD_WRITE;
			if(argv[2]==NULL || STIRCMP(argv[2], "-help"))
			{
				PrintHelp(pstrOptList->enuCmd);
				ret = 1;
			}
			else
			{
				ret = ParseWRITECmdArgs(argc - 2, &argv[2], &pstrOptList->strWriteOptions);
			}
		}
		else if(STIRCMP(argv[1], "read"))
		{
			pstrOptList->enuCmd			= CMD_READ;
			if(argv[2] == NULL || STIRCMP(argv[2], "-help"))
			{
				PrintHelp(pstrOptList->enuCmd);
				ret = 1;
			}
			else
			{
				ret = ParseREADCmdArgs(argc - 2, &argv[2], &pstrOptList->strReadOptions);
			}
		}
		else
		{
			printf("Unknown Command %s\n", argv[1]);
			ret = 1;
		}

	}
	return ret;
}

/**************************************************************/
static sint8 TlsCertStoreSaveToFlash(uint8 *pu8TlsSrvFlashSecContent, uint8 u8PortNum, uint8* vflash)
{
	sint8	s8Ret = M2M_ERR_FAIL;

	if(programmer_init(&u8PortNum, 0) == M2M_SUCCESS)
	{
        dump_flash("Before_tls.bin");

		if(programmer_erase(M2M_TLS_SERVER_FLASH_OFFSET, M2M_TLS_SERVER_FLASH_SIZE, vflash) == M2M_SUCCESS)
		{
			s8Ret = programmer_write(pu8TlsSrvFlashSecContent, M2M_TLS_SERVER_FLASH_OFFSET, M2M_TLS_SERVER_FLASH_SIZE, vflash);
		}

        dump_flash("After_tls.bin");

		programmer_deinit();
	}
	return s8Ret;
}

/**************************************************************/
static sint8 TlsCertStoreLoadFromFlash(uint8 u8PortNum)
{
	sint8	s8Ret = M2M_ERR_FAIL;

	if(programmer_init(&u8PortNum, 0) == M2M_SUCCESS)
	{
		s8Ret = programmer_read(gau8TlsSrvSec, M2M_TLS_SERVER_FLASH_OFFSET, M2M_TLS_SERVER_FLASH_SIZE);
		programmer_deinit();
	}
	return s8Ret;
}

/**************************************************************/
static sint8 TlsCertStoreSaveToFwImage(uint8 *pu8TlsSrvFlashSecContent, char *pcFwFile)
{
	FILE	*fp;
	sint8	s8Ret	= M2M_ERR_FAIL;

	fp = fopen(pcFwFile, "rb+");
	if(fp)
	{
		fseek(fp, M2M_TLS_SERVER_FLASH_OFFSET, SEEK_SET);
		fwrite(pu8TlsSrvFlashSecContent, 1, M2M_TLS_SERVER_FLASH_SIZE, fp);
		fclose(fp);
		s8Ret = M2M_SUCCESS;
	}
	else
	{
		printf("(ERR)Cannot Open Fw image <%s>\n", pcFwFile);
	}
	return s8Ret;
}

/**************************************************************/
static sint8 TlsCertStoreLoadFromFwImage(char *pcFwFile)
{
	FILE	*fp;
	sint8	s8Ret	= M2M_ERR_FAIL;

	fp = fopen(pcFwFile, "rb");
	if(fp)
	{
		fseek(fp, M2M_TLS_SERVER_FLASH_OFFSET, SEEK_SET);
		fread(gau8TlsSrvSec, 1, M2M_TLS_SERVER_FLASH_SIZE, fp);
		fclose(fp);
		s8Ret = M2M_SUCCESS;
	}
	else
	{
		printf("(ERR)Cannot Open Fw image <%s>\n", pcFwFile);
	}
	return s8Ret;
}

/**************************************************************/
static sint8 TlsCertStoreSave(tenuTLSCertStoreType enuStore, char *pcFwFile, uint8 port, uint8* vflash)
{
	sint8	ret = M2M_ERR_FAIL;

	switch(enuStore)
	{
	case TLS_STORE_FLASH:
		ret = TlsCertStoreSaveToFlash(gau8TlsSrvSec, port, vflash);
		break;

	case TLS_STORE_FW_IMG:
		ret = TlsCertStoreSaveToFwImage(gau8TlsSrvSec, pcFwFile);
		break;

	default:
		break;
	}

	if(ret == M2M_SUCCESS)
	{
		printf("TLS Certificate Store Update Success on %s\n", (enuStore == TLS_STORE_FLASH) ? "Flash" : "Firmware Image");
	}
	else
	{
		printf("TLS Certificate Store Update FAILED !!! on %s\n", (enuStore == TLS_STORE_FLASH) ? "Flash" : "Firmware Image");
	}
	return ret;
}

/**************************************************************/
sint8 TlsCertStoreLoad(tenuTLSCertStoreType enuStore, char *pcFwFile, uint8 port, uint8* vflash)
{
	sint8	ret = M2M_ERR_FAIL;

	switch(enuStore)
	{
	case TLS_STORE_FLASH:
		ret = TlsCertStoreLoadFromFlash(port);
		break;

	case TLS_STORE_FW_IMG:
		ret = TlsCertStoreLoadFromFwImage(pcFwFile);
		break;

	default:
		break;
	}
	return ret;
}

/**************************************************************/
int main(int argc, char* argv[])
{
	int					ret		= 1;
	uint8				bPause	= 0;
	tenuWriteMode		enuMode;
	tstrCmdLineOptions	strCmdLineOpt;
	uint32				u32TlsSrvSecSz;

	printName();
	if(!parseCmdLineArgs(argc, argv, &strCmdLineOpt))
	{
		if(strCmdLineOpt.enuCmd == CMD_WRITE)
		{
			/*
				Do a WRITE command.
			*/
			tstrWriteOptions	*pstrWRITE = &strCmdLineOpt.strWriteOptions;
            uint8* vflash = NULL;
            long szvflash = 0;

			if(pstrWRITE->pcServerCertFile == NULL)
			{
				printf("Server Certificate File MUST Be Supplied\n");
				bPause = 1;
				goto __EXIT;
			}
			if((pstrWRITE->pcPrivKeyFile == NULL) && (pstrWRITE->bIsKeyAvail))
			{
				printf("Server Private Key File MUST Be Supplied\n");
				bPause = 1;
				goto __EXIT;
			}

			if(pstrWRITE->pcflash_path)
            {
                FILE * fp;

                fp = fopen(pstrWRITE->pcflash_path,"rb");
                if (fp)
                {
                    fseek (fp, 0, SEEK_END);   // non-portable
                    szvflash=ftell (fp);
                    rewind(fp);

                    vflash = malloc(szvflash);
                    if(vflash)
                    {
						memset(vflash,0xFF,szvflash);
                        M2M_PRINT("Reading vflash from %s\n", pstrWRITE->pcflash_path);
                        fread(vflash,1,szvflash,fp);
                    }
                    fclose (fp);
                }
            }

			if(pstrWRITE->bEraseBeforeWrite)
			{
				/*
					Clean write after erasing the current TLS Certificate section contents.
				*/
				enuMode = TLS_SRV_SEC_MODE_WRITE;
			}
			else
			{
				/*
					Write to the end of the current TLS Certificate section.
				*/
				enuMode = TLS_SRV_SEC_MODE_APPEND;
				if(TlsCertStoreLoad(pstrWRITE->enuCertStore, pstrWRITE->pcFwPath, pstrWRITE->req_serial_number, vflash) != M2M_SUCCESS)
				{
					bPause = 1;
					goto __EXIT;
				}
			}

			/*
				Modify the TLS Certificate Store Contents.
			*/
			ret = TlsCertStoreWriteCertChain(pstrWRITE->pcPrivKeyFile, pstrWRITE->pcServerCertFile, pstrWRITE->pcCADir, gau8TlsSrvSec, &u32TlsSrvSecSz, enuMode);
			if(ret == M2M_SUCCESS)
			{
				/*
					Write the TLS Certificate Section buffer to the chosen destination,
					either to the firmware image or the WINC stacked flash directly.
				*/
				ret = TlsCertStoreSave(pstrWRITE->enuCertStore, pstrWRITE->pcFwPath, pstrWRITE->req_serial_number, vflash);
			}
			bPause = pstrWRITE->u8PauseAfterFinish;

            if(vflash)
            {
                FILE *fp;
                fp = fopen(pstrWRITE->pcflash_path,"wb");
                if(fp != NULL)
                {
                    M2M_PRINT("Saving vflash to %s\n", pstrWRITE->pcflash_path);
                    fwrite(vflash,1,szvflash,fp);
                    fclose(fp);
                }
            }

		}
		else if(strCmdLineOpt.enuCmd == CMD_READ)
		{
			/*
				Do a READ command.
			*/
			tstrReadOptions		*pstrREAD = &strCmdLineOpt.strReadOptions;
			if((pstrREAD->bWriteToFile) && (pstrREAD->pcOutPath == NULL))
			{
				printf("Please specify output path for dumping certificate files\n");
				bPause = 1;
				goto __EXIT;
			}

			ret = TlsCertStoreLoad(pstrREAD->enuCertStore, pstrREAD->pcFwPath, pstrREAD->req_serial_number, NULL);
			if(ret != M2M_SUCCESS)
			{
				bPause = 1;
				goto __EXIT;
			}

			/*
				Load the TLS Certificate Store into memory
			*/
			TlsSrvSecReadInit(gau8TlsSrvSec);
			TlsSrvSecDumpContents(pstrREAD->bIsRsa, pstrREAD->bIsEcdsa, pstrREAD->bPrintPriv, pstrREAD->bIsDumpAll, pstrREAD->bIsListFiles, pstrREAD->bWriteToFile, pstrREAD->pcOutPath);
			bPause = 1;
		}
		else
		{
			bPause = 1;
		}
	}
	else
	{
		bPause = 1;
	}

__EXIT:

	if(bPause)
	{
		system("pause");
	}
	return ret;
}

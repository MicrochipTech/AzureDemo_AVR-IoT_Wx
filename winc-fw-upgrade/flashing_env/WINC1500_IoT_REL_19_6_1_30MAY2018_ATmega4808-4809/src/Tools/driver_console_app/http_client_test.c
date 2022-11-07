/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "http_client/http_client.h"
#include "socket\include\socket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

typedef struct{
	HTTPClientHandle	hHTTPClientHandle;
	uint32				u32RxCount;
	FILE				*fpHttpFile;
	char				acFileName[256];
}tstrHTTPSession;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define N_HTTPS_FILES									(sizeof(gpacHTTPSFileList)/ 256)

#define __REPEAT_TEST_CONTINUOUSLY__
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBALS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

static volatile uint8		gbIsHTTPSSessionRunning	= 0;
static volatile	uint8		gbIsTestFinished		= 0;
static tstrHTTPSession		gstrHTTPSSession;

#define LAN_SERVER
#define LAN_SERVER_ADDR		"192.168.0.101"

#ifdef LAN_SERVER
static char					gpacHTTPSFileList[][256] = 
{
	"https://" LAN_SERVER_ADDR ":4402/"
};
#else
static char					gpacHTTPSFileList[][256] = 
{
	"https://www.facebook.com/"
};
#endif

/*********************************************************************
Function


Description


Return


Author
	Ahmed Ezzat

Version
	1.0

Date

*********************************************************************/
void http_client_test_init(void)
{
	memset(&gstrHTTPSSession, 0, sizeof(tstrHTTPSession));
	gbIsHTTPSSessionRunning	= 0;
	gbIsTestFinished		= 0;
	sslEnableCertExpirationCheck(0);
	HTTP_ClientInit();
}
/*********************************************************************
Function


Description


Return


Author
	Ahmed Ezzat

Version
	1.0

Date

*********************************************************************/
static sint8 httpsClientCallback(uint8 *pu8Chunk, sint16 s16ChunkSize, uint32 u32Arg)
{
	tstrHTTPSession		*pstrSession = (tstrHTTPSession*)u32Arg;

	if(pu8Chunk != NULL)
	{
		if(pstrSession->fpHttpFile != NULL)
		{
			fwrite(pu8Chunk, 1, s16ChunkSize, pstrSession->fpHttpFile);
			fflush(pstrSession->fpHttpFile);
		}
	}
	else
	{
		if(s16ChunkSize == HTTP_CLIENT_DOWNLOAD_STARTING)
		{
			char	acFullName[256] = ".\\Download\\";
			char	acTmp[256];
			char	*pcToken, *pcFileName = NULL;
			char	defaultFile[] = "index.html";

			/* Open file for writing the received content.
			*/
			strcpy(acTmp, pstrSession->acFileName);
			pcToken = strtok(acTmp, "/");
			while(pcToken != NULL)
			{
				if(*pcToken)
					pcFileName = pcToken;
				pcToken = strtok(NULL, "/");
			}
			if(pcFileName == NULL)
			{
				pcFileName = defaultFile;
			}
			strcpy(pstrSession->acFileName, pcFileName);
			strcat(acFullName, pcFileName);
			pstrSession->fpHttpFile = fopen(acFullName, "wb");
		}
		else if(s16ChunkSize == HTTP_CLIENT_DOWNLOAD_COMPLETE)
		{
			M2M_INFO("\"%s\" Success\n", pstrSession->acFileName);
			if(pstrSession->fpHttpFile != NULL)
			{
				fclose(pstrSession->fpHttpFile);
				pstrSession->fpHttpFile = NULL;
			}
			HTTP_ClientStop(pstrSession->hHTTPClientHandle);
			gbIsHTTPSSessionRunning = 0;
		}
		else if(s16ChunkSize == HTTP_CLIENT_CONNECTION_ERROR)
		{
			M2M_INFO("\"%s\" Fail!\n", pstrSession->acFileName);

			if(pstrSession->fpHttpFile != NULL)
			{
				fclose(pstrSession->fpHttpFile);
				pstrSession->fpHttpFile = NULL;
			}

			HTTP_ClientStop(pstrSession->hHTTPClientHandle);
			gbIsHTTPSSessionRunning = 0;
//			gbIsTestFinished = 1;
		}
	}
	return 0;
}
/*********************************************************************
Function


Description


Return


Author
	Ahmed Ezzat

Version
	1.0

Date

*********************************************************************/
void http_test_task(uint8 u8ForceTestStart)
{
	static uint32	u32Iteration = 0;
	static uint32	u32ServerIdx = 0;
	
	if(u8ForceTestStart)
		gbIsTestFinished = 0;

	if(!gbIsTestFinished)
	{
		if(!gbIsHTTPSSessionRunning)
		{
			if(u32ServerIdx < N_HTTPS_FILES)
			{
				gbIsHTTPSSessionRunning		= 1;
				gstrHTTPSSession.u32RxCount = 0;
				strcpy(gstrHTTPSSession.acFileName, gpacHTTPSFileList[u32ServerIdx]);
				gstrHTTPSSession.hHTTPClientHandle = HTTP_ClientDownload(gpacHTTPSFileList[u32ServerIdx], httpsClientCallback, 
					(uint32)&gstrHTTPSSession);
				u32ServerIdx ++;
			}
			else
			{
#ifndef __REPEAT_TEST_CONTINUOUSLY__
				M2M_INFO("Test Finished\n");
				gbIsTestFinished = 1;
#else

#if 0
				u32Iteration ++;
				switch(u32Iteration % 3)
				{
				case 0:
					sslEnableCertExpirationCheck(SSL_CERT_EXP_CHECK_DISABLE);
					break;

				case 1:
					sslEnableCertExpirationCheck(SSL_CERT_EXP_CHECK_ENABLE);
					break;

				case 2:
					sslEnableCertExpirationCheck(SSL_CERT_EXP_CHECK_EN_IF_SYS_TIME);
					break;
				}
				if(u32Iteration == 3)
					gbIsTestFinished = 1;
#endif
#endif
				u32ServerIdx = 0;
			}
		}
	}
}
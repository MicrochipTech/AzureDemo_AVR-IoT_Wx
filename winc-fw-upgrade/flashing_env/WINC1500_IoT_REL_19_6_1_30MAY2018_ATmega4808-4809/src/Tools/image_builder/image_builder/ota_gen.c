#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include "builder.h"
#include "ota_hdr.h"
#include "firmware_addresses.h"
#include "crypto_lib_api.h"


extern sint8 write_to_file(uint8 *pu8Buf, uint32 u32Offset, uint32 u32Sz);


int PrepareOta(uint8 *pu8OTAFw, uint32 u32OtaFwSize, char * pcOTAFile)
{
	sint8			ret = M2M_SUCCESS;
	tstrOtaInitHdr	strOtaInitHdr	= {0};
	FILE			*fp;

	if((pcOTAFile != NULL) && (pu8OTAFw != NULL))
	{
		uint8	au8Sha256Digest[CRYPTO_SHA256_DIGEST_SIZE];
		uint8	*pu8Img = (uint8*)malloc(sizeof(tstrOtaInitHdr) + u32OtaFwSize);
		if(pu8Img != NULL)
		{
			strOtaInitHdr.u32OtaMagicValue	= OTA_MAGIC_VALUE;
			strOtaInitHdr.u32OtaPayloadSize	= u32OtaFwSize + sizeof(tstrOtaInitHdr);
			
			memcpy(pu8Img, &strOtaInitHdr, sizeof(tstrOtaInitHdr));
			memcpy(&pu8Img[sizeof(tstrOtaInitHdr)], pu8OTAFw, u32OtaFwSize);
			CryptoSha256Hash(pu8Img, (sizeof(tstrOtaInitHdr) + u32OtaFwSize), au8Sha256Digest);

			fp = fopen(pcOTAFile, "wb");
			if(fp)
			{
				fwrite(au8Sha256Digest, 1, CRYPTO_SHA256_DIGEST_SIZE, fp);
				fwrite(&strOtaInitHdr, 1, sizeof(tstrOtaInitHdr), fp);
				fwrite(pu8OTAFw, 1, u32OtaFwSize, fp);
				fclose(fp);
			}
			else
			{
				M2M_ERR("\n Fail to open OTA File <%s> For Writing.\n", pcOTAFile);
				ret = M2M_ERR_FAIL;
			}
		}
	}
	else
	{
		ret = M2M_ERR_FAIL;
	}
	return ret;
}

int PrepOtaModifiedHttp(char * file)
{
	sint8 ret = M2M_SUCCESS;
	uint32 fileSize = 0;
	FILE	*fp;
	uint8 * pbuf = NULL;

	if(file != NULL)
	{
		fp = fopen(file, "rb");
		if(fp)
		{	
			fseek(fp,0,SEEK_END);
			fileSize = ftell(fp);
			fseek(fp,0,SEEK_SET);
			pbuf = malloc(fileSize + CRYPTO_SHA256_DIGEST_SIZE + sizeof(tstrOtaInitHdr));
			if(pbuf != NULL)
			{
				fread(pbuf, 1, fileSize, fp);
				///////// HASH ////////////
				CryptoSha256Hash((uint8*)&pbuf[CRYPTO_SHA256_DIGEST_SIZE], fileSize - CRYPTO_SHA256_DIGEST_SIZE, pbuf);
				///////////////////////////
				write_to_file(pbuf,0, CRYPTO_SHA256_DIGEST_SIZE);
				fclose(fp);
				free(pbuf);
			}
			else
			{
				M2M_ERR("Failed to alloc memory\n");
				ret = M2M_ERR_FAIL;
			}
		}
		else
		{
			M2M_ERR("\n Failed to open file \n");
			ret = M2M_ERR_FAIL;
		}
	}
	else
	{
		M2M_ERR("\n Failed to open file \n");
		ret = M2M_ERR_FAIL;
	}
	return ret;
}

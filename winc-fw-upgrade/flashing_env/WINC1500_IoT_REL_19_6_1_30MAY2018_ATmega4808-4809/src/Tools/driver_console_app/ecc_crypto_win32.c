/*!
@file
	ecc_crypto_win32.c

@brief	Elliptic Curve Cryptography Mathematics For Windows
*/

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "ecc_types.h"
#include "lm_spcomputation.h"
#include "driver/source/m2m_hif.h"
#include "socket/include/socket.h"

#define GETU32(BUF,OFFSET)				((((uint32)((BUF)[OFFSET]) << 24)) | (((uint32)((BUF)[OFFSET + 1]) << 16)) | \
										(((uint32)((BUF)[OFFSET + 2]) << 8)) | ((uint32)((BUF)[OFFSET + 3])))
/*!< 
	Retrieve 4 bytes from the given buffer at the given
	offset as 32-bit unsigned integer in the Network byte order.
*/


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBALS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

static tstrEllipticCurve	gastrECCSuppList[] = {
	{
		EC_SECP256R1,
		{
			{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF},
			{0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF},
			{0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0, 0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8},
			{
				{
					0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
					0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96
				},
				{
					0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
					0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5
				},
				32
			}
		}
	}
};
/*!<
	List of supported Elliptic Curves ordered by security level (most secure curve is at index ZERO).
*/

uint16		gu16SlotIdx = 1;
uint8		au8PrivKeySlots[5][32];


/************************************************************/
void getRand(uint8 *pu8buf, uint32 u32n)
{
	uint32 i;
	for(i = 0; i < u32n; i ++)
	{
		pu8buf[i] = rand() % 256;
		if(pu8buf[i] == 0)
			i --;
	}
}

/************************************************************/
void BI_ToBytes
(
uint32 	*pu32BigInt,
uint32 	u32BigIntSize, 
uint8 	*pu8Bytes,
uint32	u32BytesCount
)
{
	uint32		u32ByteIdx;
	sint32		s32WordIdx;

	u32ByteIdx	= 0;
	s32WordIdx	= u32BigIntSize - 1;
	
	m2m_memset(pu8Bytes, 0, u32BytesCount);
	if(u32BytesCount > (u32BigIntSize * 4))
	{
		u32ByteIdx = u32BytesCount - (u32BigIntSize * 4);
	}
	else if((u32BytesCount % 4) != 0)
	{
		uint32	u32Idx;
		uint32	u32ExtraBytes = u32BytesCount % 4;

		for(u32Idx = 0; u32Idx < u32ExtraBytes; u32Idx ++)
		{
			pu8Bytes[u32ByteIdx ++] = (pu32BigInt[s32WordIdx] >> ((u32ExtraBytes - u32Idx - 1) * 8));
		}
		s32WordIdx --;
	}
	for(; ((s32WordIdx >= 0) && u32ByteIdx < (u32BytesCount - 3)); s32WordIdx --)
	{
		pu8Bytes[u32ByteIdx ++] = BYTE_3(pu32BigInt[s32WordIdx]);
		pu8Bytes[u32ByteIdx ++] = BYTE_2(pu32BigInt[s32WordIdx]);
		pu8Bytes[u32ByteIdx ++] = BYTE_1(pu32BigInt[s32WordIdx]);
		pu8Bytes[u32ByteIdx ++] = BYTE_0(pu32BigInt[s32WordIdx]);
	}
}
/************************************************************/
void BI_FromBytes
(
uint8 	*pu8Bytes,
uint32 	u32BytesCount, 
uint32 	*pu32BigInt
)
{
	uint32	u32WordIdx;
	uint32	u32ByteIdx = 0;

	u32WordIdx	= ((u32BytesCount + 3) / 4) - 1;
	u32ByteIdx	= 0;

	if((u32BytesCount % 4) != 0)
	{
		uint32	u32Idx,u32ExtraBytes;

		u32ExtraBytes = u32BytesCount % 4;
		pu32BigInt[u32WordIdx] = 0;
		for(u32Idx = 0; u32Idx < u32ExtraBytes; u32Idx ++)
			pu32BigInt[u32WordIdx] |= (uint32)(pu8Bytes[u32ByteIdx ++] << ((u32ExtraBytes - u32Idx - 1) * 8));
		u32WordIdx --;
	}
	for(; u32ByteIdx < u32BytesCount ; u32ByteIdx += 4 , u32WordIdx --)
	{
		pu32BigInt[u32WordIdx] = GETU32(pu8Bytes,u32ByteIdx);
	}
}

/************************************************************/
static sint8 ECC_PointMul(tstrECPoint *P, uint8 *d, tstrECPoint *R)
{
	{
		lm_sp_p256_point	ZP;
		lm_sp_p256_point	ZR;
		lm_sp_num_256		Skb_naf_0;
		lm_sp_num_256		Skb_naf_1;
		lm_sp_num_256		K;

		K.Neg	= 0;
		BI_FromBytes(d, P->u16Size, K.Number);
		K.Number[7] = 0;

		m2m_memset((uint8*)&ZP, 0, sizeof(lm_sp_p256_point));
		ZP.Z.Number[0] = 1;

		BI_FromBytes(P->X, P->u16Size, ZP.X.Number);
		BI_FromBytes(P->Y, P->u16Size, ZP.Y.Number);

		atml_lm_sp_num256_NAF(&Skb_naf_0, &Skb_naf_1, &K);
		atml_lm_sp_p256_NAF_mul(&ZR, &ZP, &Skb_naf_0, &Skb_naf_1);
		lm_sp_p256_point_jacobian_to_affine(&ZR);

		m2m_memset((uint8*)R, 0, sizeof(tstrECPoint));
		R->u16Size = 32;

		BI_ToBytes((uint32*)ZR.X.Number, 8, R->X, R->u16Size);
		BI_ToBytes((uint32*)ZR.Y.Number, 8, R->Y, R->u16Size);
	}
	return 0;
}

/************************************************************/
static sint8 ecdhDeriveClientSharedSecret
(
tstrEllipticCurve	*pstrEc, 
tstrECPoint			*pstrSrvPubKey,
tstrECPoint			*pstrClientPub,
uint8				*pu8ECDHSharedSecret
)
{
	tstrECPoint	strECDHShared;
	uint8		d[32];

	getRand(d, 32);
	pstrSrvPubKey->u16Size = 32;
	ECC_PointMul(pstrSrvPubKey, d, &strECDHShared);
	ECC_PointMul(&pstrEc->strParam.G, d, pstrClientPub);
	m2m_memcpy(pu8ECDHSharedSecret, strECDHShared.X, pstrSrvPubKey->u16Size);
	return 0;
}

/************************************************************/
static sint8 ecdhDeriveKeyPair
(
tstrEllipticCurve	*pstrEc,
tstrECPoint			*pstrSrvPubKey
)
{
	sint8	ret			= M2M_ERR_FAIL;
	uint16	u16KeySlot	= gu16SlotIdx;
	uint8	*pu8PrivKey	= au8PrivKeySlots[u16KeySlot];
	
	getRand(pu8PrivKey, 32);

	if(ECC_PointMul(&pstrEc->strParam.G, pu8PrivKey, pstrSrvPubKey))
	{
		pstrSrvPubKey->u16Size		= 32;
		pstrSrvPubKey->u16PrivKeyID = u16KeySlot;

		gu16SlotIdx ++;
		if(gu16SlotIdx == 5)
			gu16SlotIdx = 0;

		ret = M2M_SUCCESS;
	}

	return ret;
}

/************************************************************/
static sint8 ecdhDeriveServerSharedSecret
(
tstrEllipticCurve	*pstrEc,
uint16				u16PrivKeyID,
tstrECPoint			*pstrClientPub,
uint8				*pu8ECDHSharedSecret
)
{
	uint8		*pu8PrivKey	= au8PrivKeySlots[u16PrivKeyID];
	tstrECPoint	strECDHShared;

	ECC_PointMul(pstrClientPub, pu8PrivKey, &strECDHShared);
	m2m_memcpy(pu8ECDHSharedSecret, strECDHShared.X, pstrEc->strParam.G.u16Size);
	return 0;
}

/************************************************************/
static void eccProcessREQ(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
{
	tstrECCEventData	strEccREQ, strECCResp;
	strECCResp.u16Status	= 1;
	M2M_INFO("ECC\n");
	if(hif_receive(u32Addr, (uint8*)&strEccREQ, sizeof(tstrECCEventData), 1) == M2M_SUCCESS)
	{
		switch(strEccREQ.u16REQ)
		{
		case ECC_EVENT_CLIENT_ECDH:
			strECCResp.u16Status = ecdhDeriveClientSharedSecret(&gastrECCSuppList[0], &strEccREQ.strPubKey, &strECCResp.strPubKey, strECCResp.au8Key);
			break;

		case ECC_EVENT_GEN_KEY:
			strECCResp.u16Status = ecdhDeriveKeyPair(&gastrECCSuppList[0], &strECCResp.strPubKey);
			break;

		case ECC_EVENT_SERVER_ECDH:
			strECCResp.u16Status = ecdhDeriveServerSharedSecret(&gastrECCSuppList[0], strEccREQ.strPubKey.u16PrivKeyID, &strEccREQ.strPubKey, strECCResp.au8Key);
			break;
		}

		strECCResp.u16REQ		= strEccREQ.u16REQ;
		strECCResp.u32UserData	= strEccREQ.u32UserData;
		strECCResp.u32SeqNo		= strEccREQ.u32SeqNo;
	}
	hif_send(M2M_REQ_GROUP_SSL, M2M_SSL_RESP_ECC|M2M_REQ_DATA_PKT, (uint8*)&strECCResp, sizeof(tstrECCEventData), NULL, 0, 0);
}

/************************************************************/
void eccInit(void)
{
	hif_register_cb(M2M_REQ_GROUP_SSL, eccProcessREQ);
	m2m_ssl_set_active_ciphersuites(SSL_ENABLE_ALL_SUITES);
	gu16SlotIdx	= 0;
}
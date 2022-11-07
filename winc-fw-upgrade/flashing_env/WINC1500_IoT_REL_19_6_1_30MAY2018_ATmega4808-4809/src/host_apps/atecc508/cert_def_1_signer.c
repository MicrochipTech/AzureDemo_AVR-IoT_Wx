#include "atcacert/atcacert_def.h"

#define AUTH_KEY_ID_SIZE		20
#define AUTH_KEY_ID_OFFSET		395

const uint8_t g_signer_1_ca_public_key[64] = {
    0xF9, 0x29, 0x04, 0xF3, 0xBF, 0xD0, 0x6C, 0x5C,  0x42, 0x02, 0x4C, 0xC3, 0x5E, 0x04, 0x8B, 0x3F,
    0xA3, 0xC7, 0xC3, 0xC8, 0x07, 0xC6, 0x6F, 0xA9,  0xE4, 0x18, 0x42, 0x1E, 0x34, 0x0F, 0x94, 0x7C,
    0xBD, 0x4D, 0xDF, 0x52, 0x6F, 0xD9, 0x5A, 0xA0,  0x55, 0x7E, 0x48, 0x08, 0xAC, 0x11, 0xFB, 0x85,
    0x50, 0x09, 0xCF, 0x59, 0xDB, 0x4F, 0x05, 0x0B,  0x4D, 0x3D, 0x67, 0xAA, 0xE9, 0x97, 0xC4, 0x91
};

const uint8_t g_cert_template_1_signer[] = {
    0x30, 0x82, 0x01, 0xF0, 0x30, 0x82, 0x01, 0x97,  0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x11, 0x69,
    0x8A, 0x50, 0x0D, 0x71, 0xA3, 0xFC, 0x37, 0xA5,  0xB8, 0x1D, 0x44, 0x05, 0xBA, 0x2F, 0xC4, 0x01,
    0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,  0x3D, 0x04, 0x03, 0x02, 0x30, 0x43, 0x31, 0x1D,
    0x30, 0x1B, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C,  0x14, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65,
    0x20, 0x4F, 0x72, 0x67, 0x61, 0x6E, 0x69, 0x7A,  0x61, 0x74, 0x69, 0x6F, 0x6E, 0x31, 0x22, 0x30,
    0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x19,  0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20,
    0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38,  0x41, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43,
    0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x31,  0x32, 0x31, 0x37, 0x32, 0x33, 0x30, 0x30, 0x30,
    0x30, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39,  0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39,
    0x35, 0x39, 0x5A, 0x30, 0x47, 0x31, 0x1D, 0x30,  0x1B, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x14,
    0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20,  0x4F, 0x72, 0x67, 0x61, 0x6E, 0x69, 0x7A, 0x61,
    0x74, 0x69, 0x6F, 0x6E, 0x31, 0x26, 0x30, 0x24,  0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x1D, 0x45,
    0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x41,  0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41,
    0x20, 0x53, 0x69, 0x67, 0x6E, 0x65, 0x72, 0x20,  0x30, 0x30, 0x33, 0x34, 0x30, 0x59, 0x30, 0x13,
    0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02,  0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
    0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x44,  0x0A, 0xE6, 0xC5, 0x5E, 0x3D, 0xD5, 0xBE, 0x2A,
    0xCE, 0x49, 0x00, 0x4B, 0x1A, 0x8D, 0xF3, 0x01,  0x1B, 0x2C, 0x57, 0x26, 0x86, 0xE1, 0x95, 0x9F,
    0xF1, 0x5C, 0x71, 0x18, 0x06, 0x8E, 0xFC, 0x6D,  0xB1, 0x1B, 0x05, 0x8C, 0xE2, 0xBD, 0xEF, 0x96,
    0xD9, 0x54, 0x53, 0x09, 0x81, 0x57, 0xB9, 0xF2,  0x8A, 0x90, 0x40, 0x8B, 0x55, 0x70, 0x83, 0x52,
    0xA6, 0xEB, 0x5A, 0x8B, 0x7F, 0xC1, 0x74, 0xA3,  0x66, 0x30, 0x64, 0x30, 0x12, 0x06, 0x03, 0x55,
    0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x08, 0x30,  0x06, 0x01, 0x01, 0xFF, 0x02, 0x01, 0x00, 0x30,
    0x0E, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x01, 0x01,  0xFF, 0x04, 0x04, 0x03, 0x02, 0x02, 0x84, 0x30,
    0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16,  0x04, 0x14, 0x04, 0xCD, 0xE7, 0x19, 0x2E, 0x83,
    0x65, 0xA4, 0xC5, 0x3B, 0xAE, 0xA9, 0x8C, 0xAC,  0xD2, 0x1C, 0xAF, 0xFB, 0xCF, 0x2C, 0x30, 0x1F,
    0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30,  0x16, 0x80, 0x14, 0x37, 0x0A, 0xA1, 0x3A, 0xEB,
    0xEB, 0xCF, 0x09, 0x89, 0x70, 0x82, 0x6B, 0x3A,  0xB2, 0x74, 0xFB, 0x96, 0x72, 0x68, 0xD3, 0x30,
    0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,  0x04, 0x03, 0x02, 0x03, 0x47, 0x00, 0x30, 0x44,
    0x02, 0x20, 0x77, 0x5E, 0x46, 0x8A, 0xA4, 0x72,  0x23, 0xE8, 0x26, 0x15, 0x9F, 0x1F, 0xF2, 0x71,
    0xE5, 0x0B, 0x73, 0x4A, 0xA8, 0x99, 0xF4, 0xC4,  0xFE, 0x56, 0x65, 0xA7, 0xE6, 0xF7, 0x0A, 0x0B,
    0xE2, 0xCB, 0x02, 0x20, 0x1A, 0x98, 0x24, 0xEC,  0xD0, 0x51, 0x0D, 0x98, 0xD2, 0x1F, 0xE9, 0x88,
    0x83, 0x4C, 0x1F, 0x5F, 0x75, 0xCB, 0x97, 0xD3,  0x0C, 0x53, 0x58, 0xFB, 0x58, 0x30, 0x0A, 0xAC,
    0x1B, 0x45, 0xBA, 0x1D
};

const atcacert_cert_element_t g_cert_elements_1_signer[] = {
	{
		.id         = "aid",
		.device_loc = {
			.zone      = DEVZONE_DATA,
			.slot      = 8,
			.is_genkey = 0,
			.offset    = 176,
			.count     = AUTH_KEY_ID_SIZE
		},
		.cert_loc   = {
			.offset = AUTH_KEY_ID_OFFSET,
			.count  = AUTH_KEY_ID_SIZE
		}
	}
};

const atcacert_def_t g_cert_def_1_signer = {
    .type                   = CERTTYPE_X509,
    .template_id            = 1,
    .chain_id               = 0,
    .private_key_slot       = 0,
    .sn_source              = SNSRC_PUB_KEY_HASH,
    .cert_sn_dev_loc        = { 
        .zone      = DEVZONE_NONE,
        .slot      = 0,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 0
    },
    .issue_date_format      = DATEFMT_RFC5280_UTC,
    .expire_date_format     = DATEFMT_RFC5280_GEN,
    .tbs_cert_loc           = {
        .offset = 4,
        .count  = 411
    },
    .expire_years           = 0,
    .public_key_dev_loc     = {
        .zone      = DEVZONE_DATA,
        .slot      = 11,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 72
    },
    .comp_cert_dev_loc      = {
        .zone      = DEVZONE_DATA,
        .slot      = 12,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 72
    },
    .std_cert_elements      = {
        { // STDCERT_PUBLIC_KEY
            .offset = 247,
            .count  = 64
        },
        { // STDCERT_SIGNATURE
            .offset = 427,
            .count  = 74
        },
        { // STDCERT_ISSUE_DATE
            .offset = 117,
            .count  = 13
        },
        { // STDCERT_EXPIRE_DATE
            .offset = 132,
            .count  = 15
        },
        { // STDCERT_SIGNER_ID
            .offset = 216,
            .count  = 4
        },
        { // STDCERT_CERT_SN
            .offset = 15,
            .count  = 16
        },
        { // STDCERT_AUTH_KEY_ID
            .offset = AUTH_KEY_ID_OFFSET,
            .count  = AUTH_KEY_ID_SIZE
        },
        { // STDCERT_SUBJ_KEY_ID
            .offset = 362,
            .count  = 20
        }
    },
    .cert_elements          = g_cert_elements_1_signer,
    .cert_elements_count    = sizeof(g_cert_elements_1_signer) / sizeof(g_cert_elements_1_signer[0]),
    .cert_template          = g_cert_template_1_signer,
    .cert_template_size     = sizeof(g_cert_template_1_signer),
};


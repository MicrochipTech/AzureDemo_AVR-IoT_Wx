#ifndef __BUILDER_H__
#define __BUILDER_H__

/**
* Include
*/
#include "programmer.h"
#include "../efuse/efuse.h"
#include "programmer.h"
#include "ota_hdr.h"
/**
* Windows specific for threads
*/
#include "stdio.h"


/* Definitions */
#define NM_BSP_PERM_FIRMWARE_SIZE	(512*1024)

/*
	TLS Root Certificates Store
*/
#define ROOT_CERT_PATH		"../../../Tools/root_certificate_downloader/binary/"

/*
	TLS Certificate Store
*/
#define TLS_CERT_STORE		"../../../tls_cert_store"
#define TLS_CA_DIR			TLS_CERT_STORE "/CA"
#define TLS_RSA_PRIV_KEY	TLS_CERT_STORE "/winc_rsa.key"
#define TLS_RSA_CERT		TLS_CERT_STORE "/winc_rsa.cer"
#define TLS_ECDSA_CERT		TLS_CERT_STORE "/winc_ecdsa.cer"

/*
	Define one (or both) of the following macros based on the required TLS
	Authentication type for the WINC Device (for TLS Client Authentication
	and/or TLS Server operation).
	If no certificates are required to be installed, disable both.
*/
#undef __TLS_INSTALL_ECDSA_CRT__
#define __TLS_INSTALL_RSA_CRT__



/*
	HTTP Provisioning Server Home Page Files
*/
#define HTTP_FILE_PATH		"../../../firmware/wifi_v111/src/nmi_m2m/source/http/Server/config/"
#define HTTP_DEFAULT_FILE	"default.html"
#define HTTP_STYLE_FILE		"style.css"
#define HTTP_FAVICON_FILE	"favicon.ico"
#define HTTP_LOGO_FILE		"logo.png"

#define TRUE	1
#define FALSE	0

/* Structures and Enums */
typedef struct{
	uint32	addr;
	/*!< Address */
	uint32	size;
	/*!< Size */
	char*	type;
	/*!< Type */
} tstrSpiFlash;


typedef struct{
	uint32	WIFIData_space;     //!< Space allocated in image for WIFI connection data
	uint32	WIFIData_used;      //!< Space used in image for WIFI connection data
	uint32	WIFIData_ofs;       //!< Offset in image for WIFI connection data

	uint32	BootFW_space;       //!< Space allocated in image for Boot FW
	uint32	BootFW_used;        //!< Space used in image for Boot FW
	uint32	BootFW_ofs;         //!< Offset in image for Boot FW

	uint32	PLL_space;          //!< Space allocated in image for Config Sector
	uint32	PLL_used;           //!< Space used in image for Config Sector
	uint32	PLL_ofs;            //!< Offset in image for Config Sector

	uint32	Gain_space;         //!< Space allocated in image for Config Sector
	uint32	Gain_used;          //!< Space used in image for Config Sector
	uint32	Gain_ofs;           //!< Offset in image for Config Sector

	uint32	Control_space;      //!< Space allocated in image for Control Sector
	uint32	Control_used;       //!< Space used in image for Control Sector
	uint32	Control_ofs;        //!< Offset in image for Control Sector

	uint32	FW_space;           //!< Space allocated in image for FW
	uint32	FW_used;            //!< Space used in image for FW
	uint32	FW_ofs;             //!< Offset in image for FW

	uint32	HTTP_space;         //!< Space allocated in image for HTTP data
	uint32	HTTP_used;          //!< Space used in image for HTTP data
	uint32	HTTP_ofs;           //!< Offset in image for HTTP data

	uint32	BT_space;           //!< Space allocated in image for BT FW
	uint32	BT_used;            //!< Space used in image for BT FW
	uint32	BT_ofs;             //!< Offset in image for BT FW

	uint32	Total_size;         //!< of flash
} tstrManifest;
tstrManifest* pManifest;

/* Functions */
sint8 builder_init(uint8 *fileName,char * pcFileOption);
sint8 builder_deinit(void);

sint8 get_firmwarefile_version(char * file, tstrM2mRev *pstrm2mrev);

sint8 build_boot_firmware(tstrM2mRev *pstrm2mrev);
sint8 build_control_sec(tstrM2mRev *pstrm2mrev,uint8 Chip);
sint8 build_firmware(tstrM2mRev *pstrm2mrev,char * file,char * otafilename,uint8 Chip,uint8 ota);
sint8 build_http_config(tstrM2mRev *pstrm2mrev,char *pcSrcFolder, uint8 u8ModifyForOta);
sint8 build_app(void);
sint8 build_look_up_table(tstrM2mRev *pstrm2mrev,double xoOffset);
sint8 build_certificates(tstrM2mRev *pstrm2mrev, uint8* vflash);
sint8 build_tls_server_flash(void);
sint8 build_ps_firmware(void);
sint8 build_conn_params(void);
sint8 create_ota_bin(char* aiofilename,char* otafilename);
sint8 build_burst_fw(tstrM2mRev *pstrm2mrev,char*);
sint8 init_custom_image_builder(char* custom_image_path);
sint8 make_custom_image(char* gain_file);

FILE* safeopen(const char* filename, const char * mode);

#endif	/* __BUILDER_H__ */

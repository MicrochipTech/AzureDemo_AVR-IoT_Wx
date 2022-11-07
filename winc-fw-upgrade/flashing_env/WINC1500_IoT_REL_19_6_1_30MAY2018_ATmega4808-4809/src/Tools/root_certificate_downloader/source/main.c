#include "windows.h"
#include "conio.h"
#include "process.h"

#include "root_setup.h"
#include "driver/include/m2m_svnrev.h"

#include "programmer.h"

/**
* CMD MACROS
*/
/**
* COMMAND LINE ARGUMENTS BITS IN RETURN BYTE
*/
#define NO_WAIT_BIT			(0x02)	/*!< Positiion of no_wait bit in the byte. */
#define ERASE_BEFORE_DL_BIT (0x04) /*!< Position of the erase before download in the byte. */
#define BREAK_BIT		    (0x08)	/*!< Positiion of break bit in the byte. */
/**
* SET VALUES DEPEND ON COMMAND LINE ARGUMENTS
*/
#define SET_BREAK_BIT(x)    (x |= BREAK_BIT)
#define SET_NO_WAITE_BIT(x) (x |= NO_WAIT_BIT)	/*!< set no_wait bit in the byte to 1. */
#define SET_ERASE_BEFORE_DL_BIT(x) (x |= ERASE_BEFORE_DL_BIT) /*!< set erase before download bit in the byte to 1. */
/**
* CHECK FOR VALUES FOR EACH CMD IN COMMAND LINE ARGUMENTS
*/
#define NO_WAIT(x)			(x & NO_WAIT_BIT)	/*!< It will return 1 if no_wait argument had been passed to main */
#define ERASE_BEFORE_DL(x)  (x & ERASE_BEFORE_DL_BIT) /*!< It will return 1 if erase before download has been passed to main */
#define BREAK(x)		(x & BREAK_BIT)


/**************************************************************/
int ReadFileToBuffer(char *pcFileName, uint8 **ppu8FileData, uint32 *pu32FileSize)
{
	FILE	*fp;
	int		ret = M2M_ERR_FAIL;

	fp = fopen(pcFileName, "rb");
	if(fp)
	{
		uint32	u32FileSize;
		uint8	*pu8Buf;

		fseek(fp, 0, SEEK_END);
		u32FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		pu8Buf = (uint8*)malloc(u32FileSize);
		if(pu8Buf != NULL)
		{
			fread(pu8Buf, 1, u32FileSize, fp);
			*pu32FileSize = u32FileSize;
			*ppu8FileData = pu8Buf;
			ret = M2M_SUCCESS;
		}
		fclose(fp);
	}
	return ret;
}

/**************************************************************/
static void print_nmi(void)
{
	printf("**************************************************\n\r");
	printf("* > WINC1500 Root Certificate Flash Downloader < *\n\r");
	printf("**************************************************\n\r");
	printf("SVN REV %u SVN BR %s \n\r",SVN_REVISION,SVN_REL_URL);
	printf("Built at %s\t%s\n", __DATE__, __TIME__);
}

/**************************************************************/
static uint8 checkArguments(char * argv[],uint8 argc, uint8* portNum, char ** vflash_path)
{
	sint8 ret = M2M_SUCCESS;
	uint8 loopCntr = 1;
	*portNum = 0;
	if(1 >= argc) goto ERR;
	for(;loopCntr<argc;loopCntr++)
	{
		if(strstr(argv[loopCntr],"-no_wait"))
		{
			SET_NO_WAITE_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-vflash_path"))
		{
			if(argc-loopCntr > 1) {
				*vflash_path =  argv[loopCntr+1];
				M2M_PRINT("Virtual Flash Path %s\n",*vflash_path);
			}
			continue;
		}
		if(strstr(argv[loopCntr],"-B"))
		{
			SET_BREAK_BIT(ret);
			continue;
		}
		if(strstr(argv[loopCntr],"-port"))
		{
			if(argc-loopCntr > 1)
				*portNum = atoi(argv[++loopCntr]);
			continue;
		}
		if(strstr(argv[loopCntr], "-e"))
		{
			SET_ERASE_BEFORE_DL_BIT(ret);
			continue;
		}
	}
ERR:
	return ret;
}

void dump_flash(char * filename)
{
#ifndef OTA_GEN
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
#endif // OTA_GEN
}


/**************************************************************/
int main(int argc, char * argv[])
{
	sint8	ret = 0;
	uint8	commands_val = 0;
	uint32	u32RootCertSz;
	uint8	*pu8RootCertBuff;
	char * vflash_path = NULL;
	uint8* vflash = NULL;

#ifdef PROFILING
	uint32	u32T1=0;
#endif

	if(argc > 3)
	{
		int		i;
		sint8	nCerts = 0;

		print_nmi();
		commands_val = checkArguments(argv,argc,&ret,&vflash_path);
		programmer_init(&ret,BREAK(commands_val));

        dump_flash("Before_rcd.bin");

        #ifndef OTA_GEN
        if(vflash_path != NULL)
        {
            uint32 sz = programmer_get_flash_size();
            vflash = malloc(sz);
            if(vflash)
            {
                FILE *fp;
				memset(vflash,0xFF,sz);
                fp = fopen(vflash_path,"rb");
                if(fp != NULL)
                {
	                M2M_PRINT("Reading vflash from %s\n", vflash_path);
                    fread(vflash,1,sz,fp);
                    fclose(fp);
                }
            }
        }
        #endif

#ifdef PROFILING
		u32T1=GetTickCount();
#endif

		if(argv[1][0] == '-')
		{
			if((argv[1][1] == 'n') || (argv[1][1] == 'N'))
			{
				nCerts = atoi(argv[2]);
				if((argc-(nCerts+3))< 0)
				{
					nCerts = 0;
					printf("(ERR)Invalid command line arg(s)\n");
				}
			}
		}

		if(ERASE_BEFORE_DL(commands_val))
		{
			/* Erase memory.
			*/
			ret = programmer_erase_root_cert(vflash);
			if(M2M_SUCCESS != ret) goto END;
			nm_bsp_sleep(50);
		}

		for(i = 3 ; nCerts > 0; nCerts --, i ++)
		{
			if(ReadFileToBuffer(argv[i], &pu8RootCertBuff, &u32RootCertSz) == M2M_SUCCESS)
			{
				if(WriteRootCertificate(pu8RootCertBuff, u32RootCertSz, vflash) != 0)
				{
					printf("Error writing certificate.\n");
					ret = -1;
					break;
				}
			}
		}

		if(nCerts == 0)
		{
			printf("All certificates have been downloaded\r\n");
		}
	}
	else
	{
		printf("(ERR)Insufficient command line args\n");
		ret = -1;
	}
END:

	dump_flash("After_rcd.bin");

    #ifndef OTA_GEN
	if(vflash)
    {
        M2M_PRINT("Saving vflash to %s\n", vflash_path);
        FILE *fp;
        uint8 * pf;
        uint32 sz = programmer_get_flash_size();
        fp = fopen(vflash_path,"wb");
        if(fp != NULL)
        {
            fwrite(vflash,1,sz,fp);
            fclose(fp);
        }
        free(pf);
    }
    #endif

	programmer_deinit();

#ifdef PROFILING
	M2M_PRINT("\n>>This task finished after %5.2f sec\n\r",(GetTickCount() - u32T1)/1000.0);
#endif

	if(!NO_WAIT(commands_val))
		system("pause");

	return ret;
}

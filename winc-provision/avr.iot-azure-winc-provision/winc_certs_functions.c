/* 
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include "winc_certs_functions.h"

uint8_t *signer_cert = NULL;
size_t signer_cert_size;
uint8_t *device_cert = NULL;
size_t device_cert_size;

uint8_t *file_list = NULL;
char *device_cert_filename = NULL;
char *signer_cert_filename = NULL;
uint32 sector_buffer[MAX_TLS_CERT_LENGTH];
uint32_t signer_ca_public_key_size = 0;

static const char* bin2hex(const void* data, size_t data_size)
{
    static char buf[256];
    static char hex[] = "0123456789abcdef";
    const uint8_t* data8 = data;
    
    if (data_size*2 > sizeof(buf)-1)
    return "[buf too small]";
    
    for (size_t i = 0; i < data_size; i++)
    {
        buf[i*2 + 0] = hex[(*data8) >> 4];
        buf[i*2 + 1] = hex[(*data8) & 0xF];
        data8++;
    }
    buf[data_size*2] = 0;
    
    return buf;
}


sint8 WINC_CERTS_appendFileBuf(uint32* buffer32, uint32 buffer_size, const char* file_name, uint32 file_size, const uint8* file_data)
{
    tstrTlsSrvSecHdr* header = (tstrTlsSrvSecHdr*)buffer32;
    tstrTlsSrvSecFileEntry* file_entry = NULL;
    uint16 str_size = m2m_strlen((uint8*)file_name) + 1;
    uint16 count = 0;
    uint8 *pBuffer = (uint8*)buffer32;

    while ((*pBuffer) == 0xFF)
    {
        
        if(count == INIT_CERT_BUFFER_LEN)
        break;
        count++;
        pBuffer++;
    }

    if(count == INIT_CERT_BUFFER_LEN)
    {
        // The WINC will need to add the reference start pattern to the header
        header->u32nEntries = 0; // No certs
        // The WINC will need to add the offset of the flash were the certificates are stored to this address
        header->u32NextWriteAddr = sizeof(*header); // Next cert will be written after the header
    }
    
    if (header->u32nEntries >= sizeof(header->astrEntries)/sizeof(header->astrEntries[0]))
    return M2M_ERR_FAIL; // Already at max number of files
    
    if ((header->u32NextWriteAddr + file_size) > buffer_size)
    return M2M_ERR_FAIL; // Not enough space in buffer for new file
    
    file_entry = &header->astrEntries[header->u32nEntries];
    header->u32nEntries++;
    
    if (str_size > sizeof(file_entry->acFileName))
    return M2M_ERR_FAIL; // File name too long
    m2m_memcpy((uint8*)file_entry->acFileName, (uint8*)file_name, str_size);
    
    file_entry->u32FileSize = file_size;
    file_entry->u32FileAddr = header->u32NextWriteAddr;
    header->u32NextWriteAddr += file_size;
    
    // Use memmove to accommodate optimizations where the file data is temporarily stored
    // in buffer32
    memmove(((uint8*)buffer32) + (file_entry->u32FileAddr), (uint8*)file_data, file_size);
    
    return M2M_SUCCESS;
}

size_t WINC_CERTS_getTotalFilesSize(const tstrTlsSrvSecHdr* header)
{
    uint8 *pBuffer = (uint8*) header;
    uint16 count = 0;

    while ((*pBuffer) == 0xFF)
    {
        
        if(count == INIT_CERT_BUFFER_LEN)
        break;
        count++;
        pBuffer++;
    }

    if(count == INIT_CERT_BUFFER_LEN)
    return sizeof(*header); // Buffer is empty, no files
    
    return header->u32NextWriteAddr;
}

void WINC_CERTS_initBuffer() {
    // Clear cert chain buffer
    memset(sector_buffer, 0xFF, MAX_TLS_CERT_LENGTH * sizeof(uint32));

    // Use the end of the sector buffer to temporarily hold the data to save RAM
    file_list   = ((uint8_t*)sector_buffer) + (MAX_TLS_CERT_LENGTH * sizeof(uint32)- TLS_FILE_NAME_MAX*2);
    signer_cert = file_list - SIGNER_CERT_MAX_LEN;
    device_cert = signer_cert - DEVICE_CERT_MAX_LEN;

    // Init the file list
    memset(file_list, 0, TLS_FILE_NAME_MAX*2);
    device_cert_filename = (char*)&file_list[0];
    signer_cert_filename = (char*)&file_list[TLS_FILE_NAME_MAX];
}

sint8 WINC_CERTS_transfer(uint8_t subject_key_id[20])
{
    sint8 status = M2M_SUCCESS;
    uint8_t signer_public_key[SIGNER_PUBLIC_KEY_MAX_LEN];
    
    uint8_t cert_sn[CERT_SN_MAX_LEN];
    size_t cert_sn_size;
    
    ATCA_STATUS atca_status = ATCACERT_E_SUCCESS;
    
    do
    {
        // Get the signer's public key from its certificate
        atca_status = atcacert_get_subj_public_key(&g_cert_def_1_signer, signer_cert,
        signer_cert_size, signer_public_key);
        if (atca_status != ATCACERT_E_SUCCESS)
        {
            // Break the do/while loop
            break;
        }

        if (subject_key_id)
        {
            atca_status = atcacert_get_subj_key_id(&g_cert_def_2_device, device_cert,
            device_cert_size, subject_key_id);
            if (atca_status != ATCACERT_E_SUCCESS)
            {
                // Break the do/while loop
                break;
            }
        }
        
        // Get the device certificate SN for the filename
        cert_sn_size = sizeof(cert_sn);
        atca_status = atcacert_get_cert_sn(&g_cert_def_2_device, device_cert,
        device_cert_size, cert_sn, &cert_sn_size);
        if (atca_status != ATCACERT_E_SUCCESS)
        {
            // Break the do/while loop
            break;
        }
        
        // Build the device certificate filename
        strcpy(device_cert_filename, "CERT_");
        strcat(device_cert_filename, bin2hex(cert_sn, cert_sn_size));
        
        // Add the DER device certificate the TLS certs buffer
        status = WINC_CERTS_appendFileBuf(sector_buffer, sizeof(sector_buffer),
        device_cert_filename, device_cert_size,
        device_cert);
        if (status != M2M_SUCCESS)
        {
            // Break the do/while loop
            break;
        }
        
        device_cert = NULL; // Make sure we don't use this now that it has moved
        
        // Get the signer certificate SN for the filename
        cert_sn_size = sizeof(cert_sn);
        atca_status = atcacert_get_cert_sn(&g_cert_def_1_signer, signer_cert,
        signer_cert_size, cert_sn, &cert_sn_size);
        if (atca_status != ATCACERT_E_SUCCESS)
        {
            // Break the do/while loop
            break;
        }
        
        
        // Build the signer certificate filename
        strcpy(signer_cert_filename, "CERT_");
        strcat(signer_cert_filename, bin2hex(cert_sn, cert_sn_size));
        
        // Add the DER signer certificate the TLS certs buffer
        status = WINC_CERTS_appendFileBuf(sector_buffer, sizeof(sector_buffer),
        signer_cert_filename, signer_cert_size, signer_cert);
        if (status != M2M_SUCCESS)
        {
            // Break the do/while loop
            break;
        }
        
        // Add the cert chain list file to the TLS certs buffer
        status = WINC_CERTS_appendFileBuf(sector_buffer, sizeof(sector_buffer),
        TLS_SRV_ECDSA_CHAIN_FILE,
        TLS_FILE_NAME_MAX*2, file_list);
        if (status != M2M_SUCCESS)
        {
            // Break the do/while loop
            break;
        }

        file_list = NULL;
        signer_cert_filename = NULL;
        device_cert_filename = NULL;
        
        // Update the TLS cert chain on the WINC.
        status = m2m_ssl_send_certs_to_winc((uint8 *)sector_buffer,
        (uint32)WINC_CERTS_getTotalFilesSize((tstrTlsSrvSecHdr*)sector_buffer));
        if (status != M2M_SUCCESS)
        {
            // Break the do/while loop
            break;
        }
    } while (false);

    if (atca_status)
    {
        M2M_ERR("eccSendCertsToWINC() failed with ret=%d", atca_status);
        status =  M2M_ERR_FAIL;
    }

    return status;
}
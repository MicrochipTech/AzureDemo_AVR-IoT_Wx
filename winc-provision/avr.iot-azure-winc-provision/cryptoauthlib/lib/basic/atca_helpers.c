/**
 * \file
 * \brief Helpers to support the CryptoAuthLib Basic API methods
 *
 * \copyright (c) 2017 Microchip Technology Inc. and its subsidiaries.
 *            You may use this software and any derivatives exclusively with
 *            Microchip products.
 *
 * \page License
 *
 * (c) 2017 Microchip Technology Inc. and its subsidiaries. You may use this
 * software and any derivatives exclusively with Microchip products.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIPS TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
 * TERMS.
 */

#include <stdlib.h>
#include <stdio.h>
#include "cryptoauthlib.h"
#include "atca_helpers.h"



/* Ruleset:
    Index   -   Meaning
    0       -   62 Character
    1       -   63 Character
    2       -   Pad Character (none if 0)
    3       -   Maximum line length (no limit if 0) */
uint8_t atcab_b64rules_default[4]   = { '+', '/', '=', 64 };
uint8_t atcab_b64rules_mime[4]      = { '+', '/', '=', 76 };
uint8_t atcab_b64rules_urlsafe[4]   = { '-', '_', 0, 0 };


#ifdef ATCAPRINTF
/** \brief Function that convert a binary buffer to a hex string suitable for human reading
 *  \param[in]  binary input buffer to convert
 *  \param[in]  bin_len length of buffer to convert
 *  \param[out] ascii_hex buffer that receives hex string
 *  \param[out] ascii_hex_len the length of the ascii_hex buffer
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_bin2hex(const uint8_t* binary, int bin_len, char* ascii_hex, int* ascii_hex_len)
{
    return atcab_bin2hex_(binary, bin_len, ascii_hex, ascii_hex_len, true);
}

/** \brief Function that converts a binary buffer to a hex string suitable for human reading
 *  \param[in]      inbuff input buffer to convert
 *  \param[in]      inbuffLen length of buffer to convert
 *  \param[out]     ascii_hex buffer that receives hex string
 *  \param[inout]     ascii_hex_len the length of the ascii_hex buffer
 *  \param[inout]     add_space indicates whether spaces and returns should be added for pretty printing
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_bin2hex_(const uint8_t* binary, int bin_len, char* ascii_hex, int* ascii_hex_len, bool add_space)
{
    int i;
    int hex_len = 0;

    // Verify the inputs
    if ((binary == NULL) || (ascii_hex == NULL) || (ascii_hex_len == NULL))
    {
        return ATCA_BAD_PARAM;
    }

    // Initialize the return bytes to all 0s
    memset(ascii_hex, 0, *ascii_hex_len);

    // Convert one byte at a time
    for (i = 0; i < bin_len; i++)
    {
        if (hex_len > *ascii_hex_len)
        {
            break;
        }
        if ((i % 16 == 0 && i != 0) && add_space)
        {
            sprintf(&ascii_hex[hex_len], "\r\n");
            hex_len += 2;
        }
        if (add_space)
        {
            sprintf(&ascii_hex[hex_len], "%02X ", *binary++);
            hex_len += 3;
        }
        else
        {
            sprintf(&ascii_hex[hex_len], "%02X", *binary++);
            hex_len += 2;
        }
    }
    *ascii_hex_len = (int)strlen(ascii_hex);

    return ATCA_SUCCESS;
}

/** \brief Function that converts a hex string to binary buffer
 *  \param[in]  ascii_hex    input buffer to convert
 *  \param[in]  ascii_hex_len length of buffer to convert
 *  \param[out] binary      buffer that receives binary
 *  \param[in]  bin_len      Hex length of binary buffer
 *  \return ATCA_SUCCESS on success, otherwise an error code.
 */
#ifdef ATCA_INTERFACE_V2
ATCA_STATUS atcab_hex2bin_v2(const char* ascii_hex, int ascii_hex_len, uint8_t* binary, int* bin_len, char* packed_hex)
#else
ATCA_STATUS atcab_hex2bin(const char* ascii_hex, int ascii_hex_len, uint8_t* binary, int* bin_len)
#endif
{
    int i = 0;
    int j = 0;
    uint32_t byt;

#ifndef ATCA_INTERFACE_V2
    char* packed_hex = NULL;
#endif
    int packed_len = ascii_hex_len;
    char hex_byte[3];

    // Verify the inputs
    if ((binary == NULL) || (ascii_hex == NULL) || (bin_len == NULL))
    {
        return ATCA_BAD_PARAM;
    }

    // Pack the bytes (remove white space & make even number of characters)
#ifndef ATCA_INTERFACE_V2
    packed_hex = (char*)malloc(packed_len);
    if (packed_hex == NULL)
    {
        return ATCA_ALLOC_FAILURE;
    }
    memset(packed_hex, 0, packed_len);
#endif
    packHex(ascii_hex, ascii_hex_len, packed_hex, &packed_len);

    // Initialize the binary buffer to all 0s
    memset(binary, 0, *bin_len);
    memset(hex_byte, 0, 3);

    // Convert the ascii bytes to binary
    for (i = 0, j = 0; i < packed_len; i += 2, j++)
    {
        if (i > packed_len || j > *bin_len)
        {
            break;
        }
        // Copy two characters to be scanned
        memcpy(hex_byte, &packed_hex[i], 2);
        sscanf(hex_byte, "%x", (unsigned int*)&byt);
        // take the msb of the uint32_t
        binary[j] = byt;
    }
    *bin_len = j;
#ifndef ATCA_INTERFACE_V2
    free(packed_hex);
#endif
    return ATCA_SUCCESS;
}

#endif

/**
 * \brief Checks to see if a character is an ASCII representation of a digit ((c ge '0') and (c le '9'))
 * \param[in] c  character to check
 * \return True if the character is a digit
 */
bool isDigit(char c)
{
    return (c >= '0') && (c <= '9');
}

/**
 * \brief Checks to see if a character is whitespace
 * \param[in] c  character to check
 * \return True if the character is whitespace
 */
bool isWhiteSpace(char c)
{
    return (c == '\n') || (c == '\r') || (c == '\t') || (c == ' ');
}

/**
 * \brief Checks to see if a character is an ASCII representation of hex ((c >= 'A') and (c <= 'F')) || ((c >= 'a') and (c <= 'f'))
 * \param[in] c  character to check
 * \return True if the character is a hex
 */
bool isAlpha(char c)
{
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

/**
 * \brief Checks to see if a character is an ASCII representation of hex ((c >= 'A') and (c <= 'F')) || ((c >= 'a') and (c <= 'f'))
 * \param[in] c  character to check
 * \return True if the character is a hex
 */
bool isHexAlpha(char c)
{
    return ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f'));
}

/**
 * \brief Returns true if this character is a valid hex character or if this is whitespace (The character can be
 *        included in a valid hexstring).
 * \param[in] c  character to check
 * \return True if the character can be included in a valid hexstring
 */
bool isHex(char c)
{
    return isHexDigit(c) || isWhiteSpace(c);
}

/**
 * \brief Returns true if this character is a valid hex character.
 * \param[in] c  character to check
 * \return True if the character can be included in a valid hexstring
 */
bool isHexDigit(char c)
{
    return isDigit(c) || isHexAlpha(c);
}

/**
 * \brief Remove white space from a ASCII hex string.
 * \param[in] ascii_hex		Initial hex string to remove white space from
 * \param[in] ascii_hex_len	Length of the initial hex string
 * \param[in] packed_hex		Resulting hex string without white space
 * \param[inout] packed_len	In: Size to packed_hex buffer
 *							Out: Number of bytes in the packed hex string
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS packHex(const char* ascii_hex, int ascii_hex_len, char* packed_hex, int* packed_len)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    int i = 0;
    int j = 0;

    do
    {
        // Verify the inputs
        if ((ascii_hex == NULL) || (packed_hex == NULL) || (packed_len == NULL))
        {
            status = ATCA_BAD_PARAM;
            BREAK(status, "Null input parameter");
        }
        // Loop through each character and only add the hex characters
        for (i = 0; i < ascii_hex_len; i++)
        {
            if (isHexDigit(ascii_hex[i]))
            {
                if (j > *packed_len)
                {
                    break;
                }
                packed_hex[j++] = ascii_hex[i];
            }
        }
        *packed_len = j;
    }
    while (false);
    // TODO: If there are not an even number of characters, then pad with a '0'

    return ATCA_SUCCESS;
}

/** \brief Print each hex character in the binary buffer with spaces between bytes
 *  \param[in] label label to print
 *  \param[in] binary input buffer to print
 *  \param[in] bin_len length of buffer to print
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_printbin_label(const char* label, uint8_t* binary, int bin_len)
{
    printf("%s", label);
    return atcab_printbin(binary, bin_len, true);
}

/** \brief Print each hex character in the binary buffer with spaces between bytes
 *  \param[in] binary input buffer to print
 *  \param[in] bin_len length of buffer to print
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_printbin_sp(uint8_t* binary, int bin_len)
{
    return atcab_printbin(binary, bin_len, true);
}

/** \brief Print each hex character in the binary buffer
 *  \param[in] binary input buffer to print
 *  \param[in] bin_len length of buffer to print
 *  \param[in] add_space indicates whether spaces and returns should be added for pretty printing
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_printbin(uint8_t* binary, int bin_len, bool add_space)
{
    int i = 0;
    int line_len = 16;

    // Verify the inputs
    if (binary == NULL)
    {
        return ATCA_BAD_PARAM;
    }

    // Set the line length
    line_len = add_space ? 16 : 32;

    // Print the bytes
    for (i = 0; i < bin_len; i++)
    {
        // Print the byte
        if (add_space)
        {
            printf("%02X ", binary[i]);
        }
        else
        {
            printf("%02X", binary[i]);
        }

        // Break at the line_len
        if ((i + 1) % line_len == 0)
        {
            printf("\r\n");
        }
    }
    // Print the last carriage return
    printf("\r\n");

    return ATCA_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Base 64 Encode/Decode

#define B64_IS_EQUAL   (uint8_t)64
#define B64_IS_INVALID (uint8_t)0xFF

/**
 * \brief Returns true if this character is a valid base 64 character or if this is whitespace (A character can be
 *        included in a valid base 64 string).
 * \param[in] c      character to check
 * \param[in] rules  base64 ruleset to use
 * \return True if the character can be included in a valid base 64 string
 */
bool isBase64(char c, const uint8_t * rules)
{
    return isBase64Digit(c, rules) || isWhiteSpace(c);
}

/**
 * \brief Returns true if this character is a valid base 64 character.
 * \param[in] c      character to check
 * \param[in] rules  base64 ruleset to use
 * \return True if the character can be included in a valid base 64 string
 */
bool isBase64Digit(char c, const uint8_t * rules)
{
    return isDigit(c) || isAlpha(c) || c == rules[0] || c == rules[1] || c == rules[2];
}

/**
 * \brief Returns the base 64 index of the given character.
 * \param[in] c      character to check
 * \param[in] rules  base64 ruleset to use
 * \return the base 64 index of the given character
 */
uint8_t base64Index(char c, const uint8_t * rules)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return (uint8_t)(c - 'A');
    }
    if ((c >= 'a') && (c <= 'z'))
    {
        return (uint8_t)(26 + c - 'a');
    }
    if ((c >= '0') && (c <= '9'))
    {
        return (uint8_t)(52 + c - '0');
    }
    if (c == rules[0])
    {
        return (uint8_t)62;
    }
    if (c == rules[1])
    {
        return (uint8_t)63;
    }
    if (c == rules[2])
    {
        return B64_IS_EQUAL;
    }
    return B64_IS_INVALID;
}

/**
 * \brief Returns the base 64 character of the given index.
 * \param[in] id     index to check
 * \param[in] rules  base64 ruleset to use
 * \return the base 64 character of the given index
 */
char base64Char(uint8_t id, const uint8_t * rules)
{
    if (id >= 0 && (id < 26))
    {
        return (char)('A' + id);
    }
    if ((id >= 26) && (id < 52))
    {
        return (char)('a' + id - 26);
    }
    if ((id >= 52) && (id < 62))
    {
        return (char)('0' + id - 52);
    }
    if (id == 62)
    {
        return rules[0];
    }
    if (id == 63)
    {
        return rules[1];
    }

    if (id == B64_IS_EQUAL)
    {
        return rules[2];
    }
    return B64_IS_INVALID;
}

static ATCA_STATUS atcab_base64decode_block(const uint8_t id[4], uint8_t* data, size_t* data_size, size_t data_max_size)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    size_t new_bytes = 0;

    do
    {
        // Make sure padding characters can only be the last two
        if ((id[0] == B64_IS_EQUAL) ||
            (id[1] == B64_IS_EQUAL) ||
            (id[2] == B64_IS_EQUAL && id[3] != B64_IS_EQUAL))
        {
            status = ATCA_BAD_PARAM;
            BREAK(status, "Base64 chars after end padding");
        }

        // Make sure output buffer has enough space
        if (id[2] == B64_IS_EQUAL)
        {
            new_bytes = 1;
        }
        else if (id[3] == B64_IS_EQUAL)
        {
            new_bytes = 2;
        }
        else
        {
            new_bytes = 3;
        }
        if ((*data_size) + new_bytes > data_max_size)
        {
            status = ATCA_BAD_PARAM;
            BREAK(status, "decoded buffer too small");
        }

        // Decode into output buffer
        data[(*data_size)++] = ((id[0] << 2) | (id[1] >> 4));
        if (id[2] == B64_IS_EQUAL)
        {
            break;
        }
        data[(*data_size)++] = ((id[1] << 4) | (id[2] >> 2));
        if (id[3] == B64_IS_EQUAL)
        {
            break;
        }
        data[(*data_size)++] = ((id[2] << 6) | id[3]);
    }
    while (false);

    return status;
}

/**
 * \brief Decode base64 string to data with ruleset option.
 *
 * \param[in]    encoded       Base64 string to be decoded.
 * \param[in]    encoded_size  Size of the base64 string in bytes.
 * \param[out]   data          Decoded data will be returned here.
 * \param[inout] data_size     As input, the size of the byte_array buffer.
 *                             As output, the length of the decoded data.
 * \param[in]    rules         base64 ruleset to use
 */
ATCA_STATUS atcab_base64decode_(const char* encoded, size_t encoded_size, uint8_t* data, size_t* data_size, const uint8_t * rules)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    uint8_t id[4];
    int id_index = 0;
    size_t enc_index = 0;
    size_t data_max_size;
    bool is_done = false;

    do
    {
        // Check the input parameters
        if (encoded == NULL || data == NULL || data_size == NULL || rules == NULL)
        {
            status = ATCA_BAD_PARAM;
            BREAK(status, "Null input parameter");
        }
        data_max_size = *data_size;
        *data_size = 0;

        // Start decoding the input data
        for (enc_index = 0; enc_index < encoded_size; enc_index++)
        {
            if (isWhiteSpace(encoded[enc_index]))
            {
                continue; // Skip any whitespace characters
            }
            if (!isBase64Digit(encoded[enc_index], rules))
            {
                status = ATCA_BAD_PARAM;
                BREAK(status, "Invalid base64 character");
            }
            if (is_done)
            {
                // We found valid base64 characters after end padding (equals)
                // characters
                status = ATCA_BAD_PARAM;
                BREAK(status, "Base64 chars after end padding");
            }
            id[id_index++] = base64Index(encoded[enc_index], rules);
            // Process data 4 characters at a time
            if (id_index >= 4)
            {
                id_index = 0;
                status = atcab_base64decode_block(id, data, data_size, data_max_size);
                if (status != ATCA_SUCCESS)
                {
                    break;
                }

                is_done = (id[3] == B64_IS_EQUAL);
            }
        }
        if (status != ATCA_SUCCESS)
        {
            break;
        }
        if (id_index)
        {
            if (id_index < 2)
            {
                status = ATCA_BAD_PARAM;
                BREAK(status, "Invalid number of base64 chars");
            }
            // End of base64 string, but no padding characters
            for (; id_index < 4; id_index++)
            {
                id[id_index] = B64_IS_EQUAL;
            }
            status = atcab_base64decode_block(id, data, data_size, data_max_size);
        }
    }
    while (false);

    return status;
}

/** \brief Encode data as base64 string with ruleset option. */
ATCA_STATUS atcab_base64encode_(
    const uint8_t*  data,         /**< [in] The input byte array that will be converted to base 64 encoded characters */
    size_t          data_size,    /**< [in] The length of the byte array */
    char*           encoded,      /**< [in] The output converted to base 64 encoded characters. */
    size_t*         encoded_size, /**< [inout] Input: The size of the encoded buffer, Output: The length of the encoded base 64 character string */
    const uint8_t * rules         /**< [in] ruleset to use during encoding */
    )
{
    ATCA_STATUS status = ATCA_SUCCESS;
    size_t data_idx = 0;
    size_t b64_idx = 0;
    size_t offset = 0;
    uint8_t id = 0;
    size_t b64_len;

    do
    {
        // Check the input parameters
        if (encoded == NULL || data == NULL || encoded_size == NULL || !rules)
        {
            status = ATCA_BAD_PARAM;
            BREAK(status, "Null input parameter");
        }

        // Calculate output length for buffer size check
        b64_len = (data_size / 3 + (data_size % 3 != 0)) * 4; // ceil(size/3)*4
        if (rules[3])
        {
            // We add newlines to the output
            if (rules[3] % 4 != 0)
            {
                status = ATCA_BAD_PARAM;
                BREAK(status, "newline rules[3] must be multiple of 4");
            }
            b64_len += (b64_len / rules[3]) * 2;
        }
        b64_len += 1; // terminating null
        if (*encoded_size < b64_len)
        {
            status = ATCA_SMALL_BUFFER;
            BREAK(status, "Length of encoded buffer too small");
        }
        // Initialize the return length to 0
        *encoded_size = 0;

        // Loop through the byte array by 3 then map to 4 base 64 encoded characters
        for (data_idx = 0; data_idx < data_size; data_idx += 3)
        {
            // Add \r\n every n bytes if specified
            if (rules[3] && data_idx > 0 && (b64_idx - offset) % rules[3] == 0)
            {
                // as soon as we do this, we introduce an offset
                encoded[b64_idx++] = '\r';
                encoded[b64_idx++] = '\n';
                offset += 2;
            }

            id = (data[data_idx] & 0xFC) >> 2;
            encoded[b64_idx++] = base64Char(id, rules);
            id = (data[data_idx] & 0x03) << 4;
            if (data_idx + 1 < data_size)
            {
                id |= (data[data_idx + 1] & 0xF0) >> 4;
                encoded[b64_idx++] = base64Char(id, rules);
                id = (data[data_idx + 1] & 0x0F) << 2;
                if (data_idx + 2 < data_size)
                {
                    id |= (data[data_idx + 2] & 0xC0) >> 6;
                    encoded[b64_idx++] = base64Char(id, rules);
                    id = data[data_idx + 2] & 0x3F;
                    encoded[b64_idx++] = base64Char(id, rules);
                }
                else
                {
                    encoded[b64_idx++] = base64Char(id, rules);
                    encoded[b64_idx++] = base64Char(B64_IS_EQUAL, rules);
                }
            }
            else
            {
                encoded[b64_idx++] = base64Char(id, rules);
                encoded[b64_idx++] = base64Char(B64_IS_EQUAL, rules);
                encoded[b64_idx++] = base64Char(B64_IS_EQUAL, rules);
            }
        }

        // Strip any trailing nulls
        while (b64_idx > 1 && encoded[b64_idx - 1] == 0)
        {
            b64_idx--;
        }

        // Null terminate end
        encoded[b64_idx++] = 0;

        // Set the final encoded length (excluding terminating null)
        *encoded_size = b64_idx - 1;
    }
    while (false);
    return status;
}


/**
 * \brief Encode data as base64 string
 *
 * \param[in]    byte_array   Data to be encode in base64.
 * \param[in]    array_len    Size of byte_array in bytes.
 * \param[in]    encoded      Base64 output is returned here.
 * \param[inout] encoded_len  As input, the size of the encoded buffer.
 *                            As output, the length of the encoded base64
 *                            character string.
 *
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_base64encode(const uint8_t* byte_array, size_t array_len, char* encoded, size_t* encoded_len)
{
    return atcab_base64encode_(byte_array, array_len, encoded, encoded_len, atcab_b64rules_default);
}

/**
 * \brief Decode base64 string to data
 *
 * \param[in]    encoded     Base64 string to be decoded.
 * \param[in]    encoded_len  Size of the base64 string in bytes.
 * \param[out]   byte_array   Decoded data will be returned here.
 * \param[inout] array_len    As input, the size of the byte_array buffer.
 *                            As output, the length of the decoded data.
 *
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS atcab_base64decode(const char* encoded, size_t encoded_len, uint8_t* byte_array, size_t* array_len)
{
    return atcab_base64decode_(encoded, encoded_len, byte_array, array_len, atcab_b64rules_default);
}




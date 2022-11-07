/**
****************************************************************************************
*
* @file lm_spcomputation.c
*
* @brief Link Manager Simple Pairing functions
*
* Copyright (C) RivieraWaves 2009-2013
*
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup LMSPCOMPUTATION
* @{
****************************************************************************************
*/

#include <string.h>               // string definitions
//#include "hci.h"                  // host controller interface definition
#include "lm_spcomputation.h"     // simple pairing computations

/// SP SHA256
static const uint32_t lm_sp_sha256k[]=
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2

};

/// SP SHA Hash
static const uint32_t lm_sp_sha_256hash_init[]=
{
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};

/// SP P192 Field
const lm_sp_num_192 lm_sp_p192_field =
{
    0x00000000,
    { 0xffffffff,0xffffffff,0xfffffffe,0xffffffff,0xffffffff,0xffffffff }
};

/// SP P192 Data
const lm_sp_p192_group_data lm_sp_p192_data =
{
    {
        { 0x00000000,
          {0x82FF1012,0xF4FF0AFD,0x43A18800,0x7CBF20EB,0xB03090F6,0x188DA80E} },
        { 0x00000000,
          {0x1e794811,0x73f977a1,0x6b24cdd5,0x631011ed,0xffc8da78,0x07192b95} },
        { 0x00000000,
          {0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000} }
    },
    {
        0x00000000,    
        {0xFFFFFFFC,0xFFFFFFFF,0xFFFFFFFE,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF}
    }
};

/// SP P256 Field
const lm_sp_num_256 lm_sp_p256_field =
{
    0x00000000,
    { 0xffffffff,0xffffffff,0xffffffff,0x00000000,0x00000000,0x00000000,0x00000001,0xffffffff }
};

/// SP P256 Data
              

const lm_sp_p256_group_data lm_sp_p256_data =
{
    {
        { 0x00000000,
          {0xd898c296,0xf4a13945,0x2deb33a0,0x77037d81,0x63a440f2,0xf8bce6e5,0xe12c4247,0x6b17d1f2} },
        { 0x00000000,
          {0x37bf51f5,0xcbb64068,0x6b315ece,0x2bce3357,0x7c0f9e16,0x8ee7eb4a,0xfe1a7f9b,0x4fe342e2} },
        { 0x00000000,
          {0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000} }
    },
    {
        0x00000000,    
        { 0xfffffffc,0xffffffff,0xffffffff,0x00000000,0x00000000,0x00000000,0x00000001,0xffffffff }
    }
};

/*
 * FUNCTION DEFINITION
 * ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x       coordinate
 * @param[in] y       coordinate
 * @param[in] z       coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_ch(uint32_t x, uint32_t y, uint32_t z)
{
    return ((x&y)^((~x)&z));
}

/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x       coordinate
 * @param[in] y       coordinate
 * @param[in] z       coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_maj(uint32_t x, uint32_t y, uint32_t z)
{
    return ((x&y)^(x&z)^(y&z));
}


/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x     coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_sigma0(uint32_t x)
{
    uint32_t result=0;
    result =  ((x>> 7)|(x<<(32- 7)));
    result ^= ((x>>18)|(x<<(32-18)));
    result ^= (x>> 3);
    return result;
}

/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x    coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_sigma1(uint32_t x)
{
    uint32_t result=0;
    result =  ((x>>17)|(x<<(32-17)));
    result ^= ((x>>19)|(x<<(32-19)));
    result ^= (x>>10);
    return result;
}

/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x     coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_sigma0_1(uint32_t x)
{
    uint32_t result=0;
    result =  ((x>> 2)|(x<<(32- 2)));
    result ^= ((x>>13)|(x<<(32-13)));
    result ^= ((x>>22)|(x<<(32-22)));
    return result;
}

/*
 ****************************************************************************************
 * @brief This function is used for SHA-256 calculation
 *
 * @param[in] x     coordinate
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_sigma1_1(uint32_t x)
{
    uint32_t result=0;
    result =  ((x>> 6)|(x<<(32-6)));
    result ^= ((x>>11)|(x<<(32-11)));
    result ^= ((x>>25)|(x<<(32-25)));
    return result;
}


/*
 ****************************************************************************************
 * @brief This function is used to pad the message block for the SHA-256
 *
 * @param[in] Message_in The input message block
 * @param[in] Length Length of the input message block
 *
 * @return returns the number of blocks of 64 bytes after the padding.
 *
 ****************************************************************************************
 */
static uint8_t lm_sha_256_pad_msg(uint8_t* Message_in,uint16_t Length)
{

    memset((Message_in+Length),0,(192-Length));
    Message_in[Length]=0x80;
    if(Length > 119)
    {
        Message_in[190] = ((uint16_t)Length*8)>>8;
        Message_in[191] = ((uint16_t)Length*8)& 0xff;
        return 3;
    }
    else if(Length > 55 )
    {
        Message_in[126] =  ((uint16_t)Length*8)>>8;
        Message_in[127] =  ((uint16_t)Length*8)& 0xff;
        return 2;
    }
    else
    {
        Message_in[63]=Length*8;
        return 1;
    }
}


/*
 ****************************************************************************************
 * @brief This function calculates the SHA hash for the given input block
 *
 * @param[in] Hash          The pointer to the hash value calculated
 * @param[in] Message       The pointer to the message block for which hash is
 *                          to be calculated
 * @param[in] Num_Chunks    The number of chunks
 *
 ****************************************************************************************
 */
static void lm_sha_256_calc_hash(uint32_t* Hash, uint8_t* Message,uint8_t Num_Chunks)
{
    uint32_t a,b,c,d,e,f,g,h;
    uint32_t W[64];
    uint8_t  t,ctr;
    uint8_t  Chunk_Num=0;
    uint8_t* Message_Ptr;

    for(Chunk_Num=0;Chunk_Num<Num_Chunks;Chunk_Num++)
    {
        Message_Ptr = &(Message[Chunk_Num*64]);

        /* Initialize the W for t<16                                                    */
        for(ctr=t=0;t<16;ctr+=4,t++)
        {
            W[t] = ( (Message_Ptr[ctr+0]<<24) | 
                     (Message_Ptr[ctr+1]<<16) | 
                     (Message_Ptr[ctr+2]<<8)  | 
                     Message_Ptr[ctr+3]
                   );
        }
        
        for(t=16;t<64;t++)
        {
            W[t] = lm_sigma1(W[t-2]) +  W[t-7] +  lm_sigma0(W[t-15])+ W[t-16];
        }

        a = Hash[0];
        b = Hash[1];
        c = Hash[2];
        d = Hash[3];
        e = Hash[4];
        f = Hash[5];
        g = Hash[6];
        h = Hash[7];

        for(t=0;t<64;t++)
        {
            uint32_t T1,T2;
            T1 = h + lm_sigma1_1(e) + lm_ch(e,f,g) + lm_sp_sha256k[t] + W[t];
            T2 = lm_sigma0_1(a) + lm_maj(a,b,c);
            h=g;
            g=f;
            f=e;
            e=d+T1;
            d=c;
            c=b;
            b=a;
            a=T1+T2;
        }
        Hash[0] += a;
        Hash[1] += b;
        Hash[2] += c;
        Hash[3] += d;
        Hash[4] += e;
        Hash[5] += f;
        Hash[6] += g;
        Hash[7] += h;
    }
}

/*
 ****************************************************************************************
 * @brief This function returns the position of the highest bit set in a uint32_t
 *
 * @param[in] x the input word
 *
 * @return uint32_t
 *
 ****************************************************************************************
 */
static uint32_t lm_word_highest_bit_idx(uint32_t x)
{
    uint32_t r = 0;
    if ( 0==x ) 
        return 0;
    if ( x & 0xffff0000 ) 
    { 
        x >>= 16; 
        r += 16; 
    }
    if ( x & 0x0000ff00 ) 
    { 
        x >>= 8; r += 8; 
    }
    if ( x & 0x000000f0 ) 
    { 
        x >>= 4; r += 4; 
    }
    if ( x & 0x0000000c ) 
    { 
        x >>= 2; r += 2; 
    }
    if ( x & 0x00000002 ) 
    { 
        r += 1; 
    }
    return (r+1);
}

/*
 ****************************************************************************************
 * @brief This function adds the number of words to be added
 *
 * @param[in] r  the result of the word addition
 * @param[in] a  One of the operand for addition
 * @param[in] b  Other operand for addition
 * @param[in] n  number of uint32_t words to be added
 *
 * @return the carry after addition.
 *
 ****************************************************************************************
 */

static uint32_t lm_word_add(uint32_t *r,uint32_t *a,uint32_t *b, int n)
{
    uint32_t c,l,t;
    int i;
    if (n <= 0) 
    {
        return((uint32_t)0);
    }

    c=0;
    for (i=0;i<n;i++)
    {
        t=a[i];
        t=(t+c);
        c=(t < c);
        l=(t+b[i]);
        c+=(l < t);
        r[i]=l;
    }
    return((uint32_t)c);
}

/*
 ****************************************************************************************
 * @brief This function subtracts the array pf two words
 *
 * @param[in] r  the result of the word addition
 * @param[in] a  One of the operand for addition
 * @param[in] b  Other operand for addition
 * @param[in] n  number of uint32_t words to be added
 *
 * @return uint32_t the borrow
 *
 ****************************************************************************************
 */
static uint32_t lm_word_sub(uint32_t *r, uint32_t *a, uint32_t *b, int n)
{
    uint32_t t1,t2;
    int c=0;
    int i;

    if (n == 0)
    {
        return((uint32_t)0);
    }

    for (i=0;i<n;i++)
    {
        t1=a[i]; 
        t2=b[i];
        r[i]=(t1-t2-c);
        if (t1 != t2) 
        {
            c=(t1 < t2);
        }
    }
    return(c);
}

/*
 ****************************************************************************************
 * @brief This function multiplies the array of uint32_t with a int
 *
 * @param[in]  rp      The result of the multiplication
 * @param[in]  ap      The array of number to be multiplied
 * @param[in]  num     The number of words to be multiplied
 * @param[in]  w       The number to be multiplied
 *
 * @return uint32_t 0 on success , non zero on failure
 *
 ****************************************************************************************
 */

static uint32_t lm_word_mul(uint32_t *rp,uint32_t *ap, uint16_t num, uint32_t w)
{
    uint32_t carry=0;
    uint32_t bl,bh;
    uint16_t i;

    if (num <= 0) 
    {
        return((uint32_t)0);
    }

    bl=LM_SPLBITS(w);
    bh=LM_SPHBITS(w);

    for (i=0;i<num;i++)
    {
        uint32_t m,m1,l,h;
        h = ap[i];
        l = LM_SPLBITS(h);
        h = LM_SPHBITS(h);
        m = bh*l;
        l *= bl;
        m1 = bl*h;
        h *= bh;
        m=(m+m1); 
        if (m < m1)
        {
            h+=LM_SPL2HBITS((uint32_t)1);
        }
        h += LM_SPHBITS(m);
        m1 = LM_SPL2HBITS(m);
        l+=m1;
        if (l < m1)
        {
            h++;
        }
        l+=carry;
        if(l< carry)
        {
            h++;
        }
        carry=h;
        rp[i]=l;
        
    }
    return(carry);
}

/*
 ****************************************************************************************
 * @brief This function does the multiplication and the addition of the words
 *
 * @param[in] rp     The result of the multiplication
 * @param[in] ap     The array of number to be multiplied
 * @param[in] num    The number of words to be multiplied
 * @param[in] w      The number to be multiplied
 *
 * @return uint32_t 0 on success , non zero on failure
 *
 ****************************************************************************************
 */
static uint32_t lm_word_mul_add(uint32_t *rp, const uint32_t *ap, uint16_t num, uint32_t w)
{
    uint32_t i,c=0;
    uint32_t bl,bh;
    
    if (num == 0) 
    {
        return((uint32_t)0);
    }

    bl=LM_SPLBITS(w);
    bh=LM_SPHBITS(w);

    for (i=0;i<num;i++)
    {
        uint32_t l,h;
        uint32_t m,m1;
        h = ap[i];
        l = LM_SPLBITS(h);
        h = LM_SPHBITS(h);
        m = bh*l;
        l *= bl;
        m1 = bl*h;
        h *= bh;
        m=(m+m1); 
        if (m < m1)
        {
            h += LM_SPL2HBITS((uint32_t)1);
        }
        h += LM_SPHBITS(m);
        m1 = LM_SPL2HBITS(m);
        l+=m1;
        if (l < m1)
        {
            h++;
        }
        l=(l+ c); 
        if (l < c) 
        {
            h++;
        }
        c=rp[i];
        l=(l+c); 
        if (l < c)
        {
            h++;
        }
        c=h;
        rp[i]=l;
    }
    return(c);
} 

/*
 ****************************************************************************************
 * @brief This function is used to square the number
 *
 * @param[in] r    The square of the array
 * @param[in] a    The input number to be squared
 * @param[in] n    The number of elements in the array a
 *
 * @return uint32_t 0 on success , non zero on failure
 *
 ****************************************************************************************
 */
static void lm_word_sqr(uint32_t *r, const uint32_t *a, int n)
{
    int i;
    if (n == 0) 
    {
        return;
    }
    for (i=0;i<n;i++)
    {
        uint32_t l,h,m;
        h=a[i];
        l = LM_SPLBITS(h);
        h = LM_SPHBITS(h);
        m =(l)*(h);
        l*=l;
        h*=h;
        h+=(m & LM_SP_WORD_MASK2h1)>>(LM_SP_WORD_BITS4-1);
        m =(m & LM_SP_WORD_MASK2l)<<(LM_SP_WORD_BITS4+1);
        l=(l+m); 
        if (l < m)
        {
            h++;
        }
        r[2*i]=l;
        r[2*i+1]=h;
    }
}

/*
 ****************************************************************************************
 * @brief This function is used to get the number of words in the N-192 number
 *
 * @param[in] a     The input
 * @param[in] Type  Type of the input
 *
 * @return the number of words in a number
 *
 ****************************************************************************************
 */
static uint16_t lm_n_get_words(void* a,uint32_t type)
{
    int16_t i=0;
    
	int16_t NUM_BYTES=0;

	if(type == LM_SP_TYPE_NUM_192)
		NUM_BYTES = LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_384)
		NUM_BYTES = 2*LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_256)
		NUM_BYTES = LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_512)
		NUM_BYTES = 2*LM_SP_NUM_256_BYTES;


    for(i=(NUM_BYTES-1);i>=0;i--)
    {
        if((((lm_sp_num_512*)a)->Number[i]))
        {
            break;
        }
    }
    return (i+1);
}


int lm_n_is_zero(void* a,uint32_t type)
{
    uint16_t i;
	uint32_t NUM_BYTES;

	if(type == LM_SP_TYPE_NUM_512)
		NUM_BYTES = 2*LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_256)
		NUM_BYTES = LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_384)
		NUM_BYTES = 2*LM_SP_NUM_192_BYTES;
	else //	if(type == LM_SP_TYPE_NUM_192)
		NUM_BYTES = LM_SP_NUM_192_BYTES;
	 

    for(i=0;i < NUM_BYTES;i++)
    {
        if(((lm_sp_num_512*)a)->Number[i])
            return 0;
    }
    return 1;
}

/*
 ****************************************************************************************
 * @brief This function returns the number of bits in a given number
 *
 * @param[in]  a     The input
 * @param[in]  Type  Type of the input
 *
 * @return number of bits in a given number
 *
 ****************************************************************************************
 */
static int lm_n_num_bits(void *a,uint32_t type)
{
    int i = 0;
    uint32_t NUM_BYTES;
    uint32_t result = 0;

	if(type == LM_SP_TYPE_NUM_192)
		NUM_BYTES = LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_384)
		NUM_BYTES = 2*LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_256)
		NUM_BYTES = LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_512)
		NUM_BYTES = 2*LM_SP_NUM_256_BYTES;


    if (lm_n_is_zero(a,NUM_BYTES))
    {
        return 0;
    }
    for(i=(NUM_BYTES-1);i>=0;i--)
    {
    	result = ((lm_sp_num_512*)a)->Number[i];
        if(result)
        {
            break;
        }
    }
    return ((i * LM_SP_NUM_192_BITS) + lm_word_highest_bit_idx(result));
}

/*
 ****************************************************************************************
 * @brief This function os used to make a given number zero
 *
 * @param[in] a     The input
 * @param[in] Type  Type of the input
 ****************************************************************************************
 */

static void lm_n_zero(void* a,uint32_t type)
{
    if(type==4)
    {
        memset(a,0,sizeof(lm_sp_num_512));
    }
    else if(type==3)
    {
        memset(a,0,sizeof(lm_sp_num_256));
    }
	else if(type==2)
    {
        memset(a,0,sizeof(lm_sp_num_384));
    }
	else// if(type==1)
    {
        memset(a,0,sizeof(lm_sp_num_192));
    }
}

/*
 ****************************************************************************************
 * @brief This function is used to check if the input number is one
 *
 * @param[in]  a     The input
 * @param[in]  Type  Type of the input
 *
 * @return nonzero if number is one else zero.
 *
 ****************************************************************************************
 */
static int lm_n_is_one(void *a,uint32_t type)
{
    uint16_t i;
	uint32_t NUM_BYTES;

	if(type == LM_SP_TYPE_NUM_192)
		NUM_BYTES = LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_384)
		NUM_BYTES = 2*LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_256)
		NUM_BYTES = LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_512)
		NUM_BYTES = 2*LM_SP_NUM_256_BYTES;


    for(i=1;i < NUM_BYTES;i++)
    {
        if(((lm_sp_num_512*)a)->Number[i])
            return 0;
    }
    if(((lm_sp_num_512*)a)->Number[0]!=1)
    {
        return 0;
    }
    return 1;    
}

/*
 ****************************************************************************************
 * @brief This function copies the number to other number.
 *
 * @param[in] dest  The destination of copy
 * @param[in] src   The source of copy
 * @param[in] Type  Type of the input
 *
 * @return non zero on success and vice-versa
 *
 ****************************************************************************************
 */
int lm_n_copy(void *dest, void *src,uint32_t type)
{
    if(dest==src)
    {
        return 1;
    }
    else
    {
		if(type==LM_SP_TYPE_NUM_512)
        {
            memset(dest,0,sizeof(lm_sp_num_512));
            memcpy(dest,src,sizeof(lm_sp_num_512));
        }
        else if(type==LM_SP_TYPE_NUM_256)
        {
            memset(dest,0,sizeof(lm_sp_num_256));
            memcpy(dest,src,sizeof(lm_sp_num_256));
        }
        else if(type==LM_SP_TYPE_NUM_384)
        {
            memset(dest,0,sizeof(lm_sp_num_384));
            memcpy(dest,src,sizeof(lm_sp_num_384));
        }
        else
        {
            memset(dest,0,sizeof(lm_sp_num_192));
            memcpy(dest,src,sizeof(lm_sp_num_192));
        }
    }
    return 1;
}

/*
 ****************************************************************************************
 * @brief This function checks if the given bit in the number is set or clear
 *
 * @param[in] a       The input
 * @param[in] bit     The bit number to be checked for value
 * @param[in] Type    The type of the number
 *
 * @return non-zero if bit is set and vice-versa
 *
 ****************************************************************************************
 */
static int lm_n_is_set(void* a,uint16_t bit,uint32_t type)
{
	uint32_t NUM_BYTES;

    uint16_t word;
    lm_sp_num_512* tmp;
    tmp = (lm_sp_num_512*)a;
    word = bit/LM_SP_NUM_192_BITS;
    bit = bit %LM_SP_NUM_192_BITS;

	if(type == LM_SP_TYPE_NUM_192)
		NUM_BYTES = LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_384)
		NUM_BYTES = 2*LM_SP_NUM_192_BYTES;
	else if(type == LM_SP_TYPE_NUM_256)
		NUM_BYTES = LM_SP_NUM_256_BYTES;
	else if(type == LM_SP_TYPE_NUM_512)
		NUM_BYTES = 2*LM_SP_NUM_256_BYTES;


    if(word >= NUM_BYTES)
    {
        return 0;
    }
    else
    {
        return (tmp->Number[word] & ((uint32_t)1)<<bit)?1:0;
    }
}

/*
 ****************************************************************************************
 * @brief This functions does the unsigned comparison of two NUM192 numbers
 *
 * @param[in] LM_SP_NUM_192* a: One of the operand for comparison
 * @param[in] LM_SP_NUM_192* b: other operand for comparison
 *
 * @return non zero if not equal else zero
 *
 ****************************************************************************************
 */
int lm_n192_ucmp(lm_sp_num_192*a,lm_sp_num_192  *b)
{
    int i;
    uint32_t t1,t2;

    for (i=(LM_SP_NUM_192_BYTES-1); i>=0; i--)
    {
        t1= a->Number[i];
        t2= b->Number[i];
        if (t1 != t2)
            return((t1 > t2) ? 1 : -1);
    }
    return(0);
}

int lm_n256_ucmp(lm_sp_num_256*a,lm_sp_num_256  *b)
{
    int i;
    uint32_t t1,t2;

    for (i=(LM_SP_NUM_256_BYTES-1); i>=0; i--)
    {
        t1= a->Number[i];
        t2= b->Number[i];
        if (t1 != t2)
            return((t1 > t2) ? 1 : -1);
    }
    return(0);
}

/*
 ****************************************************************************************
 * @brief This functions does the unsigned comparison of NUM192 and NUM384
 *        numbers
 *
 * @param[in] lm_sp_num_192* a: One of the operand for comparison
 * @param[in] lm_sp_num_384* b: other operand for comparison
 *
 * @return non zero if not equal else zero
 *
 ****************************************************************************************
 */
static int lm_n192_384_ucmp(lm_sp_num_384 *a,lm_sp_num_192* b )
{
    int la = lm_n_num_bits((void*)a,LM_SP_TYPE_NUM_384);
    if(la>192)
    {
        return 1;
    }
    return lm_n192_ucmp((lm_sp_num_192*)a,b);
}

static int lm_n256_512_ucmp(lm_sp_num_512 *a,lm_sp_num_256* b )
{
    int la = lm_n_num_bits((void*)a,LM_SP_TYPE_NUM_512);
    if(la>256)
    {
        return 1;
    }
    return lm_n256_ucmp((lm_sp_num_256*)a,b);
}

/*
 ****************************************************************************************
 * @brief This function does the comparison of the signed numbers
 *
 * @param[in] a One of the operand for comparison
 * @param[in] b Other operand for comparison
 *
 * @return non zero if not equal else zero
 *
 ****************************************************************************************
 */
static int lm_n192_384_cmp(lm_sp_num_384 *a,lm_sp_num_192* b )
{
    int i;
    if (a->Neg != b->Neg)
    {
        if (a->Neg)
        {
            return(-1);
        }
        else
        {
            return(1);
        }
    }

    i = lm_n192_384_ucmp(a,b);
    if (a->Neg)
    { 
        return -i;
    }
    else    
    { 
        return i;
    }
}

static int lm_n256_512_cmp(lm_sp_num_512 *a,lm_sp_num_256* b )
{
    int i;
    if (a->Neg != b->Neg)
    {
        if (a->Neg)
        {
            return(-1);
        }
        else
        {
            return(1);
        }
    }

    i = lm_n256_512_ucmp(a,b);
    if (a->Neg)
    { 
        return -i;
    }
    else    
    { 
        return i;
    }
}

/*
 ****************************************************************************************
 * @brief This function subtracts two unsigned numbers
 *
 * @param[in] r      The result of subtraction
 * @param[in] a      The operand for the subtraction
 * @param[in] b      The operand for the subtraction
 *
 * @return Borrow
 *
 ****************************************************************************************
 */
int lm_n192_usub(lm_sp_num_192 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    int c=0;
    lm_n_zero(r,LM_SP_TYPE_NUM_192);
    c = lm_word_sub(r->Number,a->Number,b->Number,LM_SP_NUM_192_BYTES);
    if(c)
    {
        return 0;
    }
    return 1;
}

int lm_n256_usub(lm_sp_num_256 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    int c=0;
    lm_n_zero(r,LM_SP_TYPE_NUM_256);
    c = lm_word_sub(r->Number,a->Number,b->Number,LM_SP_NUM_256_BYTES);
    if(c)
    {
        return 0;
    }
    return 1;
}
/*
 ****************************************************************************************
 * @brief This function performs the addition of two 192 bit numbers
 *
 * @param[in] r    The result of addition
 * @param[in] a    The operand for the addition
 * @param[in] b    The operand for the addition
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_uadd(lm_sp_num_384 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    uint32_t c=0;

    lm_n_zero(r,LM_SP_TYPE_NUM_384);
    c = lm_word_add(r->Number,a->Number,b->Number,LM_SP_NUM_192_BYTES);
    r->Number[LM_SP_NUM_192_BYTES]=c;
    return 1;
}

static int lm_n256_uadd(lm_sp_num_512 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    uint32_t c=0;

    lm_n_zero(r,LM_SP_TYPE_NUM_512);
    c = lm_word_add(r->Number,a->Number,b->Number,LM_SP_NUM_256_BYTES);
    r->Number[LM_SP_NUM_256_BYTES]=c;
    return 1;
}

/*
 ****************************************************************************************
 * @brief This function does the subtraction of two signed 192 bit numbers
 *
 * @param[in] r       The result of subtraction
 * @param[in] a       The operand for the subtraction
 * @param[in] b       The operand for the subtraction
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_sub(lm_sp_num_384* r,lm_sp_num_192 *a,lm_sp_num_192 *b)
{
    int add=0,neg=0;
    lm_sp_num_192*tmp;

    if (a->Neg)
    {
        if (b->Neg)
        { 
            tmp=a; 
            a=b; 
            b=tmp; 
        }
        else
        { 
            add=1; 
            neg=1; 
        }
    }
    else
    {
        if (b->Neg) 
        { 
            add=1; 
            neg=0; 
        }
    }

    if (add)
    {
        if (!lm_n192_uadd(r,a,b))
        {
            return(0);
        }
        r->Neg=neg;
        return(1);
    }

    if (lm_n192_ucmp(a,b) < 0)
    {
        if (!lm_n192_usub((lm_sp_num_192*)r,b,a))
        {
            return(0);
        }
        r->Neg=1;
    }
    else
    {
        if (!lm_n192_usub((lm_sp_num_192*)r,a,b))
        {
            return(0);
        }
        r->Neg=0;
    }
    return(1);
}


static int lm_n256_sub(lm_sp_num_512* r,lm_sp_num_256 *a,lm_sp_num_256 *b)
{
    int add=0,neg=0;
    lm_sp_num_256*tmp;

    if (a->Neg)
    {
        if (b->Neg)
        { 
            tmp=a; 
            a=b; 
            b=tmp; 
        }
        else
        { 
            add=1; 
            neg=1; 
        }
    }
    else
    {
        if (b->Neg) 
        { 
            add=1; 
            neg=0; 
        }
    }

    if (add)
    {
        if (!lm_n256_uadd(r,a,b))
        {
            return(0);
        }
        r->Neg=neg;
        return(1);
    }

    if (lm_n256_ucmp(a,b) < 0)
    {
        if (!lm_n256_usub((lm_sp_num_256*)r,b,a))
        {
            return(0);
        }
        r->Neg=1;
    }
    else
    {
        if (!lm_n256_usub((lm_sp_num_256*)r,a,b))
        {
            return(0);
        }
        r->Neg=0;
    }
    return(1);
}


/*
 ****************************************************************************************
 * @brief This function does the addition of two signed 192 bit numbers
 *
 * @param[in] r        The result of addition
 * @param[in] a        The operand for the addition
 * @param[in] b        The operand for the addition
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_add(lm_sp_num_384 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    lm_sp_num_192 *tmp;
    int a_Neg = a->Neg, ret;

    if (a_Neg ^ b->Neg)
    {
        if (a_Neg)
        { 
            tmp=a; 
            a=b; 
            b=tmp; 
        }

        if (lm_n192_ucmp(a,b) < 0)
        {
            if (!lm_n192_usub((lm_sp_num_192*)r,b,a))
            {
                return(0);
            }
            r->Neg=1;
        }
        else
        {
            if (!lm_n192_usub((lm_sp_num_192*)r,a,b))
            {
                return(0);
            }
            r->Neg=0;
        }
        return(1);
    }

    ret = lm_n192_uadd(r,a,b);
    r->Neg = a_Neg;
    return ret;
}

static int lm_n256_add(lm_sp_num_512 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    lm_sp_num_256 *tmp;
    int a_Neg = a->Neg, ret;

    if (a_Neg ^ b->Neg)
    {
        if (a_Neg)
        { 
            tmp=a; 
            a=b; 
            b=tmp; 
        }

        if (lm_n256_ucmp(a,b) < 0)
        {
            if (!lm_n256_usub((lm_sp_num_256*)r,b,a))
            {
                return(0);
            }
            r->Neg=1;
        }
        else
        {
            if (!lm_n256_usub((lm_sp_num_256*)r,a,b))
            {
                return(0);
            }
            r->Neg=0;
        }
        return(1);
    }

    ret = lm_n256_uadd(r,a,b);
    r->Neg = a_Neg;
    return ret;
}


/*
 ****************************************************************************************
 * @brief This function performs the left shift by one for 192 bit number mul by 2
 *
 * @param[in] r      The result of shifting
 * @param[in] a      The operand for the shifting
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_lshift1(lm_sp_num_384 *r, lm_sp_num_192 *a)
{
    uint32_t *ap,*rp,t,c;
    int i;

    if(r!=(lm_sp_num_384*)a)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_384);
        lm_n_copy(r,a,LM_SP_TYPE_NUM_192);
    }
    ap=a->Number;
    rp=r->Number;
    c=0;

    for (i=0; i<LM_SP_NUM_192_BYTES; i++)
    {
        t= *(ap++);
        *(rp++)=((t<<1)|c);
        c=(t & LM_SP_NUM_192_SIGN_BIT)?1:0;
    }
    if (c)
    {
        *rp=1;
    }
    return(1);
}

static int lm_n256_lshift1(lm_sp_num_512 *r, lm_sp_num_256 *a)
{
    uint32_t *ap,*rp,t,c;
    int i;

    if(r!=(lm_sp_num_512*)a)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_512);
        lm_n_copy(r,a,LM_SP_TYPE_NUM_256);
    }
    ap=a->Number;
    rp=r->Number;
    c=0;

    for (i=0; i<LM_SP_NUM_256_BYTES; i++)
    {
        t= *(ap++);
        *(rp++)=((t<<1)|c);
        c=(t & LM_SP_NUM_256_SIGN_BIT)?1:0;
    }
    if (c)
    {
        *rp=1;
    }
    return(1);
}


/*
 ****************************************************************************************
 * @brief This function performs the right shift by one
 *
 * @param[in] r        The result of shifting
 * @param[in] a        The operand for the shifting
 * @param[in] Type     Type of the operand
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_rshift1(void *r, void *a,uint32_t type)
{
    uint32_t *ap,*rp,t,c;
    int i;

    if (lm_n_is_zero(a,type))
    {
        lm_n_zero(r,type);
        return(1);
    }
    if (a != r)
    {
        ((lm_sp_num_384*)r)->Neg=((lm_sp_num_384*)a)->Neg;
    }

    ap=((lm_sp_num_384*)a)->Number;
    rp=((lm_sp_num_384*)r)->Number;
    c=0;
    type *= LM_SP_NUM_192_BYTES;
    type -=1;
    for (i=type; i>=0; i--)
    {
        t=ap[i];
        rp[i]=(t>>1)|c;
        c=(t&1)?LM_SP_NUM_192_SIGN_BIT:0;
    }
    return(1);
}

static int lm_n256_rshift1(void *r, void *a,uint32_t type)
{
    uint32_t *ap,*rp,t,c;
	int i;

    if (lm_n_is_zero(a,type))
    {
        lm_n_zero(r,type);
        return(1);
    }
    if (a != r)
    {
        ((lm_sp_num_512*)r)->Neg=((lm_sp_num_512*)a)->Neg;
    }

    ap=((lm_sp_num_512*)a)->Number;
    rp=((lm_sp_num_512*)r)->Number;
    c=0;
    type = (type-2)*LM_SP_NUM_256_BYTES;
    type -=1;
    for (i=type; i>=0; i--)
    {
        t=ap[i];
        rp[i]=(t>>1)|c;
        c=(t&1)?LM_SP_NUM_256_SIGN_BIT:0;
    }
    return(1);
}


/*
 ****************************************************************************************
 * @brief This function performs the left shift operation.
 *
 * @param[in] r    The result
 * @param[in] a    The operand
 * @param[in] n    The number of bits to be shifted
 * @param[in] type Type of the number
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_lshift(lm_sp_num_384 *r, void *a, uint16_t n,uint32_t type)
{
    int i,nw,lb,rb;
    uint32_t *t,*f;
    uint32_t l;
    
    type *= LM_SP_NUM_192_BYTES;
    if(r!=a)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_384);
    }

    
    if(n>(192*type))
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_384);
        return 1;
    }

    r->Neg=((lm_sp_num_384*)a)->Neg;

    nw=n/LM_SP_NUM_192_BITS;
    lb=n%LM_SP_NUM_192_BITS;
    rb=LM_SP_NUM_192_BITS-lb;
    f= ((lm_sp_num_384*)a)->Number;
    t=r->Number;

    if (lb == 0)
    {
        for (i=(type-1); i>=0; i--)
        {
            t[nw+i]=f[i];
        }
    }
    else
    {
        for (i=(type-1); i>=0; i--)
        {
            l=f[i];
            t[nw+i+1]|=(l>>rb);
            t[nw+i]=(l<<lb);
        }
    }
    memset(t,0,nw*sizeof(t[0]));
    return(1);
}



static int lm_n256_lshift(lm_sp_num_512 *r, void *a, uint16_t n,uint32_t type)
{
    int i,nw,lb,rb;
    uint32_t *t,*f;
    uint32_t l;
    //uint32_t NUM_BYTES;
    //uint32_t NUM_BITS;

	if(type<2 || type >3 )
		return 0;

    if(type == 3)
		type = LM_SP_NUM_256_BYTES;
	else
		type = 2*LM_SP_NUM_256_BYTES;


    if(r!=a)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_512);
    }

    
    if(n>(256*(type-2)))
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_512);
        return 1;
    }

    r->Neg=((lm_sp_num_512*)a)->Neg;

    nw=n/LM_SP_NUM_256_BITS;
    lb=n%LM_SP_NUM_256_BITS;
    rb=LM_SP_NUM_256_BITS-lb;
    f= ((lm_sp_num_512*)a)->Number;
    t=r->Number;

    if (lb == 0)
    {
        for (i=(type-1); i>=0; i--)
        {
            t[nw+i]=f[i];
        }
    }
    else
    {
        for (i=(type-1); i>=0; i--)
        {
            l=f[i];
            t[nw+i+1]|=(l>>rb);
            t[nw+i]=(l<<lb);
        }
    }
    memset(t,0,nw*sizeof(t[0]));
    return(1);
}


/*
 ****************************************************************************************
 * @brief This function performs the right shift operation
 *
 * @param[in] r       The result
 * @param[in] a       The operand
 * @param[in] n       The number of bits to be shifted
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_rshift(lm_sp_num_192 *r,lm_sp_num_192 *a, int n)
{
    int i,j,nw,lb,rb;
    uint32_t *t,*f;
    uint32_t l,tmp;

    nw=n/LM_SP_NUM_192_BITS;
    rb=n%LM_SP_NUM_192_BITS;
    lb=LM_SP_NUM_192_BITS-rb;

    if (n  >  192)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_192);
        return(1);
    }
    
    if (r != a)
    {
        r->Neg=a->Neg;
    }
    else
    {
        if (n == 0)
            return 1;
    }

    f= &(a->Number[nw]);
    t=r->Number;
    j=LM_SP_NUM_192_BYTES-nw;

    if (rb == 0)
    {
        for (i=j; i != 0; i--)
            *(t++)= *(f++);
    }
    else
    {
        l= *(f++);
        for (i=j-1; i != 0; i--)
        {
            tmp =(l>>rb);
            l= *(f++);
            *(t++) =(tmp|(l<<lb));
        }
        *(t++) =(l>>rb);
    }
    return(1);
}



static int lm_n256_rshift(lm_sp_num_256 *r,lm_sp_num_256 *a, int n)
{
    int i,j,nw,lb,rb;
    uint32_t *t,*f;
    uint32_t l,tmp;

    nw=n/LM_SP_NUM_256_BITS;
    rb=n%LM_SP_NUM_256_BITS;
    lb=LM_SP_NUM_256_BITS-rb;

    if (n  >  256)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_256);
        return(1);
    }
    
    if (r != a)
    {
        r->Neg=a->Neg;
    }
    else
    {
        if (n == 0)
            return 1;
    }

    f= &(a->Number[nw]);
    t=r->Number;
    j=LM_SP_NUM_256_BYTES-nw;

    if (rb == 0)
    {
        for (i=j; i != 0; i--)
            *(t++)= *(f++);
    }
    else
    {
        l= *(f++);
        for (i=j-1; i != 0; i--)
        {
            tmp =(l>>rb);
            l= *(f++);
            *(t++) =(tmp|(l<<lb));
        }
        *(t++) =(l>>rb);
    }
    return(1);
}

/*
 ****************************************************************************************
 * @brief This function multiplies two lm_sp_num_192numbers.
 *
 * @param[in] r      The result
 * @param[in] a      The operand for the Mul
 * @param[in] b      The operand for the Mul
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_mul(lm_sp_num_384* r,lm_sp_num_192 *a,lm_sp_num_192 *b)
{
    uint16_t i;
    uint32_t *rr;
    uint16_t  na,nb;

    na = lm_n_get_words(a,LM_SP_TYPE_NUM_192);
    nb = lm_n_get_words(b,LM_SP_TYPE_NUM_192);
    lm_n_zero(r,LM_SP_TYPE_NUM_384);
    if( (!na) || (!nb))
    {
        return 1;
    }

    if(na<nb)
    {
        i = na;
        na =nb;
        nb = i;

        rr = (uint32_t *)a;
        a = b;
        b = (lm_sp_num_192*) rr;

    }
    rr = &(r->Number[na]);

    if(nb==0)
    {
        lm_word_mul(r->Number,a->Number,na,0);
        return 1;
    
    }
    else
    {
        rr[0]=lm_word_mul(r->Number,a->Number,na,b->Number[0]);
    }
    
    for (i=0;i<(nb-1);i++)
    {
        rr[i+1]=lm_word_mul_add(&(r->Number[i+1]),(a->Number),na, b->Number[i+1]);
    }    
    return 1;    
}


static int lm_n256_mul(lm_sp_num_512* r,lm_sp_num_256 *a,lm_sp_num_256 *b)
{
    uint16_t i;
    uint32_t *rr;
    uint16_t  na,nb;

    na = lm_n_get_words(a,LM_SP_TYPE_NUM_256);
    nb = lm_n_get_words(b,LM_SP_TYPE_NUM_256);
    lm_n_zero(r,LM_SP_TYPE_NUM_384);
    if( (!na) || (!nb))
    {
        return 1;
    }

    if(na<nb)
    {
        i = na;
        na =nb;
        nb = i;

        rr = (uint32_t *)a;
        a = b;
        b = (lm_sp_num_256*) rr;

    }
    rr = &(r->Number[na]);

    if(nb==0)
    {
        lm_word_mul(r->Number,a->Number,na,0);
        return 1;
    
    }
    else
    {
        rr[0]=lm_word_mul(r->Number,a->Number,na,b->Number[0]);
    }
    
    for (i=0;i<(nb-1);i++)
    {
        rr[i+1]=lm_word_mul_add(&(r->Number[i+1]),(a->Number),na, b->Number[i+1]);
    }    
    return 1;    
}

/*
 ****************************************************************************************
 * @brief This function squares two lm_sp_num_192 numbers.
 *
 * @param[in] lm_sp_num_384* r,The result
 * @param[in] lm_sp_num_192* a, The operand for the Sqr
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_n192_sqr(lm_sp_num_384 *r, lm_sp_num_192 *a)
{
    lm_sp_num_384 tmp;
    int i,j,max,n;
    uint32_t *ap;
    uint32_t *rp;

    r->Neg=0;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_384);
    n=lm_n_get_words(a,LM_SP_TYPE_NUM_192);
    max=n<<1;
    ap = a->Number;
    rp=r->Number;
    rp[0]=rp[max-1]=0;
    rp++;
    j=n;

    if (--j > 0)
    {
        ap++;
        rp[j]=lm_word_mul(rp,ap,(uint16_t) j,ap[-1]);
        rp+=2;
    }

    for (i=n-2; i>0; i--)
    {
        j--;
        ap++;
        rp[j]=lm_word_mul_add(rp,ap,(uint16_t) j,ap[-1]);
        rp+=2;
    }

    lm_word_add(r->Number,r->Number,r->Number,max);
    lm_word_sqr(tmp.Number,a->Number,n);
    lm_word_add(r->Number,r->Number,tmp.Number,max);
    return 1;
}

static int lm_n256_sqr(lm_sp_num_512 *r, lm_sp_num_256 *a)
{
    lm_sp_num_512 tmp;
    int i,j,max,n;
    uint32_t *ap;
    uint32_t *rp;

    r->Neg=0;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_512);
    n=lm_n_get_words(a,LM_SP_TYPE_NUM_256);
    max=n<<1;
    ap = a->Number;
    rp=r->Number;
    rp[0]=rp[max-1]=0;
    rp++;
    j=n;

    if (--j > 0)
    {
        ap++;
        rp[j]=lm_word_mul(rp,ap,(uint16_t) j,ap[-1]);
        rp+=2;
    }

    for (i=n-2; i>0; i--)
    {
        j--;
        ap++;
        rp[j]=lm_word_mul_add(rp,ap,(uint16_t) j,ap[-1]);
        rp+=2;
    }

    lm_word_add(r->Number,r->Number,r->Number,max);
    lm_word_sqr(tmp.Number,a->Number,n);
    lm_word_add(r->Number,r->Number,tmp.Number,max);
    return 1;
}


/*
 ****************************************************************************************
 * @brief This function does the subtraction of two signed 192 bit numbers from
 *        384 bit number
 *
 * @param[in] r    The result of subtraction
 * @param[in] a    The operand for the subtraction
 * @param[in] b    The operand for the subtraction
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n384_192_sub(lm_sp_num_192 *r, lm_sp_num_384 *a, lm_sp_num_192 *b)
{
    int c=0;
    c = lm_word_sub(r->Number,a->Number,b->Number,LM_SP_NUM_192_BYTES);
    if(c)
    {
        if(a->Number[LM_SP_NUM_192_BYTES])
        {
            a->Number[LM_SP_NUM_192_BYTES]-=c;
            if(a->Number[LM_SP_NUM_192_BYTES])
            {
                return 0;
            }
        }
    }
    return 1;

}

static int lm_n512_256_sub(lm_sp_num_256 *r, lm_sp_num_512 *a, lm_sp_num_256 *b)
{
    int c=0;
    c = lm_word_sub(r->Number,a->Number,b->Number,LM_SP_NUM_256_BYTES);
    if(c)
    {
        if(a->Number[LM_SP_NUM_256_BYTES])
        {
            a->Number[LM_SP_NUM_256_BYTES]-=c;
            if(a->Number[LM_SP_NUM_256_BYTES])
            {
                return 0;
            }
        }
    }
    return 1;

}

/*
 ****************************************************************************************
 * @brief This function does the modular addition of two 192 both number.
 *
 * @param[in] r     The result of addition
 * @param[in] a     The operand for the addition
 * @param[in] b     The operand for the addition
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_add(lm_sp_num_192 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    lm_sp_num_384 tmp;

    memset((uint8_t*)&tmp, 0, sizeof(lm_sp_num_384));

    if (!lm_n192_uadd(&tmp,a, b))
    {
        return 0;
    }

    if (lm_n192_384_ucmp(&tmp, (lm_sp_num_192*)&lm_sp_p192_field) >= 0)
    {
        return lm_n384_192_sub(r, &tmp, (lm_sp_num_192*)&lm_sp_p192_field);
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_192);
    }
}

static int lm_n256_mod_add(lm_sp_num_256 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    lm_sp_num_512 tmp;

    memset((uint8_t*)&tmp, 0, sizeof(lm_sp_num_512));

    if (!lm_n256_uadd(&tmp,a, b))
    {
        return 0;
    }

    if (lm_n256_512_ucmp(&tmp, (lm_sp_num_256*)&lm_sp_p256_field) >= 0)
    {
        return lm_n512_256_sub(r, &tmp, (lm_sp_num_256*)&lm_sp_p256_field);
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_256);
    }
}


/*
 ****************************************************************************************
 * @brief This function performs the modular sub.
 *
 * @param[in] r  The result of sub
 * @param[in] a  The operand for the sub
 * @param[in] b  The operand for the sub
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_sub(lm_sp_num_192 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    lm_sp_num_384 tmp;

    memset((uint8_t*)&tmp, 0, sizeof(lm_sp_num_384));

    if (!lm_n192_sub(&tmp, a, b)) return 0;
    if (tmp.Neg)
    {
        return (lm_n192_add((lm_sp_num_384*)r,(lm_sp_num_192*)&tmp,
                        (lm_sp_num_192*)&lm_sp_p192_field));
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_192);
    }
}

static int lm_n256_mod_sub(lm_sp_num_256 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    lm_sp_num_512 tmp;

    memset((uint8_t*)&tmp, 0, sizeof(lm_sp_num_512));

    if (!lm_n256_sub(&tmp, a, b)) return 0;
    if (tmp.Neg)
    {
        return (lm_n256_add((lm_sp_num_512*)r,(lm_sp_num_256*)&tmp,
                        (lm_sp_num_256*)&lm_sp_p256_field));
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_256);
    }
}

/**
 ****************************************************************************************
 * @brief This function is used to find the Modulus with the P-192 field.
 *
 * @param[in] r,The result
 * @param[in] a, The operand for the Modulus
 *
 * @return non-zero on success and vice-versa.
 *
 ****************************************************************************************
 */
static int lm_sp_n192_n_mod(lm_sp_num_192 *r, lm_sp_num_384 *a)
{
    int i;
    uint32_t *res;
    lm_sp_num_384 tmp1;
    lm_sp_num_192 tmp2;

    memset((uint8_t*)&tmp1, 0, sizeof(tmp1));
    memset((uint8_t*)&tmp2, 0, sizeof(tmp2));

    r->Neg=0;
    if(lm_n_is_zero(a,LM_SP_TYPE_NUM_384))
    {
        return lm_n_copy(r,a,LM_SP_TYPE_NUM_384);
    }

    i=lm_n192_384_cmp(a,(lm_sp_num_192*)&lm_sp_p192_field);
    if(i==0)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_192);
        return 1;
    }
    else if (i<0)
    {
        return lm_n_copy(r,a,LM_SP_TYPE_NUM_192);
    }
    i=0;
    if(!lm_n_copy(&tmp1,a,LM_SP_TYPE_NUM_192))
    {
        return 0;
    }
    res = r->Number;

    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_192);
    tmp2.Number[0]= a->Number[6];
    tmp2.Number[1]= a->Number[7];
    tmp2.Number[2]= a->Number[6];
    tmp2.Number[3]= a->Number[7];

    if(lm_word_add(res,tmp1.Number,tmp2.Number,LM_SP_NUM_192_BYTES))
    {
        i++;
    }

    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_192);
    tmp2.Number[2]= a->Number[8];
    tmp2.Number[3]= a->Number[9];
    tmp2.Number[4]= a->Number[8];
    tmp2.Number[5]= a->Number[9];

    if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_192_BYTES))
    {
        i++;
    }

    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_192);
    tmp2.Number[0]= a->Number[10];
    tmp2.Number[1]= a->Number[11];
    tmp2.Number[2]= a->Number[10];
    tmp2.Number[3]= a->Number[11];
    tmp2.Number[4]= a->Number[10];
    tmp2.Number[5]= a->Number[11];

    if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_192_BYTES))
    {
        i++;
    }

    while(i)
    {
        lm_word_sub(res,res,(uint32_t*)lm_sp_p192_field.Number,LM_SP_NUM_192_BYTES);
        --i;
    }
    if(lm_n192_ucmp(r,(lm_sp_num_192*)&lm_sp_p192_field)>=0)
    {
        lm_word_sub(res,res,(uint32_t*)lm_sp_p192_field.Number,LM_SP_NUM_192_BYTES);
    }

    return 1;
}



static int lm_sp_n256_n_mod(lm_sp_num_256 *r, lm_sp_num_512 *a)
{
    int i;
    uint32_t *res;
    lm_sp_num_512 tmp1;
    lm_sp_num_256 tmp2;

    memset((uint8_t*)&tmp1, 0, sizeof(tmp1));
    memset((uint8_t*)&tmp2, 0, sizeof(tmp2));

    r->Neg=0;
    if(lm_n_is_zero(a,LM_SP_TYPE_NUM_512))
    {
        return lm_n_copy(r,a,LM_SP_TYPE_NUM_512);
    }

    i=lm_n256_512_cmp(a,(lm_sp_num_256*)&lm_sp_p256_field);
    if(i==0)
    {
        lm_n_zero(r,LM_SP_TYPE_NUM_256);
        return 1;
    }
    else if (i<0)
    {
        return lm_n_copy(r,a,LM_SP_TYPE_NUM_256);
    }
    i=0;
    if(!lm_n_copy(&tmp1,a,LM_SP_TYPE_NUM_256))
    {
        return 0;
    }
    res = r->Number;
	//s1
    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[3]= a->Number[11];
    tmp2.Number[4]= a->Number[12];
    tmp2.Number[5]= a->Number[13];
    tmp2.Number[6]= a->Number[14];
	tmp2.Number[7]= a->Number[15];

    if(lm_word_add(res,tmp1.Number,tmp2.Number,LM_SP_NUM_256_BYTES)) // t+s1
    {
        i++;
    }
	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES))         // t+2s1
    {
        i++;
    }


	//s2
    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[3]= a->Number[12];
    tmp2.Number[4]= a->Number[13];
    tmp2.Number[5]= a->Number[14];
    tmp2.Number[6]= a->Number[15];

    if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t+2s1+s2
    {
        i++;
    }
	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t+2s1+2s2
    {
        i++;
    }

	//s3
    lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[8];
    tmp2.Number[1]= a->Number[9];
    tmp2.Number[2]= a->Number[10];
    tmp2.Number[6]= a->Number[14];
    tmp2.Number[7]= a->Number[15];

    if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t+2s1+2s2+s3
    {
        i++;
    }

	//s4
	lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[9];
    tmp2.Number[1]= a->Number[10];
    tmp2.Number[2]= a->Number[11];
	tmp2.Number[3]= a->Number[13];
    tmp2.Number[4]= a->Number[14];
    tmp2.Number[5]= a->Number[15];
    tmp2.Number[6]= a->Number[13];
    tmp2.Number[7]= a->Number[8];

    if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4
    {
        i++;
    }

	//d1
	lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[11];
    tmp2.Number[1]= a->Number[12];
    tmp2.Number[2]= a->Number[13];
    tmp2.Number[6]= a->Number[8];
    tmp2.Number[7]= a->Number[10];

	//2p256-d1
	if(lm_n256_ucmp(&tmp2, (lm_sp_num_256*) &lm_sp_p256_field) == 1) // if(d1 > p256) use d1 = 2*p256 - d1
	{
		lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
		lm_word_add(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
	}
	else // use d1 = p256-d1
		lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
	
	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1    (d1 <--- 2p256 - tmp2)
		i++;
   /* if(lm_word_sub(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1    (d1 <--- 2p256 - tmp2)
		i++;
    if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
		i++;
	if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
		i++;*/
    
	//d2
	lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[12];
    tmp2.Number[1]= a->Number[13];
    tmp2.Number[2]= a->Number[14];
	tmp2.Number[3]= a->Number[15];
    tmp2.Number[6]= a->Number[9];
    tmp2.Number[7]= a->Number[11];
	
	if(lm_n256_ucmp(&tmp2, (lm_sp_num_256*) &lm_sp_p256_field) == 1) // if(d1 > p256) use d1 = 2*p256 - d1
	{
		lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
		lm_word_add(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
	}
	else // use d1 = p256-d1
		lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);
	
	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1    (d1 <--- 2p256 - tmp2)
		i++;
	/*if(lm_word_sub(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1 +d2   (d2 <--- 2p256 - tmp2)
		i++;
	if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
		i++;	
	if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
				i++;*/
    
	//d3
	lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[13];
    tmp2.Number[1]= a->Number[14];
    tmp2.Number[2]= a->Number[15];
	tmp2.Number[3]= a->Number[8];
    tmp2.Number[4]= a->Number[9];
	tmp2.Number[5]= a->Number[10];
    tmp2.Number[7]= a->Number[12];

	lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);

	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1 +d2 +d3  (d3 <--- p256 - tmp2)
		i++;

	//if(lm_word_sub(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1 +d2 +d3  (d3 <--- p256 - tmp2)
	//	i++;
    //if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
	//	i++;

	//d4
	lm_n_zero(&tmp2,LM_SP_TYPE_NUM_256);
    tmp2.Number[0]= a->Number[14];
    tmp2.Number[1]= a->Number[15];
	tmp2.Number[3]= a->Number[9];
    tmp2.Number[4]= a->Number[10];
	tmp2.Number[5]= a->Number[11];
    tmp2.Number[7]= a->Number[13];
	lm_word_sub(tmp2.Number, (uint32_t*)lm_sp_p256_field.Number, tmp2.Number, LM_SP_NUM_256_BYTES);

	if(lm_word_add(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1 +d2 +d3 +d4  (d4 <--- p256 - tmp2)
		i++;

	//if(lm_word_sub(res,res,tmp2.Number,LM_SP_NUM_256_BYTES)) //t + 2s1 + 2s2 + s3 + s4 +d1 +d2 +d3 +d4  (d4 <--- p256 - tmp2)
	//	i++;
	//if(lm_word_add(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES))
	//	i++;


    while(i)
    {
        lm_word_sub(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES);
        --i;
    }
    if(lm_n256_ucmp(r,(lm_sp_num_256*)&lm_sp_p256_field)>=0)
    {
        lm_word_sub(res,res,(uint32_t*)lm_sp_p256_field.Number,LM_SP_NUM_256_BYTES);
    }

    return 1;
}



/*
 ****************************************************************************************
 * @brief This function performs the modular mul
 *
 * @param[in] r    The result of MUL
 * @param[in] a    The operand for the MUL
 * @param[in] b    The operand for the MUL
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_mul(lm_sp_num_192 *r, lm_sp_num_192 *a, lm_sp_num_192 *b)
{
    lm_sp_num_384 t;

    memset((uint8_t*)&t, 0, sizeof(t));

    if (a == b)
    { 
        if (!lm_n192_sqr(&t,a))
            return 0;
    }
    else
    { 
        if (!lm_n192_mul(&t,a,b))
        {
            return 0;
        }
    }
    if (!lm_sp_n192_n_mod(r,&t))
    {
        return 0;
    }
    return 1;
}

static int lm_n256_mod_mul(lm_sp_num_256 *r, lm_sp_num_256 *a, lm_sp_num_256 *b)
{
    lm_sp_num_512 t;

    memset((uint8_t*)&t, 0, sizeof(t));

    if (a == b)
    { 
        if (!lm_n256_sqr(&t,a))
            return 0;
    }
    else
    { 
        if (!lm_n256_mul(&t,a,b))
        {
            return 0;
        }
    }
    if (!lm_sp_n256_n_mod(r,&t))
    {
        return 0;
    }
    return 1;
}

/*
 ****************************************************************************************
 * @brief This function performs the modular sqr
 *
 * @param[in] r     The result of sqr
 * @param[in] a     The operand for the sqr
 * @param[in] b     The operand for the sqr
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_sqr(lm_sp_num_192 *r, lm_sp_num_192 *a)
{
    lm_sp_num_384 tmp;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_384);
    if (!lm_n192_sqr(&tmp, a))
    {
        return 0;
    }
    return lm_sp_n192_n_mod(r, &tmp);
}

static int lm_n256_mod_sqr(lm_sp_num_256 *r, lm_sp_num_256 *a)
{
    lm_sp_num_512 tmp;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_512);
    if (!lm_n256_sqr(&tmp, a))
    {
        return 0;
    }
    return lm_sp_n256_n_mod(r, &tmp);
}

/*
 ****************************************************************************************
 * @brief This function performs the modular left shift or mul by 2
 *
 * @param[in] r      The result of sqr
 * @param[in] a      The operand for the sqr
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_lshift1_quick(lm_sp_num_192 *r, lm_sp_num_192 *a)
{
    lm_sp_num_384 tmp;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_384);
    if (!lm_n192_lshift1(&tmp, a))
    {
        return 0;
    }
    if (lm_n192_384_cmp(&tmp, (lm_sp_num_192*)&lm_sp_p192_field) >= 0)
    {
        return lm_n384_192_sub(r, &tmp,(lm_sp_num_192*)&lm_sp_p192_field);
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_192);
    }
}

static int lm_n256_mod_lshift1_quick(lm_sp_num_256 *r, lm_sp_num_256 *a)
{
    lm_sp_num_512 tmp;
    lm_n_zero(&tmp,LM_SP_TYPE_NUM_512);
    if (!lm_n256_lshift1(&tmp, a))
    {
        return 0;
    }
    if (lm_n256_512_cmp(&tmp, (lm_sp_num_256*)&lm_sp_p256_field) >= 0)
    {
        return lm_n512_256_sub(r, &tmp,(lm_sp_num_256*)&lm_sp_p256_field);
    }
    else
    {
        return lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_256);
    }
}

/*
 ****************************************************************************************
 * @brief This function performs the modular left shift.
 *
 * @param[in] r       The result of sqr
 * @param[in] a       The operand for the sqr
 * @param[in] n       The number of bits to be shifted
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_lshift_quick(lm_sp_num_192 *r, lm_sp_num_192 *a, int n)
{
    lm_sp_num_384 tmp;

    lm_n_zero(&tmp,LM_SP_TYPE_NUM_384);
    lm_n_copy(&tmp,a,LM_SP_TYPE_NUM_192);

    while (n > 0)
    {
        int max_shift;

        max_shift = LM_SP_NUM_192_BITS*LM_SP_NUM_192_BYTES - lm_n_num_bits(&tmp,LM_SP_TYPE_NUM_192);

        if (max_shift < 0)
        {
            return 0;
        }

        if (max_shift > n)
        {
            max_shift = n;
        }

        if (max_shift)
        {
            if (!lm_n192_lshift(&tmp, &tmp, max_shift,LM_SP_TYPE_NUM_192))
            {
                return 0;
            }
            n -= max_shift;
        }
        else
        {
            if (!lm_n192_lshift1(&tmp, (lm_sp_num_192*)&tmp))
            {
                return 0;
            }
            --n;
        }
        if (lm_n192_384_cmp(&tmp, (lm_sp_num_192*)&lm_sp_p192_field) >= 0)
        {
            if (!lm_n384_192_sub((lm_sp_num_192*)&tmp,
                               &tmp, 
                               (lm_sp_num_192*)&lm_sp_p192_field))
            {
                return 0;
            }
        }
    }
    lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_192);
    return 1;
}


static int lm_n256_mod_lshift_quick(lm_sp_num_256 *r, lm_sp_num_256 *a, int n)
{
    lm_sp_num_512 tmp;

    lm_n_zero(&tmp,LM_SP_TYPE_NUM_512);
    lm_n_copy(&tmp,a,LM_SP_TYPE_NUM_256);

    while (n > 0)
    {
        int max_shift;

        max_shift = LM_SP_NUM_256_BITS*LM_SP_NUM_256_BYTES - lm_n_num_bits(&tmp,LM_SP_TYPE_NUM_256);

        if (max_shift < 0)
        {
            return 0;
        }

        if (max_shift > n)
        {
            max_shift = n;
        }

        if (max_shift)
        {
            if (!lm_n256_lshift(&tmp, &tmp, max_shift,LM_SP_TYPE_NUM_256))
            {
                return 0;
            }
            n -= max_shift;
        }
        else
        {
            if (!lm_n256_lshift1(&tmp, (lm_sp_num_256*)&tmp))
            {
                return 0;
            }
            --n;
        }
        if (lm_n256_512_cmp(&tmp, (lm_sp_num_256*)&lm_sp_p256_field) >= 0)
        {
            if (!lm_n512_256_sub((lm_sp_num_256*)&tmp,
                               &tmp, 
                               (lm_sp_num_256*)&lm_sp_p256_field))
            {
                return 0;
            }
        }
    }
    lm_n_copy(r,&tmp,LM_SP_TYPE_NUM_256);
    return 1;
}


/*
 ****************************************************************************************
 * @brief This function returns the inverse of a number i.e. solves ax=1.
 *
 * @param[in] r         The result of sqr
 * @param[in] a         The operand for the sqr
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
static int lm_n192_mod_inverse(lm_sp_num_192 *in,lm_sp_num_192 *a)
{
    lm_sp_num_192 X,Y;
    lm_sp_num_192 A;
    lm_sp_num_192 B;
    uint16_t shift;

    memset((uint8_t*)&A, 0, sizeof(A));
    memset((uint8_t*)&B, 0, sizeof(B));

    lm_sp_n_one (&X,LM_SP_TYPE_NUM_192);
    lm_n_zero(&Y,LM_SP_TYPE_NUM_192);

    if (!lm_n_copy(&B,a,LM_SP_TYPE_NUM_192))
    {
        return 0;
    }
    if (!lm_n_copy(&A,(lm_sp_num_192*)&lm_sp_p192_field,LM_SP_TYPE_NUM_192))
    {
        return 0;
    }
    
    A.Neg = 0;

    if (B.Neg || (lm_n192_ucmp(&B, &A) >= 0))
    {
        lm_sp_num_384 tmp;
        lm_n_zero(&tmp,LM_SP_TYPE_NUM_384);
        lm_n_copy(&tmp,&B,LM_SP_TYPE_NUM_192);
        if (!lm_sp_n192_n_mod(&B, (lm_sp_num_384*)&tmp))
        {
            return 0;
        }
    }
    while (!lm_n_is_zero(&B,LM_SP_TYPE_NUM_192))
    {
        shift = 0;
        while (!lm_n_is_set(&B,shift,LM_SP_TYPE_NUM_192))
        {
            shift++;
            if (X.Number[0]&0x01)
            {
                lm_sp_num_384 tmp;
                if (!lm_n192_add(&tmp, &X,(lm_sp_num_192*)&lm_sp_p192_field))
                {
                    return 0;
                }
                if (!lm_n192_rshift1(&tmp, &tmp, LM_SP_TYPE_NUM_384))
                {
                    return 0;
                }
                if(!lm_sp_n192_n_mod(&X,&tmp))
                {
                    return 0;
                }
            }
            else
            {
                if(!lm_n192_rshift1(&X, &X, LM_SP_TYPE_NUM_192))
                {
                    return 0;
                }
            }
        }
        if (shift > 0)
        {
            if (!lm_n192_rshift(&B, &B, shift))
            {
                return 0;
            }
        }

        shift = 0;
        while (!lm_n_is_set(&A,shift,LM_SP_TYPE_NUM_192))
        {
            shift++;

            if (Y.Number[0]&0x01)
            {
                lm_sp_num_384 tmp;
                if (!lm_n192_add(&tmp, &Y, (lm_sp_num_192*)&lm_sp_p192_field))
                {
                    return 0;
                }
                if (!lm_n192_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_384))
                {
                    return 0;
                }
                if(!lm_sp_n192_n_mod(&Y,&tmp))
                {
                    return 0;
                }
            }
            else
            {
                if(!lm_n192_rshift1(&Y, &Y, LM_SP_TYPE_NUM_192))
                {
                    return 0;
                }
            }
        }
        if (shift > 0)
        {
            if (!lm_n192_rshift(&A, &A, shift))
            {
                return 0;
            }
        }

        if (lm_n192_ucmp(&B, &A) >= 0)
        {
            if (!lm_n192_mod_sub(&X, &X, &Y))
            {
                return 0;
            }
            if (!lm_n192_mod_sub(&B, &B, &A))
            {
                return 0;
            }
        }
        else
        {
            if (!lm_n192_mod_sub(&Y, &Y, &X))
            {
                return 0;
            }
            if (!lm_n192_mod_sub(&A, &A, &B))
            {
                return 0;
            }
        }
    }
    if (lm_n_is_one(&A,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(in,&Y,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return 1;
}

static int lm_n256_mod_inverse(lm_sp_num_256 *in,lm_sp_num_256 *a)
{
    lm_sp_num_256 X,Y;
    lm_sp_num_256 A;
    lm_sp_num_256 B;
    uint16_t shift;

    memset((uint8_t*)&A, 0, sizeof(A));
    memset((uint8_t*)&B, 0, sizeof(B));

    lm_sp_n_one (&X,LM_SP_TYPE_NUM_256);
    lm_n_zero(&Y,LM_SP_TYPE_NUM_256);

    if (!lm_n_copy(&B,a,LM_SP_TYPE_NUM_256))
    {
        return 0;
    }
    if (!lm_n_copy(&A,(lm_sp_num_256*)&lm_sp_p256_field,LM_SP_TYPE_NUM_256))
    {
        return 0;
    }
    
    A.Neg = 0;

    if (B.Neg || (lm_n256_ucmp(&B, &A) >= 0))
    {
        lm_sp_num_512 tmp;
        lm_n_zero(&tmp,LM_SP_TYPE_NUM_512);
        lm_n_copy(&tmp,&B,LM_SP_TYPE_NUM_256);
        if (!lm_sp_n256_n_mod(&B, (lm_sp_num_512*)&tmp))
        {
            return 0;
        }
    }
    while (!lm_n_is_zero(&B,LM_SP_TYPE_NUM_256))
    {
        shift = 0;
        while (!lm_n_is_set(&B,shift,LM_SP_TYPE_NUM_256))
        {
            shift++;
            if (X.Number[0]&0x01)
            {
                lm_sp_num_512 tmp;
                if (!lm_n256_add(&tmp, &X,(lm_sp_num_256*)&lm_sp_p256_field))
                {
                    return 0;
                }
                if (!lm_n256_rshift1(&tmp, &tmp, LM_SP_TYPE_NUM_512))
                {
                    return 0;
                }
                if(!lm_sp_n256_n_mod(&X,&tmp))
                {
                    return 0;
                }
            }
            else
            {
                if(!lm_n256_rshift1(&X, &X, LM_SP_TYPE_NUM_256))
                {
                    return 0;
                }
            }
        }
        if (shift > 0)
        {
            if (!lm_n256_rshift(&B, &B, shift))
            {
                return 0;
            }
        }

        shift = 0;
        while (!lm_n_is_set(&A,shift,LM_SP_TYPE_NUM_256))
        {
            shift++;

            if (Y.Number[0]&0x01)
            {
                lm_sp_num_512 tmp;
                if (!lm_n256_add(&tmp, &Y, (lm_sp_num_256*)&lm_sp_p256_field))
                {
                    return 0;
                }
                if (!lm_n256_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_512))
                {
                    return 0;
                }
                if(!lm_sp_n256_n_mod(&Y,&tmp))
                {
                    return 0;
                }
            }
            else
            {
                if(!lm_n256_rshift1(&Y, &Y, LM_SP_TYPE_NUM_256))
                {
                    return 0;
                }
            }
        }
        if (shift > 0)
        {
            if (!lm_n256_rshift(&A, &A, shift))
            {
                return 0;
            }
        }

        if (lm_n256_ucmp(&B, &A) >= 0)
        {
            if (!lm_n256_mod_sub(&X, &X, &Y))
            {
                return 0;
            }
            if (!lm_n256_mod_sub(&B, &B, &A))
            {
                return 0;
            }
        }
        else
        {
            if (!lm_n256_mod_sub(&Y, &Y, &X))
            {
                return 0;
            }
            if (!lm_n256_mod_sub(&A, &A, &B))
            {
                return 0;
            }
        }
    }
    if (lm_n_is_one(&A,LM_SP_TYPE_NUM_256))
    {
		//memset(&in,0,sizeof(lm_sp_num_256));
		//memcpy(&in,&Y,sizeof(lm_sp_num_256));
        if (!lm_n_copy(in,&Y,LM_SP_TYPE_NUM_256))
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



/*
 ****************************************************************************************
 * @brief This function checks of the point is at infinity
 *
 * @param[in] p        Input
 *
 * @return non zero if point is at infinity and vice-versa
 *
 ****************************************************************************************
 */
static int lm_p192_is_point_at_inf(lm_sp_p192_point *p)
{
    return lm_n_is_zero(&p->Z,LM_SP_TYPE_NUM_192);
}

static int lm_p256_is_point_at_inf(lm_sp_p256_point *p)
{
    return lm_n_is_zero(&p->Z,LM_SP_TYPE_NUM_256);
}

void lm_sp_sha256_calculate(uint8_t* Input_Message,uint32_t* SHA_256_Hash,uint32_t Length)
{
    uint8_t Num_Message_Chunk = lm_sha_256_pad_msg(Input_Message,(uint16_t)Length);

    /* Initialize the hash                                                                */
    memcpy(SHA_256_Hash, lm_sp_sha_256hash_init,sizeof(uint32_t)*8);

    lm_sha_256_calc_hash(SHA_256_Hash,Input_Message,Num_Message_Chunk);
}

void lm_sp_n_one(void* a,uint32_t type)
{
	 if(type==4)
    {
        memset(a,0,sizeof(lm_sp_num_512));
    }
    else if(type==3)
    {
        memset(a,0,sizeof(lm_sp_num_256));
    }
    else if(type==2)
    {
        memset(a,0,sizeof(lm_sp_num_384));
    }
    else
    {
        memset(a,0,sizeof(lm_sp_num_192));
    }
    ((lm_sp_num_192*)a)->Number[0] = 1;
}

int lm_sp_n192_convert_wnaf(signed char *r, lm_sp_num_192* input, uint32_t *ret_len)
{
    
    int window_val;
    uint32_t len, j;

    len = lm_n_num_bits(input,LM_SP_TYPE_NUM_192);
    window_val = input->Number[0] & LM_SP_WNAF_DIGIT_MASK;
    j = 0;
    while ((window_val != 0) || (j + LM_SP_WNAF_WINDOW + 1 < len))
    {

        int digit = 0;
        if (window_val & 1)
        {
            if (window_val & LM_SP_WNAF_MSB_BIT)
            {
                digit = window_val - LM_SP_WNAF_NEXT_DIGIT_BIT;

                if (j + LM_SP_WNAF_WINDOW + 1 >= len)
                {
                    digit = window_val & (LM_SP_WNAF_DIGIT_MASK >> 1);
                }
            }
            else
            {
                digit = window_val;
            }

            if ( digit <= -LM_SP_WNAF_MSB_BIT || 
                 digit >= LM_SP_WNAF_MSB_BIT || !(digit & 1)
               )
            {
                return 0;
            }
            window_val -= digit;
            if ( window_val != 0 && 
                 window_val != LM_SP_WNAF_NEXT_DIGIT_BIT && 
                 window_val != LM_SP_WNAF_MSB_BIT
               )
            {
                return 0;
            }
        }

        r[j++] = digit;

        window_val >>= 1;
        window_val += LM_SP_WNAF_MSB_BIT * lm_n_is_set(input, (uint16_t)(j + LM_SP_WNAF_WINDOW),LM_SP_TYPE_NUM_192);

        if (window_val > LM_SP_WNAF_NEXT_DIGIT_BIT)
        {
            return 0;
        }
    }

    if (j > len + 1)
    {
        return 0;
    }
    len = j;
    *ret_len = len;    
    return 1;
}


int lm_sp_n256_convert_wnaf(signed char *r, lm_sp_num_256* input, uint32_t *ret_len)
{
    
    int window_val;
    uint32_t len, j;

    len = lm_n_num_bits(input,LM_SP_TYPE_NUM_256);
    window_val = input->Number[0] & LM_SP_WNAF_DIGIT_MASK;
    j = 0;
    while ((window_val != 0) || (j + LM_SP_WNAF_WINDOW + 1 < len))
    {

        int digit = 0;
        if (window_val & 1)
        {
            if (window_val & LM_SP_WNAF_MSB_BIT)
            {
                digit = window_val - LM_SP_WNAF_NEXT_DIGIT_BIT;

                if (j + LM_SP_WNAF_WINDOW + 1 >= len)
                {
                    digit = window_val & (LM_SP_WNAF_DIGIT_MASK >> 1);
                }
            }
            else
            {
                digit = window_val;
            }

            if ( digit <= -LM_SP_WNAF_MSB_BIT || 
                 digit >= LM_SP_WNAF_MSB_BIT || !(digit & 1)
               )
            {
                return 0;
            }
            window_val -= digit;
            if ( window_val != 0 && 
                 window_val != LM_SP_WNAF_NEXT_DIGIT_BIT && 
                 window_val != LM_SP_WNAF_MSB_BIT
               )
            {
                return 0;
            }
        }

        r[j++] = digit;

        window_val >>= 1;
        window_val += LM_SP_WNAF_MSB_BIT * lm_n_is_set(input, (uint16_t)(j + LM_SP_WNAF_WINDOW),LM_SP_TYPE_NUM_256);

        if (window_val > LM_SP_WNAF_NEXT_DIGIT_BIT)
        {
            return 0;
        }
    }

    if (j > len + 1)
    {
        return 0;
    }
    len = j;
    *ret_len = len;    
    return 1;
}





int lm_sp_p192_point_to_inf(lm_sp_p192_point *p)
{
    lm_n_zero(&p->Z,LM_SP_TYPE_NUM_192);
    return 1;
}

int lm_sp_p256_point_to_inf(lm_sp_p256_point *p)
{
    lm_n_zero(&p->Z,LM_SP_TYPE_NUM_256);
    return 1;
}

int lm_sp_p192_point_jacobian_to_affine(lm_sp_p192_point *p)
{
    lm_sp_num_192 Z_1;
    lm_sp_num_192 Z_2;

    memset((uint8_t*)&Z_1, 0, sizeof(Z_1));
    memset((uint8_t*)&Z_2, 0, sizeof(Z_2));

    if (lm_n_is_one(&p->Z,LM_SP_TYPE_NUM_192) || lm_p192_is_point_at_inf(p))
    {
        return 1;
    }

    if(!lm_n192_mod_inverse(&Z_1,&p->Z))
    {
        return 0;
    }
    if(!lm_n192_mod_sqr(&Z_2,&Z_1))
    {
        return 0;
    }
    if(!lm_n192_mod_mul(&p->X,&Z_2,&p->X))
    {
        return 0;
    }
    if(!lm_n192_mod_mul(&Z_2,&Z_2,&Z_1))
    {
        return 0;
    }

    if(!lm_n192_mod_mul(&p->Y,&Z_2,&p->Y))
    {
        return 0;
    }
    lm_sp_n_one(&p->Z,LM_SP_TYPE_NUM_192);
    return 1;
}

int lm_sp_p256_point_jacobian_to_affine(lm_sp_p256_point *p)
{
    lm_sp_num_256 Z_1;
    lm_sp_num_256 Z_2;

    memset((uint8_t*)&Z_1, 0, sizeof(Z_1));
    memset((uint8_t*)&Z_2, 0, sizeof(Z_2));

    if (lm_n_is_one(&p->Z,LM_SP_TYPE_NUM_256) || lm_p256_is_point_at_inf(p))
    {
        return 1;
    }

    if(!lm_n256_mod_inverse(&Z_1,&p->Z))
    {
        return 0;
    }
    if(!lm_n256_mod_sqr(&Z_2,&Z_1))
    {
        return 0;
    }
    if(!lm_n256_mod_mul(&p->X,&Z_2,&p->X))
    {
        return 0;
    }
    if(!lm_n256_mod_mul(&Z_2,&Z_2,&Z_1))
    {
        return 0;
    }

    if(!lm_n256_mod_mul(&p->Y,&Z_2,&p->Y))
    {
        return 0;
    }
    lm_sp_n_one(&p->Z,LM_SP_TYPE_NUM_256);
    return 1;
}


int lm_sp_p192_points_jacobian_to_affine(lm_sp_p192_point *points)
{
    uint32_t i;
    lm_sp_num_192 tmp0;
    lm_sp_num_192 tmp1;
    lm_sp_num_192 heap[8];

    memset((uint8_t*)&tmp0, 0, sizeof(lm_sp_num_192));
    memset((uint8_t*)&tmp1, 0, sizeof(lm_sp_num_192));
    memset(heap,0,sizeof(heap));
    
    for(i=0;i<4;i++)    
    {
        lm_n_copy((heap+4+i), &((points+i)->Z),LM_SP_TYPE_NUM_192);
    }
    
    for(i=3;i>0;i--)
    {
        if(!lm_n192_mod_mul(&heap[i],&heap[2*i],&heap[2*i+1]))
        {
            return 0;
        }
    }
    if(!lm_n192_mod_inverse(&heap[1],&heap[1]))
    {
        return 0;
    }

    for(i=2;i<8;i+=2)
    {
        if(!lm_n192_mod_mul(&tmp0,&heap[i/2],&heap[i+1]))
        {
            return 0;
        }    
        if(!lm_n192_mod_mul(&tmp1,&heap[i/2],&heap[i]))
        {
            return 0;
        }        
        if(!lm_n_copy(&heap[i],&tmp0,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
        if(!lm_n_copy(&heap[i+1],&tmp1,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
    }
    for (i = 4; i < 8; i++)
    {
        lm_sp_p192_point *p = &points[i-4];

        if (!lm_n_is_zero(&p->Z,LM_SP_TYPE_NUM_192))
        {
            if (!lm_n192_mod_sqr(&tmp1,&heap[i]))
            {
                return 0;
            }
            if (!lm_n192_mod_mul(&p->X, &p->X, &tmp1))
            {
                return 0;
            }

            if (!lm_n192_mod_mul(&tmp1, &tmp1, &heap[i]))
            {
                return 0;
            }
            if (!lm_n192_mod_mul(&p->Y, &p->Y, &tmp1))
            {
                return 0;
            }
            lm_sp_n_one(&p->Z,LM_SP_TYPE_NUM_192);
        }
    }

    return 1;
}


int lm_sp_p256_points_jacobian_to_affine(lm_sp_p256_point *points)
{
    uint32_t i;
    lm_sp_num_256 tmp0;
    lm_sp_num_256 tmp1;
    lm_sp_num_256 heap[8];

    memset((uint8_t*)&tmp0, 0, sizeof(lm_sp_num_256));
    memset((uint8_t*)&tmp1, 0, sizeof(lm_sp_num_256));
    memset(heap,0,sizeof(heap));
    
    for(i=0;i<4;i++)    
    {
        lm_n_copy((heap+4+i), &((points+i)->Z),LM_SP_TYPE_NUM_256);
    }
    
    for(i=3;i>0;i--)
    {
        if(!lm_n256_mod_mul(&heap[i],&heap[2*i],&heap[2*i+1]))
        {
            return 0;
        }
    }
    if(!lm_n256_mod_inverse(&heap[1],&heap[1]))
    {
        return 0;
    }

    for(i=2;i<8;i+=2)
    {
        if(!lm_n256_mod_mul(&tmp0,&heap[i/2],&heap[i+1]))
        {
            return 0;
        }    
        if(!lm_n256_mod_mul(&tmp1,&heap[i/2],&heap[i]))
        {
            return 0;
        }        
        if(!lm_n_copy(&heap[i],&tmp0,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
        if(!lm_n_copy(&heap[i+1],&tmp1,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
    }
    for (i = 4; i < 8; i++)
    {
        lm_sp_p256_point *p = &points[i-4];

        if (!lm_n_is_zero(&p->Z,LM_SP_TYPE_NUM_192))
        {
            if (!lm_n256_mod_sqr(&tmp1,&heap[i]))
            {
                return 0;
            }
            if (!lm_n256_mod_mul(&p->X, &p->X, &tmp1))
            {
                return 0;
            }

            if (!lm_n256_mod_mul(&tmp1, &tmp1, &heap[i]))
            {
                return 0;
            }
            if (!lm_n256_mod_mul(&p->Y, &p->Y, &tmp1))
            {
                return 0;
            }
            lm_sp_n_one(&p->Z,LM_SP_TYPE_NUM_256);
        }
    }

    return 1;
}


int lm_sp_pre_compute_points(lm_sp_p192_point *r)
{
    int i;
    lm_sp_p192_point tmp;

    memset((uint8_t*)&tmp, 0, sizeof(tmp));
    
    if(!lm_sp_p192_dbl(&tmp,&r[0]))
    {
        return 0;
    }
    for(i=1;i<4;i++)
    {
        if(!lm_sp_p192_add(&r[i],&r[i-1],&tmp))
        {
            return 0;
        }
    }
    return 1;
}

int lm_sp_pre_compute_points_256(lm_sp_p256_point *r)
{
    int i;
    lm_sp_p256_point tmp;

    memset((uint8_t*)&tmp, 0, sizeof(tmp));

    
    if(!lm_sp_p256_dbl(&tmp,&r[0]))
    {
        return 0;
    }
    for(i=1;i<4;i++)
    {
        if(!lm_sp_p256_add(&r[i],&r[i-1],&tmp))
        {
            return 0;
        }
    }
    return 1;
}


int lm_sp_p192_dbl(lm_sp_p192_point *r, lm_sp_p192_point *a)
{
    lm_sp_num_192 n0;
    lm_sp_num_192 n1;
    lm_sp_num_192 n2;
    lm_sp_num_192 n3;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));

    if (lm_p192_is_point_at_inf(a))
    {
        lm_n_zero(&r->Z,LM_SP_TYPE_NUM_192);
        return 1;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n192_mod_sqr(&n0,&a->X))
        {
            return 0;
        }
        if (!lm_n192_mod_lshift1_quick(&n1, &n0))
        {
            return 0;
        }
        if (!lm_n192_mod_add(&n0,&n0,&n1))
        {
            return 0;
        }
        if (!lm_n192_mod_add(&n1, &n0, (lm_sp_num_192*)&lm_sp_p192_data.A))
        {
            return 0;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n1, &a->Z))
        {
            return 0;
        }
        if (!lm_n192_mod_add(&n0, &a->X, &n1))
        {
            return 0;
        }
        if (!lm_n192_mod_sub(&n2, &a->X, &n1))
        {
            return 0;
        }
        if (!lm_n192_mod_mul(&n1, &n0, &n2))
        {
            return 0;
        }
        if (!lm_n192_mod_lshift1_quick(&n0, &n1))
        {
            return 0;
        }
        if (!lm_n192_mod_add(&n1, &n0, &n1))
        {
            return 0;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n0, &a->Y,LM_SP_TYPE_NUM_192))
        {
            return 0;
        }
    }
    else
    {
        if (!lm_n192_mod_mul(&n0, &a->Y, &a->Z))
        {
            return 0;
        }
    }
    if (!lm_n192_mod_lshift1_quick(&r->Z, &n0))
    {
        return 0;
    }

    if (!lm_n192_mod_sqr(&n3, &a->Y))
    {
        return 0;
    }
    if (!lm_n192_mod_mul(&n2, &a->X, &n3))
    {
        return 0;
    }
    if (!lm_n192_mod_lshift_quick(&n2, &n2, 2))
    {
        return 0;
    }

    if (!lm_n192_mod_lshift1_quick(&n0, &n2))
    {
        return 0;
    }
    if (!lm_n192_mod_sqr(&r->X, &n1))
    {
        return 0;
    }
    if (!lm_n192_mod_sub(&r->X, &r->X, &n0))
    {
        return 0;
    }

    if (!lm_n192_mod_sqr(&n0, &n3))
    {
        return 0;
    }
    if (!lm_n192_mod_lshift_quick(&n3, &n0, 3))
    {
        return 0;
    }

    if (!lm_n192_mod_sub(&n0, &n2, &r->X))
    {
        return 0;
    }
    if (!lm_n192_mod_mul(&n0, &n1, &n0))
    {
        return 0;
    }
    if (!lm_n192_mod_sub(&r->Y, &n0, &n3))
    {
        return 0;
    }
    return 1;

}

int lm_sp_p256_dbl(lm_sp_p256_point *r, lm_sp_p256_point *a)
{
    lm_sp_num_256 n0;
    lm_sp_num_256 n1;
    lm_sp_num_256 n2;
    lm_sp_num_256 n3;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));

    if (lm_p256_is_point_at_inf(a))
    {
        lm_n_zero(&r->Z,LM_SP_TYPE_NUM_256);
        return 1;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_256))
    {
        if (!lm_n256_mod_sqr(&n0,&a->X))
        {
            return 0;
        }
        if (!lm_n256_mod_lshift1_quick(&n1, &n0))
        {
            return 0;
        }
        if (!lm_n256_mod_add(&n0,&n0,&n1))
        {
            return 0;
        }
        if (!lm_n256_mod_add(&n1, &n0, (lm_sp_num_256*)&lm_sp_p256_data.A))
        {
            return 0;
        }
    }
    else
    {
        if (!lm_n256_mod_sqr(&n1, &a->Z))
        {
            return 0;
        }
        if (!lm_n256_mod_add(&n0, &a->X, &n1))
        {
            return 0;
        }
        if (!lm_n256_mod_sub(&n2, &a->X, &n1))
        {
            return 0;
        }
        if (!lm_n256_mod_mul(&n1, &n0, &n2))
        {
            return 0;
        }
        if (!lm_n256_mod_lshift1_quick(&n0, &n1))
        {
            return 0;
        }
        if (!lm_n256_mod_add(&n1, &n0, &n1))
        {
            return 0;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_256))
    {
        if (!lm_n_copy(&n0, &a->Y,LM_SP_TYPE_NUM_256))
        {
            return 0;
        }
    }
    else
    {
        if (!lm_n256_mod_mul(&n0, &a->Y, &a->Z))
        {
            return 0;
        }
    }
    if (!lm_n256_mod_lshift1_quick(&r->Z, &n0))
    {
        return 0;
    }

    if (!lm_n256_mod_sqr(&n3, &a->Y))
    {
        return 0;
    }
    if (!lm_n256_mod_mul(&n2, &a->X, &n3))
    {
        return 0;
    }
    if (!lm_n256_mod_lshift_quick(&n2, &n2, 2))
    {
        return 0;
    }

    if (!lm_n256_mod_lshift1_quick(&n0, &n2))
    {
        return 0;
    }
    if (!lm_n256_mod_sqr(&r->X, &n1))
    {
        return 0;
    }
    if (!lm_n256_mod_sub(&r->X, &r->X, &n0))
    {
        return 0;
    }

    if (!lm_n256_mod_sqr(&n0, &n3))
    {
        return 0;
    }
    if (!lm_n256_mod_lshift_quick(&n3, &n0, 3))
    {
        return 0;
    }

    if (!lm_n256_mod_sub(&n0, &n2, &r->X))
    {
        return 0;
    }
    if (!lm_n256_mod_mul(&n0, &n1, &n0))
    {
        return 0;
    }
    if (!lm_n256_mod_sub(&r->Y, &n0, &n3))
    {
        return 0;
    }
    return 1;

}


int lm_sp_p192_add(lm_sp_p192_point *r, lm_sp_p192_point *a, lm_sp_p192_point *b)
{
    lm_sp_num_192 n0;
    lm_sp_num_192 n1;
    lm_sp_num_192 n2;
    lm_sp_num_192 n3;
    lm_sp_num_192 n4;
    lm_sp_num_192 n5;
    lm_sp_num_192 n6;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));
    memset((uint8_t*)&n5, 0, sizeof(n5));
    memset((uint8_t*)&n6, 0, sizeof(n6));

    if (a == b)
    {
        return lm_sp_p192_dbl(r, a);
    }
    if (lm_p192_is_point_at_inf( a))
    {
        memcpy(r,b,sizeof(lm_sp_p192_point));
        return 1;
    }
    if (lm_p192_is_point_at_inf( b))
    {
        memcpy(r,a,sizeof(lm_sp_p192_point));
        return 1;
    }

    if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n1, &a->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n2, &a->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &b->Z ))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n1, &a->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &b->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n2, &a->Y, &n0))
        {
            goto end;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n3, &b->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n4, &b->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n3, &b->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul( &n4, &b->Y, &n0))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sub(&n5, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n6, &n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_zero(&n5,LM_SP_TYPE_NUM_192))
    {
        if (lm_n_is_zero(&n6,LM_SP_TYPE_NUM_192))
        {
            return lm_sp_p192_dbl( r, a);
            
        }
        else
        {
            lm_n_zero(&r->Z,LM_SP_TYPE_NUM_192);
            return 1;
        }
    }

    if (!lm_n192_mod_add(&n1, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_add(&n2,&n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192) && lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&r->Z, &n5,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &b->Z,LM_SP_TYPE_NUM_192))
            {
                goto end; 
            }
        }
        else if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &a->Z,LM_SP_TYPE_NUM_192))
            {
                goto end;
            }
        }
        else
        { 
            if (!lm_n192_mod_mul( &n0, &a->Z, &b->Z))
            {
                goto end;
            }
        }
        if (!lm_n192_mod_mul( &r->Z, &n0, &n5))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sqr( &n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_sqr( &n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n3, &n1, &n4))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&r->X, &n0, &n3))
    {
        goto end;
    }

    if (!lm_n192_mod_lshift1_quick(&n0, &r->X))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n3, &n0))
    {
        goto end;
    }

    if (!lm_n192_mod_mul( &n0,&n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n5,&n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n1, &n2, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n0, &n1))
    {
        goto end;
    }
    if (n0.Number[0]&0x01)
    {
        lm_sp_num_384 tmp;
        if (!lm_n192_add(&tmp,&n0,(lm_sp_num_192*)&lm_sp_p192_field))
        {
            goto end;
        }
        if (!lm_n192_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_384))
        {
            goto end;
        }
        if(!lm_n_copy(&r->Y,&tmp,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        return 1;
    }

    if (!lm_n192_rshift1(&r->Y, &n0,LM_SP_TYPE_NUM_192))
    {
        goto end;
    }
    return 1;

end:
    return 0;
}

int lm_sp_p256_add(lm_sp_p256_point *r, lm_sp_p256_point *a, lm_sp_p256_point *b)
{
    lm_sp_num_256 n0;
    lm_sp_num_256 n1;
    lm_sp_num_256 n2;
    lm_sp_num_256 n3;
    lm_sp_num_256 n4;
    lm_sp_num_256 n5;
    lm_sp_num_256 n6;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));
    memset((uint8_t*)&n5, 0, sizeof(n5));
    memset((uint8_t*)&n6, 0, sizeof(n6));

    if (a == b)
    {
        return lm_sp_p256_dbl(r, a);
    }
    if (lm_p256_is_point_at_inf( a))
    {
        memcpy(r,b,sizeof(lm_sp_p256_point));
        return 1;
    }
    if (lm_p256_is_point_at_inf( b))
    {
        memcpy(r,a,sizeof(lm_sp_p256_point));
        return 1;
    }

    if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_256))
    {
        if (!lm_n_copy(&n1, &a->X,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
        if (!lm_n_copy(&n2, &a->Y,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n256_mod_sqr(&n0, &b->Z ))
        {
            goto end;
        }
        if (!lm_n256_mod_mul(&n1, &a->X, &n0))
        {
            goto end;
        }

        if (!lm_n256_mod_mul(&n0, &n0, &b->Z))
        {
            goto end;
        }
        if (!lm_n256_mod_mul(&n2, &a->Y, &n0))
        {
            goto end;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_256))
    {
        if (!lm_n_copy(&n3, &b->X,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
        if (!lm_n_copy(&n4, &b->Y,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n256_mod_sqr(&n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n256_mod_mul(&n3, &b->X, &n0))
        {
            goto end;
        }

        if (!lm_n256_mod_mul(&n0, &n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n256_mod_mul( &n4, &b->Y, &n0))
        {
            goto end;
        }
    }

    if (!lm_n256_mod_sub(&n5, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n256_mod_sub(&n6, &n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_zero(&n5,LM_SP_TYPE_NUM_256))
    {
        if (lm_n_is_zero(&n6,LM_SP_TYPE_NUM_256))
        {
            return lm_sp_p256_dbl( r, a);
            
        }
        else
        {
            lm_n_zero(&r->Z,LM_SP_TYPE_NUM_256);
            return 1;
        }
    }

    if (!lm_n256_mod_add(&n1, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n256_mod_add(&n2,&n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_256) && lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_256))
    {
        if (!lm_n_copy(&r->Z, &n5,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
    }
    else
    {
        if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_256))
        { 
            if (!lm_n_copy(&n0, &b->Z,LM_SP_TYPE_NUM_256))
            {
                goto end; 
            }
        }
        else if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_256))
        { 
            if (!lm_n_copy(&n0, &a->Z,LM_SP_TYPE_NUM_256))
            {
                goto end;
            }
        }
        else
        { 
            if (!lm_n256_mod_mul( &n0, &a->Z, &b->Z))
            {
                goto end;
            }
        }
        if (!lm_n256_mod_mul( &r->Z, &n0, &n5))
        {
            goto end;
        }
    }

    if (!lm_n256_mod_sqr( &n0, &n6))
    {
        goto end;
    }
    if (!lm_n256_mod_sqr( &n4, &n5))
    {
        goto end;
    }
    if (!lm_n256_mod_mul( &n3, &n1, &n4))
    {
        goto end;
    }
    if (!lm_n256_mod_sub(&r->X, &n0, &n3))
    {
        goto end;
    }

    if (!lm_n256_mod_lshift1_quick(&n0, &r->X))
    {
        goto end;
    }
    if (!lm_n256_mod_sub(&n0, &n3, &n0))
    {
        goto end;
    }

    if (!lm_n256_mod_mul( &n0,&n0, &n6))
    {
        goto end;
    }
    if (!lm_n256_mod_mul( &n5,&n4, &n5))
    {
        goto end;
    }
    if (!lm_n256_mod_mul( &n1, &n2, &n5))
    {
        goto end;
    }
    if (!lm_n256_mod_sub(&n0, &n0, &n1))
    {
        goto end;
    }
    if (n0.Number[0]&0x01)
    {
        lm_sp_num_512 tmp;
        if (!lm_n256_add(&tmp,&n0,(lm_sp_num_256*)&lm_sp_p256_field))
        {
            goto end;
        }
        if (!lm_n256_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_512))
        {
            goto end;
        }
        if(!lm_n_copy(&r->Y,&tmp,LM_SP_TYPE_NUM_256))
        {
            goto end;
        }
        return 1;
    }

    if (!lm_n256_rshift1(&r->Y, &n0,LM_SP_TYPE_NUM_256))
    {
        goto end;
    }
    return 1;

end:
    return 0;
}

int lm_sp_p192_sub(lm_sp_p192_point *r, lm_sp_p192_point *a, lm_sp_p192_point *b)
{
	lm_sp_p192_point b_neg;
	lm_sp_num_192 tmp;
	lm_n_copy(&b_neg.X,&b->X,LM_SP_TYPE_NUM_192);
	lm_n_copy(&b_neg.Y,&b->Y,LM_SP_TYPE_NUM_192);
	lm_n_copy(&b_neg.Z,&b->Z,LM_SP_TYPE_NUM_192);

	lm_n192_usub(&tmp, (lm_sp_num_192*)&lm_sp_p192_field, &b_neg.Y);
	lm_n_copy(&b_neg.Y,&tmp,LM_SP_TYPE_NUM_192);
	return lm_sp_p192_add(r, a, &b_neg);
	 
}


int lm_sp_p256_sub(lm_sp_p256_point *r, lm_sp_p256_point *a, lm_sp_p256_point *b)
{
	lm_sp_p256_point b_neg;
	lm_sp_num_256 tmp;
	lm_n_copy(&b_neg.X,&b->X,LM_SP_TYPE_NUM_256);
	lm_n_copy(&b_neg.Y,&b->Y,LM_SP_TYPE_NUM_256);
	lm_n_copy(&b_neg.Z,&b->Z,LM_SP_TYPE_NUM_256);

	lm_n256_usub(&tmp, (lm_sp_num_256*)&lm_sp_p256_field, &b_neg.Y);
	lm_n_copy(&b_neg.Y,&tmp,LM_SP_TYPE_NUM_256);
	return lm_sp_p256_add(r, a, &b_neg);
}



int lm_sp_p192_invert(lm_sp_p192_point *p)
{
    if (lm_p192_is_point_at_inf(p) || lm_n_is_zero(&p->Y,LM_SP_TYPE_NUM_192))
    {
        return 1;
    }
    {
        lm_sp_num_192 tmp;
        lm_n_copy(&tmp,&p->Y,LM_SP_TYPE_NUM_192);
        return (lm_n192_usub(&p->Y, (lm_sp_num_192*)&lm_sp_p192_field, &tmp));
    }
}

int lm_sp_p256_invert(lm_sp_p256_point *p)
{
    if (lm_p256_is_point_at_inf(p) || lm_n_is_zero(&p->Y,LM_SP_TYPE_NUM_256))
    {
        return 1;
    }
    {
        lm_sp_num_256 tmp;
        lm_n_copy(&tmp,&p->Y,LM_SP_TYPE_NUM_256);
        return (lm_n256_usub(&p->Y, (lm_sp_num_256*)&lm_sp_p256_field, &tmp));
    }
}







int atml_lm_sp_p192_mul(lm_sp_p192_point *r, lm_sp_p192_point *a, lm_sp_num_192 *k)
{
    /*lm_sp_num_192 n0;
    lm_sp_num_192 n1;
    lm_sp_num_192 n2;
    lm_sp_num_192 n3;
    lm_sp_num_192 n4;
    lm_sp_num_192 n5;
    lm_sp_num_192 n6;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));
    memset((uint8_t*)&n5, 0, sizeof(n5));
    memset((uint8_t*)&n6, 0, sizeof(n6));*/
    int kk;
    int ii;
    
	lm_sp_p192_point *tmp = malloc(sizeof(lm_sp_p192_point));//new lm_sp_p192_point;


	memset((uint8_t*)&(r->X),0,sizeof(r->X));
	r->X.Number[0] = 1;
	memset((uint8_t*)&(r->Y),0,sizeof(r->Y));
	r->Y.Number[0] = 1;
	memset((uint8_t*)&(r->Z),0,sizeof(r->Z));
   
/*    if (lm_p192_is_point_at_inf( a) || lm_p192_is_point_at_inf( b))
    {
        memset(r->Z,0,sizeof(LM_SP_TYPE_NUM_192));
        return 1;
    }
	*/

	for(kk=6-1; kk>=0; kk--)
	{	
		for(ii=31; ii>=0; ii--)
		{
			int di;
			if(!lm_sp_p192_dbl(r, r))
				goto end;

			di = (k->Number[kk]>>ii)&0x00000001;
			if(di == 1)
			{
				if(!lm_sp_p192_add(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_192))
					goto end;
			}
		}
	}








	/*
    if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n1, &a->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n2, &a->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &b->Z ))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n1, &a->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &b->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n2, &a->Y, &n0))
        {
            goto end;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n3, &b->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n4, &b->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n3, &b->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul( &n4, &b->Y, &n0))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sub(&n5, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n6, &n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_zero(&n5,LM_SP_TYPE_NUM_192))
    {
        if (lm_n_is_zero(&n6,LM_SP_TYPE_NUM_192))
        {
            return lm_sp_p192_dbl( r, a);
            
        }
        else
        {
            lm_n_zero(&r->Z,LM_SP_TYPE_NUM_192);
            return 1;
        }
    }

    if (!lm_n192_mod_add(&n1, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_add(&n2,&n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192) && lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&r->Z, &n5,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &b->Z,LM_SP_TYPE_NUM_192))
            {
                goto end; 
            }
        }
        else if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &a->Z,LM_SP_TYPE_NUM_192))
            {
                goto end;
            }
        }
        else
        { 
            if (!lm_n192_mod_mul( &n0, &a->Z, &b->Z))
            {
                goto end;
            }
        }
        if (!lm_n192_mod_mul( &r->Z, &n0, &n5))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sqr( &n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_sqr( &n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n3, &n1, &n4))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&r->X, &n0, &n3))
    {
        goto end;
    }

    if (!lm_n192_mod_lshift1_quick(&n0, &r->X))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n3, &n0))
    {
        goto end;
    }

    if (!lm_n192_mod_mul( &n0,&n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n5,&n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n1, &n2, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n0, &n1))
    {
        goto end;
    }
    if (n0.Number[0]&0x01)
    {
        lm_sp_num_384 tmp;
        if (!lm_n192_add(&tmp,&n0,(lm_sp_num_192*)&lm_sp_p192_field))
        {
            goto end;
        }
        if (!lm_n192_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_384))
        {
            goto end;
        }
        if(!lm_n_copy(&r->Y,&tmp,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        return 1;
    }

    if (!lm_n192_rshift1(&r->Y, &n0,LM_SP_TYPE_NUM_192))
    {
        goto end;
    }*/
    return 1;

end:
    return 0;
}



int atml_lm_sp_p256_mul(lm_sp_p256_point *r, lm_sp_p256_point *a, lm_sp_num_256 *k)
{
    /*lm_sp_num_192 n0;
    lm_sp_num_192 n1;
    lm_sp_num_192 n2;
    lm_sp_num_192 n3;
    lm_sp_num_192 n4;
    lm_sp_num_192 n5;
    lm_sp_num_192 n6;

    memset((uint8_t*)&n0, 0, sizeof(n0));
    memset((uint8_t*)&n1, 0, sizeof(n1));
    memset((uint8_t*)&n2, 0, sizeof(n2));
    memset((uint8_t*)&n3, 0, sizeof(n3));
    memset((uint8_t*)&n5, 0, sizeof(n5));
    memset((uint8_t*)&n6, 0, sizeof(n6));*/
    int kk;
    int ii;
    int di;
	lm_sp_p256_point *tmp = malloc(sizeof(lm_sp_p256_point));//new lm_sp_p256_point;


	memset((uint8_t*)&(r->X),0,sizeof(r->X));
	r->X.Number[0] = 1;
	memset((uint8_t*)&(r->Y),0,sizeof(r->Y));
	r->Y.Number[0] = 1;
	memset((uint8_t*)&(r->Z),0,sizeof(r->Z));
   
/*    if (lm_p192_is_point_at_inf( a) || lm_p192_is_point_at_inf( b))
    {
        memset(r->Z,0,sizeof(LM_SP_TYPE_NUM_192));
        return 1;
    }
	*/

	for(kk=8-1; kk>=0; kk--)
	{

		for(ii=31; ii>=0; ii--)
		{
			if(!lm_sp_p256_dbl(r, r))
				goto end;

			di = (k->Number[kk]>>ii)&0x00000001;
			if(di == 1)
			{
				if(!lm_sp_p256_add(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_256))
					goto end;
			}
		}
	}








	/*
    if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n1, &a->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n2, &a->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &b->Z ))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n1, &a->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &b->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n2, &a->Y, &n0))
        {
            goto end;
        }
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&n3, &b->X,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        if (!lm_n_copy(&n4, &b->Y,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (!lm_n192_mod_sqr(&n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul(&n3, &b->X, &n0))
        {
            goto end;
        }

        if (!lm_n192_mod_mul(&n0, &n0, &a->Z))
        {
            goto end;
        }
        if (!lm_n192_mod_mul( &n4, &b->Y, &n0))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sub(&n5, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n6, &n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_zero(&n5,LM_SP_TYPE_NUM_192))
    {
        if (lm_n_is_zero(&n6,LM_SP_TYPE_NUM_192))
        {
            return lm_sp_p192_dbl( r, a);
            
        }
        else
        {
            lm_n_zero(&r->Z,LM_SP_TYPE_NUM_192);
            return 1;
        }
    }

    if (!lm_n192_mod_add(&n1, &n1, &n3))
    {
        goto end;
    }
    if (!lm_n192_mod_add(&n2,&n2, &n4))
    {
        goto end;
    }

    if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192) && lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
    {
        if (!lm_n_copy(&r->Z, &n5,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
    }
    else
    {
        if (lm_n_is_one(&a->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &b->Z,LM_SP_TYPE_NUM_192))
            {
                goto end; 
            }
        }
        else if (lm_n_is_one(&b->Z,LM_SP_TYPE_NUM_192))
        { 
            if (!lm_n_copy(&n0, &a->Z,LM_SP_TYPE_NUM_192))
            {
                goto end;
            }
        }
        else
        { 
            if (!lm_n192_mod_mul( &n0, &a->Z, &b->Z))
            {
                goto end;
            }
        }
        if (!lm_n192_mod_mul( &r->Z, &n0, &n5))
        {
            goto end;
        }
    }

    if (!lm_n192_mod_sqr( &n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_sqr( &n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n3, &n1, &n4))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&r->X, &n0, &n3))
    {
        goto end;
    }

    if (!lm_n192_mod_lshift1_quick(&n0, &r->X))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n3, &n0))
    {
        goto end;
    }

    if (!lm_n192_mod_mul( &n0,&n0, &n6))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n5,&n4, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_mul( &n1, &n2, &n5))
    {
        goto end;
    }
    if (!lm_n192_mod_sub(&n0, &n0, &n1))
    {
        goto end;
    }
    if (n0.Number[0]&0x01)
    {
        lm_sp_num_384 tmp;
        if (!lm_n192_add(&tmp,&n0,(lm_sp_num_192*)&lm_sp_p192_field))
        {
            goto end;
        }
        if (!lm_n192_rshift1(&tmp, &tmp,LM_SP_TYPE_NUM_384))
        {
            goto end;
        }
        if(!lm_n_copy(&r->Y,&tmp,LM_SP_TYPE_NUM_192))
        {
            goto end;
        }
        return 1;
    }

    if (!lm_n192_rshift1(&r->Y, &n0,LM_SP_TYPE_NUM_192))
    {
        goto end;
    }*/
    return 1;

end:
    return 0;
}


int atml_lm_sp_num192_NAF(lm_sp_num_192 *k_naf_0, lm_sp_num_192 *k_naf_1, lm_sp_num_192 *k)
{
	int bit_cnt  = 0;
	int word_cnt = 0;
	lm_sp_num_192 const_one;
	lm_sp_n_one (&const_one,LM_SP_TYPE_NUM_192);

	lm_n_zero(k_naf_0,LM_SP_TYPE_NUM_192);
	lm_n_zero(k_naf_1,LM_SP_TYPE_NUM_192);

	while(!lm_n_is_zero(k,LM_SP_TYPE_NUM_192))
	{
		if(k->Number[0]&0x1)
		{
			uint8_t ki = (uint8_t)(k->Number[0]&0x3);
			//printf("ki = %d \n",ki);
			if(ki == 1)
			{
				if (!lm_n192_mod_sub(k, k, &const_one)) return 0;
				k_naf_0->Number[word_cnt] += 1<<bit_cnt;
			}
			else if (ki == 3) // 2 - kmod4 = -1
			{
				if (!lm_n192_mod_add(k, k, &const_one)) return 0;
				k_naf_1->Number[word_cnt] += 1<<bit_cnt;
			}

		}
		
		lm_n192_rshift1(k, k,LM_SP_TYPE_NUM_192);
		bit_cnt++;
		if(bit_cnt == 32)
			word_cnt++;
		bit_cnt %=32;
	}
	return 1;
}


int atml_lm_sp_num256_NAF(lm_sp_num_256 *k_naf_0, lm_sp_num_256 *k_naf_1, lm_sp_num_256 *k)
{
	int bit_cnt  = 0;
	int word_cnt = 0;
	lm_sp_num_256 const_one;
	lm_sp_n_one (&const_one,LM_SP_TYPE_NUM_256); // memset

	lm_n_zero(k_naf_0,LM_SP_TYPE_NUM_256);
	lm_n_zero(k_naf_1,LM_SP_TYPE_NUM_256);

	while(!lm_n_is_zero(k,LM_SP_TYPE_NUM_256))
	{
		if(k->Number[0]&0x1)
		{
			uint8_t ki = (uint8_t)(k->Number[0]&0x3);
			if(ki == 1)
			{
				if (!lm_n256_mod_sub(k, k, &const_one)) return 0;
				k_naf_0->Number[word_cnt] += 1<<bit_cnt;
			}
			else if (ki == 3) // 2 - kmod4 = -1
			{
				if (!lm_n256_mod_add(k, k, &const_one)) return 0;
				k_naf_1->Number[word_cnt] += 1<<bit_cnt;
			}

		}
		
		lm_n256_rshift1(k, k,LM_SP_TYPE_NUM_256);
		bit_cnt++;
		if(bit_cnt == 32)
			word_cnt++;
		bit_cnt %=32;
	}
	return 1;
}


int atml_lm_sp_p192_NAF_mul(lm_sp_p192_point *r, lm_sp_p192_point *a, lm_sp_num_192 *k_naf_0, lm_sp_num_192 *k_naf_1)
{
	int kk;
        int ii;
        int di;
        //lm_sp_num_192 ks;
		
	lm_sp_p192_point* tmp = malloc(sizeof(lm_sp_p192_point));//new lm_sp_p192_point;

	memset((uint8_t*)&(r->X),0,sizeof(r->X));
	r->X.Number[0] = 1;
	memset((uint8_t*)&(r->Y),0,sizeof(r->Y));
	r->Y.Number[0] = 1;
	memset((uint8_t*)&(r->Z),0,sizeof(r->Z));

	
	//====================
	// NAF multiplication
	//====================
   
	for(kk=6-1; kk>=0; kk--)
	{

		for(ii=31; ii>=0; ii--)
		{
			if(!lm_sp_p192_dbl(r, r))
				goto end;

			di = (int)((k_naf_0->Number[kk]>>ii)&0x00000001) - (int)((k_naf_1->Number[kk]>>ii)&0x00000001);
			if(di == 1)
			{
				if(!lm_sp_p192_add(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_192))
					goto end;
			}
			else if(di == -1)
			{
				if(!lm_sp_p192_sub(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_192))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_192))
					goto end;
			}
		}
	}
    return 1;

end:
    return 0;
} 



int atml_lm_sp_p256_NAF_mul(lm_sp_p256_point *r, lm_sp_p256_point *a, lm_sp_num_256 *k_naf_0, lm_sp_num_256 *k_naf_1)
{
	int kk;
	int ii;
	int di;
	//lm_sp_num_256 ks;
		
	lm_sp_p256_point *tmp = malloc(sizeof(lm_sp_p256_point));//new lm_sp_p256_point;

	memset((uint8_t*)&(r->X),0,sizeof(r->X));
	r->X.Number[0] = 1;
	memset((uint8_t*)&(r->Y),0,sizeof(r->Y));
	r->Y.Number[0] = 1;
	memset((uint8_t*)&(r->Z),0,sizeof(r->Z));

	
	//====================
	// NAF multiplication
	//====================
   
	for(kk=8-1; kk>=0; kk--)
	{

		for(ii=31; ii>=0; ii--)
		{
			if(!lm_sp_p256_dbl(r, r))
				goto end;

			di = (int)((k_naf_0->Number[kk]>>ii)&0x00000001) - (int)((k_naf_1->Number[kk]>>ii)&0x00000001);
			if(di == 1)
			{
				if(!lm_sp_p256_add(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_256))
					goto end;
			}
			else if(di == -1)
			{
				if(!lm_sp_p256_sub(tmp, r, a))
					goto end;

				if(!lm_n_copy(&r->X,&tmp->X,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Y,&tmp->Y,LM_SP_TYPE_NUM_256))
					goto end;
				if(!lm_n_copy(&r->Z,&tmp->Z,LM_SP_TYPE_NUM_256))
					goto end;
			}
		}
	}
    return 1;

end:
    return 0;
} 



int atml_lm_sp_p192_montgomery_ladder_mul(lm_sp_p192_point *R, lm_sp_p192_point *P, lm_sp_num_192 *k)
{
	int ii;
	int jj;
	//int di;
	//lm_sp_p256_point A;
	lm_sp_p192_point* B = malloc(sizeof(lm_sp_p192_point));//new lm_sp_p192_point;

	

	lm_sp_n_one(&R->X,LM_SP_TYPE_NUM_192);
	lm_sp_n_one(&R->Y,LM_SP_TYPE_NUM_192);
	lm_n_zero(&R->Z,LM_SP_TYPE_NUM_192);

	lm_n_copy(&B->X, &P->X,LM_SP_TYPE_NUM_192);
	lm_n_copy(&B->Y, &P->Y,LM_SP_TYPE_NUM_192);
	lm_n_copy(&B->Z, &P->Z,LM_SP_TYPE_NUM_192);

	for(ii=6-1; ii>=0; ii--)
	{
		for(jj=31; jj>= 0 ; jj--)
		{	
		    if( (int)((k->Number[ii] >>jj)&0x01) == 0)
			{	
				lm_sp_p192_add(B, R, B);
				lm_sp_p192_dbl(R, R);
			}
			else
			{	
				lm_sp_p192_add(R, R, B);
				lm_sp_p192_dbl(B, B);
			}
		}
	}


	
	return 1;

}

int atml_lm_sp_p256_montgomery_ladder_mul(lm_sp_p256_point *R, lm_sp_p256_point *P, lm_sp_num_256 *k)
{
	int ii;
	int jj;
	//lm_sp_p256_point A;
	lm_sp_p256_point* B = malloc(sizeof(lm_sp_p256_point));//new lm_sp_p256_point;

	

	lm_sp_n_one(&R->X,LM_SP_TYPE_NUM_256);
	lm_sp_n_one(&R->Y,LM_SP_TYPE_NUM_256);
	lm_n_zero(&R->Z,LM_SP_TYPE_NUM_256);

	lm_n_copy(&B->X, &P->X,LM_SP_TYPE_NUM_256);
	lm_n_copy(&B->Y, &P->Y,LM_SP_TYPE_NUM_256);
	lm_n_copy(&B->Z, &P->Z,LM_SP_TYPE_NUM_256);

	for(ii=8-1; ii>=0; ii--)
	{
		for(jj=31; jj>= 0 ; jj--)
		{	
		    if( (int)((k->Number[ii] >>jj)&0x01) == 0)
			{	
				lm_sp_p256_add(B, R, B);
				lm_sp_p256_dbl(R, R);
			}
			else
			{	
				lm_sp_p256_add(R, R, B);
				lm_sp_p256_dbl(B, B);
			}
		}
	}


	
	return 1;

}

/*
int atml_lm_sp_p256_montgomery_ladder_sch_mul(lm_sp_p256_point *r, lm_sp_p256_point *a, lm_sp_num_256 *k) //scheduled version
{
	lm_sp_num_256* Xa = new lm_sp_num_256;
	lm_sp_num_256* Za = new lm_sp_num_256;
	
	lm_sp_num_256* Xb = new lm_sp_num_256;
	lm_sp_num_256* Zb =  new lm_sp_num_256;
	
	lm_sp_num_256* T1 = new lm_sp_num_256;
	lm_sp_num_256* T2 = new lm_sp_num_256;

	lm_sp_num_256* tmp0 = new lm_sp_num_256;
	lm_sp_num_256* tmp1 = new lm_sp_num_256;

	lm_sp_n_one(Xa,LM_SP_TYPE_NUM_256);
	lm_n_zero(Za,LM_SP_TYPE_NUM_256);

	lm_n_copy(Xb, &a->X,LM_SP_TYPE_NUM_256);
	lm_sp_n_one(Zb,LM_SP_TYPE_NUM_256);

	for(int ii=8-1; ii>=0; ii--)
	{
		for(int jj=31; jj>= 0 ; jj--)
		{
			
			if(!lm_n256_mod_mul(T1, Xa, Zb))
				return 0;
			
			if(!lm_n256_mod_mul(T2, Xb, Za))
				return 0;
			
			//printf("N[%d][%d] =  %d \n" , ii , jj ,  (k->Number[ii] >>jj)&0x01);

			if( int((k->Number[ii] >>jj)&0x01) == 0)
			{	
				//printf("*****N[%d][%d] =  %d \n" , ii , jj ,  (k->Number[ii] >>jj)&0x01);
				if(!lm_n256_mod_add(Zb,T1,T2))
					return 0;

				if(!lm_n256_mod_sqr(Zb, Zb))
					return 0;

				if(!lm_n256_mod_mul(Xb, &a->X, Zb))
					return 0;

				if(!lm_n256_mod_mul(tmp0, T1, T2))
					return 0;

				if(!lm_n256_mod_add(Xb, Xb , tmp0))
					return 0;

				if(!lm_n256_mod_mul(T1, Xa, Za))
					return 0;

				if(!lm_n256_mod_add(tmp0, Xa, Za))
					return 0;

				if(!lm_n256_mod_sqr(Xa, tmp0))
					return 0;

				if(!lm_n256_mod_sqr(Xa, Xa))
					return 0;

				if(!lm_n256_mod_sqr(Za, T1))
					return 0;

			}
			else
			{
				if(!lm_n256_mod_add(Za, T1, T2))
					return 0;

				if(!lm_n256_mod_sqr(Za, Za))
					return 0;

				if(!lm_n256_mod_mul(Xa, &a->X, Za))
					return 0;

				if(!lm_n256_mod_mul(tmp0, T1, T2))
					return 0;

				if(!lm_n256_mod_add(Xa, Xa, tmp0))
					return 0;

				if(!lm_n256_mod_mul(T1, Xb, Zb))
					return 0;

				if(!lm_n256_mod_add(tmp0, Xb, Zb))
					return 0;

				if(!lm_n256_mod_sqr(Xb, tmp0))
					return 0;

				if(!lm_n256_mod_sqr(Xb, Xb))
					return 0;

				if(!lm_n256_mod_sqr(Zb, T1))
					return 0;

			}
		}
	}


	if(lm_n_is_zero(Zb,LM_SP_TYPE_NUM_256))
	{
		lm_n_copy(Xa,&a->X,LM_SP_TYPE_NUM_256);
		if(!lm_n256_mod_add(Za, &a->X, &a->Y))
					return 0;
	}
	else
	{
		if(!lm_n256_mod_inverse(tmp0,Za))
			return 0;
		
		if(!lm_n256_mod_mul(Xa,Xa,tmp0))
			return 0;

		if(!lm_n256_mod_inverse(tmp0,Zb))
			return 0;
		
		if(!lm_n256_mod_mul(Xb,Xb,tmp0))
			return 0;

		if(!lm_n256_mod_add(tmp0, Xa, &a->X))
			return 0;

		if(!lm_n256_mod_add(tmp1, Xb, &a->X))
			return 0;

		if(!lm_n256_mod_mul(tmp0,tmp0,tmp1))
			return 0;

		if(!lm_n256_mod_sqr(tmp1,&a->X))
			return 0;

		if(!lm_n256_mod_add(tmp0, tmp0, tmp1))
			return 0;

		if(!lm_n256_mod_add(Za, tmp0, &a->Y))
			return 0;

		if(!lm_n256_mod_add(tmp0, Xa, &a->X))
			return 0;

		if(!lm_n256_mod_mul(Za,Za,tmp0))
			return 0;

		if(!lm_n256_mod_inverse(tmp0,&a->X))
			return 0;

		if(!lm_n256_mod_mul(Za,Za,tmp0))
			return 0;

		if(!lm_n256_mod_add(Za, Za, &a->Y))
			return 0;
	}

	if(!lm_n_copy(&r->X,Xa,LM_SP_TYPE_NUM_256))
		return 0;

	if(!lm_n_copy(&r->Y,Za,LM_SP_TYPE_NUM_256))
		return 0;

	lm_sp_n_one (&r->Z,LM_SP_TYPE_NUM_256);

	return 1;

} 
*/


///@} LMSPCOMPUTATION

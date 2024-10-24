/*
MIT License

Copyright (c) 2020 Dani Huertas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/*
 * Data Encryption Standard
 * An approach to DES algorithm
 *
 * By: Daniel Huertas Gonzalez
 * Email: huertas.dani@gmail.com
 * Version: 0.1
 *
 * Based on the document FIPS PUB 46-3
 */
#include"minimacy.h"
#include"crypto_des.h"

#define swap64(x) (((x&0xff)<<56)|((x&0xff00)<<40)|((x&0xff0000)<<24)|((x&0xff000000)<<8)|((x>>8)&0xff000000)|((x>>24)&0xff0000)|((x>>40)&0xff00)|((x>>56)&0xff))
#define LB32   0x00000001
#define LB64   0x0000000000000001
#define L64_MASK    0x00000000ffffffff
#define H64_MASK    0xffffffff00000000

static const char IP[] = {
62, 54, 46, 38, 30, 22, 14,  6,
60, 52, 44, 36, 28, 20, 12,  4,
58, 50, 42, 34, 26, 18, 10,  2,
56, 48, 40, 32, 24, 16,  8,  0,
63, 55, 47, 39, 31, 23, 15,  7,
61, 53, 45, 37, 29, 21, 13,  5,
59, 51, 43, 35, 27, 19, 11,  3,
57, 49, 41, 33, 25, 17,  9,  1
};

static const char PI[] = {
31, 63, 23, 55, 15, 47, 7, 39, 30, 62, 22, 54, 14, 46, 6, 38,
29, 61, 21, 53, 13, 45, 5, 37, 28, 60, 20, 52, 12, 44, 4, 36,
27, 59, 19, 51, 11, 43, 3, 35, 26, 58, 18, 50, 10, 42, 2, 34,
25, 57, 17, 49, 9, 41, 1, 33, 24, 56, 16, 48, 8, 40, 0, 32,
};

static const char E[] = {
    32,  1,  2,  3,  4,  5,
     4,  5,  6,  7,  8,  9,
     8,  9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32,  1
};

static const char P[] = {
    16,  7, 20, 21,
    29, 12, 28, 17,
     1, 15, 23, 26,
     5, 18, 31, 10,
     2,  8, 24, 14,
    32, 27,  3,  9,
    19, 13, 30,  6,
    22, 11,  4, 25
};

static const char S[8][64] = { {
        /* S1 */
        14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
         0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
         4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
        15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
    },{
        /* S2 */
        15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
         3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
         0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
        13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
    },{
        /* S3 */
        10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
        13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
        13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
         1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
    },{
        /* S4 */
         7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
        13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
        10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
         3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
    },{
        /* S5 */
         2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
        14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
         4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
        11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
    },{
        /* S6 */
        12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
        10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
         9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
         4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
    },{
        /* S7 */
         4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
        13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
         1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
         6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
    },{
        /* S8 */
        13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
         1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
         7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
         2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
    } };

static const char PC1[] = {
63, 55, 47, 39, 31, 23, 15,  7,
62, 54, 46, 38, 30, 22, 14,  6,
61, 53, 45, 37, 29, 21, 13,  5,
60, 52, 44, 36, 57, 49, 41, 33,
25, 17,  9,  1, 58, 50, 42, 34,
26, 18, 10,  2, 59, 51, 43, 35,
27, 19, 11,  3, 28, 20, 12,  4
};

static const char PC2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

static const char iteration_shift[] = {
       1,  1,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  1
};

void desInit(char* key, uint64_t* sub_key)
{
    int i, j;
    /* 28 bits */
    uint32_t C = 0;
    uint32_t D = 0;

    /* 56 bit */
    uint64_t permuted_choice_1 = 0;
    uint64_t permuted_choice_2 = 0;

    for (i = 0; i < 56; i++) {
        char d = PC1[i];
        permuted_choice_1 <<= 1;
        permuted_choice_1 |= (key[d>>3] >> (d&7)) & LB64;
    }

    C = (uint32_t)((permuted_choice_1 >> 28) & 0x000000000fffffff);
    D = (uint32_t)(permuted_choice_1 & 0x000000000fffffff);
    for (i = 0; i < 16; i++) {
        // shift Ci and Di
        for (j = 0; j < iteration_shift[i]; j++) {

            C = (0x0fffffff & (C << 1)) | (0x00000001 & (C >> 27));
            D = (0x0fffffff & (D << 1)) | (0x00000001 & (D >> 27));

        }
        permuted_choice_2 = 0;
        permuted_choice_2 = (((uint64_t)C) << 28) | (uint64_t)D;

        sub_key[i] = 0;

        for (j = 0; j < 48; j++) {

            sub_key[i] <<= 1;//32+32
            sub_key[i] |= (permuted_choice_2 >> (56 - PC2[j])) & LB64;

        }
    }
}

uint64_t des(char* input,uint64_t* sub_key, int enc) {


    int i, j;

    /* 8 bit */
    char satir, sutun;


    /* 32 bit */
    uint32_t L = 0;
    uint32_t R = 0;
    uint32_t s_output = 0;
    uint32_t f_function_res = 0;
    uint32_t temp = 0;

    /* 48 bit */
    uint64_t s_input = 0;


    /* 64 bit */
    uint64_t init_perm_res = 0;
    uint64_t inv_init_perm_res = 0;
    uint64_t pre_output = 0;
//    PRINTF(LOG_DEV,"%llx %llx %d\n", input, key, enc);
    /* initial permutation */

    for (i = 0; i < 64; i++) {
        char d = IP[i];
        init_perm_res <<= 1;
        init_perm_res |= (input[d>>3] >> (d&7)) & LB64;
    }

    L = (uint32_t)(init_perm_res >> 32) & L64_MASK;

    R = (uint32_t)init_perm_res & L64_MASK;

    for (i = 0; i < 16; i++) {


        s_input = 0;

        for (j = 0; j < 48; j++) {

            s_input <<= 1;
            s_input |= (uint64_t)((R >> (32 - E[j])) & LB32);

        }
        if (enc == 0) {
            //decryption
            s_input = s_input ^ sub_key[15 - i];

        }
        else {
            // encryption
            s_input = s_input ^ sub_key[i]; 

        }


        for (j = 0; j < 8; j++) {


            satir = (char)((s_input & (((uint64_t)0x0000840000000000) >> (6 * j))) >> (42 - 6 * j));
            satir = ((satir >> 4)) | (satir & 0x01);

            sutun = (char)((s_input & (((uint64_t)0x0000780000000000) >> (6 * j))) >> (43 - 6 * j));

            s_output <<= 4;
            s_output |= (uint32_t)(S[j][16 * satir + sutun] & 0x0f);

        }

        f_function_res = 0;

        for (j = 0; j < 32; j++) {

            f_function_res <<= 1;
            f_function_res |= (s_output >> (32 - P[j])) & LB32;

        }

        temp = R;
        R = L ^ f_function_res;
        L = temp;
    }

    pre_output = (((uint64_t)R) << 32) | (uint64_t)L;

    for (i = 0; i < 64; i++) {

        inv_init_perm_res <<= 1;
        inv_init_perm_res |= (pre_output >> PI[i]) & LB64;

    }
    return inv_init_perm_res;
}


void DESCreate(DesCtx* ctx, char* key)
{
    desInit(key, ctx->sub_key);
}
void DESProcess(DesCtx* ctx, char* data, int enc)
{
    ctx->result = des(data, ctx->sub_key, enc);
}
void DESOutput(DesCtx* ctx, char* output)
{
    *(uint64_t*)output = ctx->result;
}

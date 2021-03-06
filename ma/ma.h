

/**
 * \file
 *         Implmentation of a lightweight puiblic key encryption scheme
 * developed in CSIRO
 * \author
 *         Dongxi Liu <dongxi.liu@csiro.au>
 */

#include "mini-gmp.h"

#define DTLS_MA_M 74u
#define DTLS_MA_N 13u
#define DTLS_MA_T_BITS 16u
#define DTLS_MA_Q_BITS 32u
#define DTLS_MA_G 0u
#define DTLS_MA_W 86u  // W must be bigger than B (number of positive BS)
#define DTLS_MA_W_MIN 60u
#define DTLS_MA_B 16u  // the maximum of common public key element
//#define P_BITS 16u
#define DTLS_MA_E_BITS 8u
#define DTLS_MA_E_SIGN '+'
#define DTLS_MA_SK_BITS 8u
#define DTLS_MA_COMMON_LEN 256u

struct dtls_ma_cipher {
    mpz_t element[DTLS_MA_N];
    mpz_t tail;
};

struct dtls_ma_private_key {
    mpz_t n;
    mpz_t w;
    mpz_t wMin;
    mpz_t m;
    mpz_t p;
    mpz_t q;
    mpz_t r;
    mpz_t t;
    mpz_t k[DTLS_MA_N];
    mpz_t sk;
    mpz_t skInvQ;
    mpz_t skInvP;
};

struct dtls_ma_public_key {
    mpz_t q;
    mpz_t Element[DTLS_MA_M];
    mpz_t t;
    mpz_t bias;
    mpz_t w;
    mpz_t wMin;
    mpz_t n;
};

// BN+12
static const char dtls_ma_common[DTLS_MA_COMMON_LEN] = {
    4,  2,  13, 7,  9,  7,  7,  12, 6,  8,  10, 11, 15, 4,  8,  12, 10, 3,  10,
    10, 9,  6,  12, 4,  8,  9,  11, 9,  5,  11, 3,  15, 5,  11, 7,  13, 3,  8,
    11, 5,  3,  5,  11, 3,  4,  14, 12, 4,  2,  11, 13, 7,  12, 15, 9,  11, 5,
    12, 3,  9,  4,  3,  10, 8,  2,  13, 4,  15, 7,  11, 5,  6,  11, 10, 2,  3,
    4,  3,  4,  15, 14, 2,  9,  2,  9,  2,  9,  8,  10, 5,  8,  4,  11, 14, 7,
    11, 2,  6,  3,  13, 4,  13, 4,  7,  2,  12, 4,  6,  3,  12, 2,  4,  12, 9,
    10, 3,  5,  12, 7,  7,  3,  3,  4,  14, 2,  8,  5,  4,  4,  10, 9,  3,  10,
    9,  14, 14, 5,  8,  3,  4,  7,  14, 12, 5,  15, 5,  2,  3,  10, 2,  5,  4,
    5,  4,  2,  8,  8,  5,  10, 2,  9,  14, 5,  14, 9,  12, 5,  12, 4,  12, 11,
    12, 6,  3,  2,  3,  9,  10, 15, 6,  12, 15, 13, 4,  13, 15, 4,  3,  4,  3,
    8,  6,  12, 9,  8,  10, 13, 9,  5,  15, 10, 10, 9,  6,  10, 10, 2,  8,  9,
    2,  10, 8,  3,  14, 3,  10, 7,  9,  2,  9,  9,  6,  4,  4,  13, 3,  6,  15,
    10, 5,  3,  14, 2,  3,  14, 12, 6,  11, 15, 11, 10, 7,  6,  14, 15, 11, 13,
    3,  7,  13, 12, 14, 13, 12, 10, 11,
};

static const unsigned char dtls_ma_common_sign[] = {
    0x83, 0x17, 0xe3, 0x60, 0x5c, 0xc3, 0x8a, 0xda, 0xcb, 0x8c, 0x98, 0x25,
    0xdf, 0x77, 0x8d, 0xda, 0x2a, 0x91, 0x99, 0x92, 0x77, 0x1b, 0x00, 0x70,
    0xf7, 0xcf, 0x2a, 0xa9, 0xe3, 0xd7, 0x2d, 0xc7, 0x7f, 0x5b, 0x3e, 0x25,
    0x42, 0x3a, 0xfe, 0x5a, 0x2b, 0x74, 0xdc, 0x65, 0x3c, 0xa0, 0x85, 0xc1,
    0x49, 0xab, 0xb7, 0x80, 0xf4, 0x8a, 0x20, 0x89, 0xd0, 0xed, 0x39, 0xcc,
    0x9c, 0x31, 0x16, 0xd4, 0xd4, 0xa5, 0x6b, 0xf9, 0x29, 0xa4, 0x45, 0x2c,
    0x68, 0xd1, 0x3a, 0x38, 0x50, 0x7f, 0x35, 0x42, 0x96, 0x15, 0xbf, 0x92,
    0xc3, 0x4d, 0xc1, 0xcf, 0x30, 0x7c, 0x24, 0xbd, 0x98, 0x7d, 0x2e, 0x8b,
    0xb1, 0x0b, 0xf6, 0x75, 0x76, 0xcc, 0x47, 0x21, 0x79, 0x5c, 0x7f, 0x5f,
    0x36, 0x73, 0x1a, 0x7a, 0xeb, 0x55, 0x3d, 0x60, 0x5f, 0xfa, 0x4e, 0x21,
    0x80, 0xc4, 0xbc, 0xab, 0xfb, 0x32, 0x08, 0x39, 0xdb, 0x69, 0x43, 0x64,
    0xfc, 0x46, 0x2a, 0xdd, 0x1f, 0x21, 0x41, 0xdd, 0x39, 0x4f, 0x67, 0x28,
    0x7b, 0x7c, 0x5b, 0x87, 0x1e, 0x31, 0x11, 0x58, 0x5a, 0x19, 0x77, 0x47,
    0xee, 0x13, 0xb7, 0x55, 0x7a, 0x7e, 0x10, 0x26, 0xa3, 0xb7, 0x9b, 0xc1,
    0x9e, 0x17, 0xc4, 0x03, 0xc1, 0xeb, 0xa6, 0x2d, 0x5f, 0x49, 0x73, 0x01,
    0xae, 0x61, 0x39, 0x9a, 0xf0, 0xa1, 0x8d, 0x53, 0x92, 0xa2, 0x70, 0x49,
    0x77, 0x8f, 0xd5, 0xe4, 0x0e, 0x07, 0xaf, 0x97, 0xe8, 0xdf, 0x0a, 0x2d,
    0xb5, 0x29, 0x36, 0x8a, 0x72, 0x67, 0xcf, 0xbe, 0x02, 0x66, 0x52, 0x23,
    0x05, 0xd3, 0xe7, 0x83, 0x02, 0x2b, 0x9e, 0x58, 0x51, 0x78, 0x05, 0x81,
    0xce, 0x0d, 0x45, 0xb8, 0x93, 0xbf, 0xd7, 0xca, 0x1f, 0x1b, 0x24, 0x5f,
    0xe5, 0x14, 0x12, 0xde, 0x4b, 0x6b, 0x9d, 0x11, 0x69, 0x67, 0x63, 0x4e,
    0x53, 0x67, 0x09, 0xdc,
};

void dtls_ma_generate_keys(struct dtls_ma_private_key* privk,
                           struct dtls_ma_public_key* pubk);

void dtls_ma_generate_private_key(struct dtls_ma_private_key* privk);

void dtls_ma_print_private_key(struct dtls_ma_private_key* privk);

void dtls_ma_save_private_key(const char* fname,
                              struct dtls_ma_private_key* privk);

int dtls_ma_load_private_key(const char* fname,
                             struct dtls_ma_private_key* privk);

void dtls_ma_get_public_key(struct dtls_ma_private_key* privk,
                            struct dtls_ma_public_key* pubk);

void dtls_ma_save_public_key(const char* fname,
                             struct dtls_ma_public_key* pubk);
struct dtls_ma_public_key* dtls_ma_load_public_key(const char* fname);

void dtls_ma_print_public_key(char* msg, struct dtls_ma_public_key* pubk);

// struct dtls_ma_cipher* enc_str(struct dtls_ma_public_key* pubk, unsigned
// char* buf, unsigned long len);
unsigned char* dtls_ma_enc_str_G(struct dtls_ma_public_key* pubk,
                                 unsigned char* buf, int* len);
void dtls_ma_enc(struct dtls_ma_public_key* pubk, mpz_t v,
                 struct dtls_ma_cipher*);

int dtls_ma_cipher_out_str(struct dtls_ma_cipher* D, char* buf, int buf_len);
struct dtls_ma_cipher* dtls_ma_cipher_in_str(char* buf, int* pos);

void dtls_ma_print_cipher_string(struct dtls_ma_cipher* D);

void dtls_ma_dec(struct dtls_ma_private_key* privk, struct dtls_ma_cipher*,
                 mpz_t v);
unsigned char* dtls_ma_dec_str_G(struct dtls_ma_private_key* privk,
                                 unsigned char* buf, int* buf_len);

void dtls_ma_print_SIS_sample(char* msg, struct dtls_ma_public_key* pubk,
                              struct dtls_ma_cipher* D);

/* from encode.h */
int dtls_ma_decode(unsigned char* buf, unsigned long buf_len, mpz_t res);
int dtls_ma_encode(unsigned char* buf, unsigned long buf_len, mpz_t res);

int dtls_ma_encodeG(unsigned char* buf, unsigned long buf_len, mpz_t* res);
int dtls_ma_decodeG(unsigned char* buf, unsigned long buf_len, mpz_t* res,
                    int debug);

unsigned char* dtls_ma_random_bytes(int buflen);
unsigned char* dtls_ma_random_hex(int len);

int dtls_ma_check(unsigned char* tagged_buf, int tagged_buf_len);
void dtls_ma_debug(char* msg);
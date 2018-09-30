#ifndef KEYFORMAT
#define KEYFORMAT


#define MAX_MODULUS_LEN (2048/8)

#define KEY_MARKER 0xAABBCCDD

#define PROD_KEY     0
#define DEV_KEY      1
#define GOLD_KEY     2
#include"IFWD_std_types.h"
typedef struct{
        U32 KeyLength;/* length of modulus */
        U8 Exponent[MAX_MODULUS_LEN];
        U8 Modulus[MAX_MODULUS_LEN];
}BasicKeyStructType;

typedef struct {
        U32 PublicSignature[5]; /* SHA-1 of pub key struct*/
        U32 KeyType;
        U32 KeyMarker;/* magic number for quick detection of possible key presence*/
        BasicKeyStructType Key;
}PublicKeyStructType;

typedef struct {
        U32 PrivateSignature[5];/* SHA-1 of priv key struct*/
        U32 KeyType;
        U32 KeyMarker;/*magic number for quick detection of possible key presence*/
        BasicKeyStructType Key;
        U32 PublicSignature[5]; /* SHA-1 of pub key struct*/
}PrivateKeyStructType;

//New Public Key structure:
typedef struct{
   U32 KeyType;
   U32 KeyLength;
   U32 Exponent;
   U8 Modulus[256];
   U8 Montgom[256];
}NewPublicKeyStructType;


#endif

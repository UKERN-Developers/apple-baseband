/*
 * An implementation of the SHA-256 hash function, this is endian neutral
 * so should work just about anywhere.
 *
 * This code works much like the MD5 code provided by RSA.  You sha_init()
 * a "sha_state" then sha_process() the bytes you want and sha_done() to get
 * the output.
 *
 * Revised Code:  Complies to SHA-256 standard now.
 *
 * Tom St Denis -- http://tomstdenis.home.dhs.org
 * */
#include <stdio.h>

typedef struct {
    unsigned long state[8], length, curlen;
    unsigned char buf[64];
}
sha_state;



/* init the SHA state */
void sha_init(sha_state * md);

void sha_process(sha_state * md, const unsigned char *buf, int len);


void sha_done(sha_state * md, unsigned *hash);


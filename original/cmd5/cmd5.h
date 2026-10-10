#ifndef _MD5_H
#define _MD5_H

#include <stdio.h>
#include <time.h>
#include <string.h>

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;
/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;
/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* MD5 context. */
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

typedef struct rc4_key {      
     unsigned char state[256];       
     unsigned char x;        
     unsigned char y;
} rc4_key;

class CMD5 {
private:
	void MD5Init(MD5_CTX *context);
	void MD5Update(MD5_CTX * context, unsigned char * input, unsigned int inputLen);
	void MD5Final (unsigned char digest[16], MD5_CTX * context);
	void MD5Transform (UINT4 state[4], unsigned char block[64]);
	void Encode (unsigned char * output, UINT4 * input, unsigned int len);
	void Decode (UINT4 * output, unsigned char * input, unsigned int len);
  	void MD5_memcpy(POINTER output, POINTER input, unsigned int len);
	void MD5_memset(POINTER output, int value, unsigned int len);	
	void RC4PrepareKey(unsigned char *key_data_ptr, int key_data_len, rc4_key *key);
	static void swap_byte(unsigned char *a, unsigned char *b);
public:
	CMD5(void);
	unsigned char * MD5Calculate(unsigned char *);	
    void RC4Calculate(unsigned char *buffer_ptr, int buffer_len, unsigned char * pass_key);	
};

#endif
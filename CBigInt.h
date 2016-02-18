#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define BII_POS 1
#define BII_ZER 0
#define BII_NEG -1

typedef struct bigint{
	uint32_t *val;
	size_t len;
	char sign;
} bigint;

//initialization functions
void bigint_initlen(bigint *r,  size_t len);
void bigint_init(bigint *r);
void bigint_init32(bigint *r, uint32_t i);
//bigint bigint_init64(uint64_t i);
//void bigint_init32s(bigint *r, int32_t t);
void bigint_inithex(bigint *r,  char *hex);
void bigint_initrand(bigint *r, uint32_t nbits);

void bigint_setval(bigint *dest, bigint src);
void bigint_setval32(bigint *a, uint32_t val);
void bigint_setvalhex(bigint *a, char *hex);
void bigint_setvalrand(bigint *dest, uint32_t nbits);

void bigint_tohex(bigint a, char **buf);
void bigint_tobinary(bigint a, char **buf);

void bigint_free(bigint *a);

 

//#define BII_MAX(X,Y) (((X)>(Y))?(X):(Y))

/*********************
 * GENERAL UTILITIES *
 * *******************/

char bigint_cmp(bigint a, bigint b);
uint8_t bigint_parity(bigint a);
void bigint_plusequals(bigint *a, bigint b);
void bigint_minusequals(bigint *a, bigint b);
void bigint_timesequals(bigint *a, bigint b);
void bigint_modequals(bigint *a, bigint m);
void bigint_incr(bigint *a);
void bigint_decr(bigint *a);
void bigint_getsigbits(bigint *dest, bigint a, uint32_t nbits);
char bigint_isval(bigint a, uint32_t val);
void bigint_setneg(bigint *b);
void bigint_setpos(bigint *b);
void bigint_setzer(bigint *b);
void bigint_negate(bigint *b);


//arithmetic
void bigint_rightshift(bigint *a, uint32_t nbits);
void bigint_leftshift(bigint *a, uint32_t nbits);
void bigint_add(bigint *sum, bigint a, bigint b);
void bigint_subtract(bigint *diff, bigint a, bigint b);
void bigint_multiply(bigint *prod, bigint a, bigint b);
void bigint_divide(bigint *q, bigint *r, bigint a, bigint b);

//number theory
void bigint_egcd(bigint a, bigint b, bigint c, bigint *x, bigint *y);
void bigint_modexp(bigint *dest, bigint base, bigint exp, bigint mod);


#define MAX(X,Y) (((X)>(Y))?(X):(Y))

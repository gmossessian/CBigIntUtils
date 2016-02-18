#pragma once

#include "CBigInt.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#define bii_sigbits(b) (bii_sigbits32(b.val[0])+((b.len-1)<<5))

void bii_cat(bigint *dest, bigint cat);
char bii_cmp(bigint a, bigint b);

uint8_t bii_sigbits32(uint32_t a);
void bii_stripzeroes(bigint *a);
uint32_t bii_ispowerof2(bigint a);
void bii_add(bigint *sum, bigint a, bigint b);
void bii_subtract(bigint *diff, bigint a, bigint b);
void bii_divide(bigint *q, bigint *r, bigint a, bigint b);
//void bii_div2(bigint *q, bigint *r, bigint a, bigint b);


extern int subt_printing;

int bii_test_modexp(void);
int bii_test_egcd(void);
int bii_test_divide(void);

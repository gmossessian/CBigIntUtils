/*
 * Writing some bigint stuff in C... 
 * 
 */
 
#include "CBigInt_internal.h"

//#define BII_MAX(X,Y) (((X)>(Y))?(X):(Y))

/*********************
 * GENERAL UTILITIES *
 * *******************/

char bigint_cmp(bigint a, bigint b){
	if(a.sign == BII_POS){
		if(b.sign == BII_NEG || b.sign == BII_ZER) return 1;
		if(b.sign == BII_POS) return bii_cmp(a,b);
	}
	if(a.sign == BII_NEG){
		if(b.sign == BII_POS || b.sign == BII_ZER) return -1;
		if(b.sign == BII_NEG) return bii_cmp(b,a);
	}
	if(a.sign == BII_ZER){
		if (b.sign == BII_POS) return -1;
		if (b.sign == BII_NEG) return 1;
		if (b.sign == BII_ZER) return 0;
	}
	return 0;
}

char bii_cmp(bigint a, bigint b){ // a>b is 1. a==b is 0. a<b is -1. Assumes no leading zero words.
	if(a.len > b.len) return 1;
	if(a.len < b.len) return -1;
	
	for(size_t i = 0; i<a.len; i++){
		if(a.val[i] > b.val[i]) return 1;
		if(a.val[i] < b.val[i]) return -1;
	}
	return 0;
}

uint8_t bigint_parity(bigint a){
	return a.val[a.len-1]&1;
}

void bii_cat(bigint *dest, bigint src){
	dest->val = realloc(dest->val, (dest->len + src.len)*sizeof(uint32_t));
	memcpy(&(dest->val[dest->len]), src.val, (src.len*sizeof(uint32_t)));
	dest->len += src.len;
}

uint32_t bii_ispowerof2(bigint a){ //returns -1 if not, otherwise returns the power
	int p=-1;
	for(uint32_t i=0; i<a.len; i++){
		for(uint32_t j=0; j<32; j++){
			if(((a.val[i] >> j) & 1)){
				if(p!=-1) return -1;
				p = ((a.len-i-1)<<5)+j;
			}
		}
	}
	return p;
}

void bigint_plusequals(bigint *a, bigint b){
	bigint tmp;
	bigint_init(&tmp);
	bigint_setval(&tmp, *a);
	bigint_add(a,tmp,b);
	bigint_free(&tmp);
}

void bigint_minusequals(bigint *a, bigint b){
	bigint tmp;
	bigint_init(&tmp);
	bigint_setval(&tmp, *a);
	bigint_subtract(a,tmp,b);
	bigint_free(&tmp);

}

void bigint_timesequals(bigint *a, bigint b){
	bigint tmp;
	bigint_init(&tmp);
	bigint_setval(&tmp, *a);
	bigint_multiply(a,b,tmp);
	bigint_free(&tmp);

}

void bigint_modequals(bigint *a, bigint m){
	bigint tmp;
	bigint tmp2;
	bigint_init(&tmp);
	bigint_init(&tmp2);
	bigint_setval(&tmp, *a);
	bigint_divide(&tmp2, a, tmp, m);
	bigint_free(&tmp);
	bigint_free(&tmp2);
}

void bigint_incr(bigint *a){ //increment a
	bigint one;
	bigint_init32(&one, 1);
	bigint_plusequals(a,one);
	bigint_free(&one);
}

void bigint_decr(bigint *a){ //decrement a
	bigint one;
	bigint_init32(&one, 1);
	bigint_minusequals(a,one);
	bigint_free(&one);
}

void bigint_getsigbits(bigint *dest, bigint a, uint32_t nbits){
	if (nbits > bii_sigbits(a)){
		bigint_setval(dest,a);
		return;
	}
	if(nbits == 0){
		bigint_setval32(dest,0);
		return;
	}
	uint32_t mask = (((uint64_t)1<<(nbits&31))-1)&0xFFFFFFFF;
	
	dest->len = ((nbits-1)>>5)+1;
	dest->val = realloc(dest->val, dest->len*sizeof(uint32_t));
	dest->sign = a.sign;
	
	for(uint32_t i = 0; i<dest->len-1; i++){
		dest->val[dest->len-i-1] = a.val[a.len-i-1];
	}

	dest->val[0] = a.val[a.len-dest->len]&mask;
}

uint8_t bii_sigbits32(uint32_t a){
	uint8_t i;
	uint32_t t=0x80000000;
	for(i=0; (a&t)==0 && i<32; i++) t>>=1;
	return 32-i;
}

void bii_stripzeroes(bigint *a){
	size_t i;
	for(i=0; i<a->len; i++){
		if(a->val[i]!=0) break;
	}
	
	if(i==a->len){
		bigint_setval32(a,0);
		return;
	}
	
	bigint A;
	bigint_initlen(&A, (a->len)-i);
	for(size_t j=i ; j < a->len; j++){
		A.val[j-i] = a->val[j];
	}
	A.sign = a->sign;
	bigint_setval(a,A);
	bigint_free(&A);
}

char bigint_isval(bigint a, uint32_t val){
	if(a.val[0]==val && a.len==1) return 1;
	return 0;
}

void bigint_setneg(bigint *b){ 
	b->sign = BII_NEG;
}

void bigint_setpos(bigint *b){ 
	b->sign = BII_POS;
}

void bigint_setzer(bigint *b){ 
	bigint_setval32(b,0);
}

void bigint_negate(bigint *b){ 
	if(b->sign == BII_NEG) bigint_setpos(b);
	else if(b->sign == BII_POS) bigint_setneg(b);
}



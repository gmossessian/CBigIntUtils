#include "CBigInt_internal.h"

static uint8_t bii_hexvallookup(char c);
static const char *BII_HEX="0123456789abcdef";

void bigint_initlen(bigint *r, size_t len){
/*
 * all other initialization functions should refer to this one
 * initalizes a length=len, 0-value bigint.
 */
	r->val = calloc(len, sizeof(uint32_t));
	r->len = len;
	r->sign=BII_ZER;
}

void bigint_init(bigint *r){
	/*
	 * Initializes a 0-valued bigint.
	 */
	 bigint_initlen(r, 1);
}

void bigint_init32(bigint *r, uint32_t i){
	/*
	 * Initalize a bigint from an unsigned 32-bit integer
	 */
	bigint_initlen(r,1);
	if(i!=0) bigint_setpos(r);
	r->val[0] = i;
}
/*
bigint bigint_init64(uint64_t i){
	if((i >> 32) == 0) return bigint_init32(i&0xFFFFFFFF);
	bigint r;
	bigint_initlen(&r, 2);
	if(i!=0) bigint_setpos(&r);
	r.val[0] = (uint32_t)((i >> 32) & 0xFFFFFFFF);
	r.val[1] = (uint32_t)(i & 0xFFFFFFFF);
	
	return r;
}*/

void bigint_setval32(bigint *a, uint32_t val){
	a->len=1;
	a->val = realloc(a->val, a->len*sizeof(uint32_t));
	a->val[0] = val;
	if(val > 0) a->sign = BII_POS;
	else a->sign = BII_ZER;
}

void bigint_setvalhex(bigint *a, char *hex){
	bigint tmp;
	bigint_inithex(&tmp, hex);
	bigint_setval(a,tmp);
	bigint_free(&tmp);
}

void bigint_setval(bigint *dest, bigint src){
	/*
	 * Set the value of dest to the value in src.
	 */
	dest->len = src.len;
	dest->val = realloc(dest->val, (dest->len)*sizeof(uint32_t));
	dest->val = memcpy(dest->val, src.val, dest->len*sizeof(uint32_t));
	dest->sign = src.sign;
}

void bigint_setvalrand(bigint *dest, uint32_t nbits){
	bigint tmp; 
	bigint_initrand(&tmp, nbits);
	bigint_setval(dest,tmp);
	bigint_free(&tmp);
}
/*
void bigint_init32s(bigint *r, int32_t t){
	bigint_init32(r, t & 0x7FFFFFFF);
	if(t<0){
		bigint_setneg(&r);
	}
	return r;
}*/

void bigint_inithex(bigint *b, char *hex){
	size_t len = strlen(hex);
	if(len==0){
		bigint_setval32(b,0);
		return;
	}
	
	bigint_initlen(b, ((len-1)>>3)+1);
			
	char *nhex = hex;
	if(hex[0] == '-'){
		nhex++;
		len--;
		bigint_setneg(b);
	} else{
		bigint_setpos(b);
	}
	
	if(len>2 && nhex[1]=='x'){
		nhex+=2;
		len-=2;
	}

	for(size_t i=0; i<len; i++){
		b->val[b->len-(i>>3)-1] |= ((uint32_t)(bii_hexvallookup(nhex[len-i-1])) << ((i&7)<<2));
	}
}

void bigint_initrand(bigint *r, uint32_t nbits){
	bigint_initlen(r, ((nbits-1)>>5)+1);
	uint32_t mask = ((((uint64_t)1)<<(32-((32-(nbits&31))&31)))-1)&0xFFFFFFFF;
	r->val[0] = (((rand()&0xFFFF) << 16) | (rand()&0xFFFFF))&mask;
	for(uint32_t i = 1; i<r->len; i++){
		r->val[i] =  ((rand()&0xFFFF) << 16) | (rand()&0xFFFFF);
	}
	bigint_setpos(r);
}

uint8_t bii_hexvallookup(char c){
	switch(tolower(c)){
		case '0': return 0; break;
		case '1': return 1; break;
		case '2': return 2; break;
		case '3': return 3; break;
		case '4': return 4; break;
		case '5': return 5; break;
		case '6': return 6; break;
		case '7': return 7; break;
		case '8': return 8; break;
		case '9': return 9; break;
		case 'a': return 0xa; break;
		case 'b': return 0xb; break;
		case 'c': return 0xc; break;
		case 'd': return 0xd; break;
		case 'e': return 0xe; break;
		case 'f': return 0xf; break;
		default: break;
	} 
	return 0xFF;
}

void bigint_free(bigint *a){
	free(a->val);
}

void bigint_tohex(bigint a, char **buf){
	if(a.sign==BII_ZER){
		*buf = realloc(*buf, 4*sizeof(char));
		strcpy(*buf, "0x0");
		return;
	}
	size_t hexlen = (a.len)<<3;
	uint8_t offset = 2;
	if(a.sign==BII_NEG){
		 offset++;
	 }
	 hexlen+=offset;
	(*buf) = realloc((*buf),(hexlen+1)*sizeof(char));
	
	if(a.sign==BII_NEG){
		(*buf)[0]='-';
	}
	(*buf)[offset-2]='0';
	(*buf)[offset-1]='x';
	
	uint8_t j;
	for(size_t i=0; i<a.len; i++){
		for(j=0; j<8; j++){
			(*buf)[ (i<<3) + j + offset] = BII_HEX[(a.val[i] >> ((7-j)<<2)) & 0xF];
		}
	}
	(*buf)[offset+(a.len<<3)] = 0;

	//any leading 0s should be removed:
	size_t i;
	for(i=offset; (*buf)[i+1]!=0; i++){
		if( (*buf)[i]!='0' ) break;
	}
	
	size_t k;
	if(i!=offset){
		for(k=offset; (*buf)[k+i-offset]!=0; k++){
			(*buf)[k] = (*buf)[k+i-offset];
		}
	}
	
	(*buf) = realloc((*buf), (k+1)*sizeof(char));
	
	(*buf)[k] = 0;
}

void bigint_tobinary(bigint a, char **buf){
	*buf = calloc((a.len << 5) + 1, sizeof(char));
	uint8_t j;
	for(size_t i=0; i<a.len; i++){
		for(j=0; j<32; j++){
			(*buf)[(i<<5)+j] = ((a.val[i] >> (31-j)) & 1) ? '1' : '0';
		}
	}
	(*buf)[a.len << 5] = 0;
	
	uint32_t i=0;
	while((*buf)[i]=='0' && (*buf)[i+1]!=0) ++i;
	memmove(*buf, (*buf)+i, strlen(*buf)+1-i);
}

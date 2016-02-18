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

#define MAX(X,Y) (((X)>(Y))?(X):(Y))

static const char *BII_HEX="0123456789abcdef";

int subt_printing=0;

typedef struct bigint{
	uint32_t *val;
	size_t len;
	char sign;
} bigint;

bigint bigint_initlen(size_t len);
bigint bigint_init(void);
bigint bigint_inithex(char *hex);
uint8_t bii_hexvallookup(char c);
bigint bigint_init32(uint32_t i);
void bigint_free(bigint *a);
char *bigint_tohex(bigint a);

void bigint_setpos(bigint *b);
char bigint_cmp(bigint a, bigint b);
char bii_cmp(bigint a, bigint b); 
void bigint_cpy(bigint *dest, bigint src);
char bigint_isval(bigint a, uint32_t val);
uint8_t bii_sigbits32(uint32_t a);

void bigint_rightshift(bigint *a, uint32_t nbits);
void bigint_leftshift(bigint *a, uint32_t nbits);
void bii_add(bigint a, bigint b, bigint *sum);
void bii_subtract(bigint a, bigint b, bigint *diff);

void bigint_setneg(bigint *b);
void bii_divide(bigint a, bigint b, bigint *q, bigint *r);
void bii_findquotient32(bigint a, bigint b,  bigint *q, bigint *r);
void bigint_add(bigint a, bigint b, bigint *sum);
void bigint_subtract(bigint a, bigint b, bigint *diff);
uint32_t bii_ispowerof2(bigint a);
void bigint_multiply(bigint a, bigint b, bigint *prod);
void bigint_divide(bigint a, bigint b, bigint *q, bigint *r);
void bigint_incr(bigint *a);
void bigint_decr(bigint *a);
 void bigint_egcd(bigint a, bigint b, bigint c, bigint *x, bigint *y);


bigint bigint_initlen(size_t len){
/*
 * all other initialization functions should refer to this one
 * initalizes a length=len, 0-value bigint.
 */
	bigint r;
	r.val = calloc(len, sizeof(uint32_t));
	r.len = len;
	r.sign=BII_ZER;
	return r;
}
 
bigint bigint_init(void){ 
	bigint r = bigint_initlen(1);
	return r;
}

bigint bigint_init32(uint32_t i){ //initialize a length-1 bigint storing a single uint32_t.
	bigint r = bigint_initlen(1);
	if(i!=0) bigint_setpos(&r);
	r.val[0] = i;
	return r;
}

void bigint_setval32(bigint *a, uint32_t val){ //re-initialize $a to a length-1 bigint storing a single uint32_t.
	bigint_free(a);
	*a = bigint_init32(val);
}


bigint bigint_inithex(char *hex){
	size_t len = strlen(hex);
	if(len==0){
		return bigint_init32(0);
	}
	
	bigint b = bigint_initlen(((len-1)>>3)+1);
	
	char *nhex = hex;
	if(hex[0] == '-'){
		nhex++;
		len--;
		bigint_setneg(&b);
	} else{
		bigint_setpos(&b);
	}

	for(size_t i=0; i<len; i++){
		b.val[b.len-(i>>3)-1] |= ((uint32_t)(bii_hexvallookup(nhex[len-i-1])) << ((i&7)<<2));
	}
	return b;
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

void bigint_setpos(bigint *b){ //set b.sign to BII_POS
	b->sign = BII_POS;
}

void bigint_setneg(bigint *b){ // set b.sign to BII_NEG
	b->sign = BII_NEG;
}

void bigint_negate(bigint *b){ //reverse the sign of b
	if(b->sign == BII_NEG) bigint_setpos(b);
	else if(b->sign == BII_POS) bigint_setneg(b);
}

void bigint_free(bigint *a){ // just frees the uint32_t *val. 
	free(a->val);
}

char *bigint_tohex(bigint a){ //returns a hex-string representation of the bigint, with no leading 0s and a leading - if it's negative
	char *hex = calloc((a.len<<3) + 1,sizeof(char));
	uint8_t j;
	for(size_t i=0; i<a.len; i++){
		for(j=0; j<8; j++){
			hex[ (i<<3) + j ] = BII_HEX[(a.val[i] >> ((7-j)<<2)) & 0xF];
		}
	}
	hex[a.len<<3] = 0;
	
	//any leading 0s should be removed:
	int i=0;
	while(hex[i]=='0' && hex[i+1]!=0) ++i;
	
	memmove(hex,hex+i,strlen(hex)+1-i);
	
	if(a.sign == BII_NEG){
		hex = realloc(hex, (strlen(hex)+1)*sizeof(char));
		memmove(hex+1,hex,strlen(hex)+1);
		hex[0]='-';
	}
	return hex;
}

char bigint_cmp(bigint a, bigint b){// a>b is 1. a==b is 0. a<b is -1. 
	if(a.sign == BII_POS){
		if(b.sign == BII_NEG || b.sign == BII_ZER) return 1;
		if(b.sign == BII_POS) return bii_cmp(a,b);
	} else if(a.sign == BII_NEG){
		if(b.sign == BII_POS || b.sign == BII_ZER) return -1;
		if(b.sign == BII_NEG) return bii_cmp(b,a);
	} else if(a.sign == BII_ZER){
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

void bii_cat(bigint *dest, bigint src){ //concatenate the arrays. only used in long division and in the case that summation results in a longer answer. 
	dest->val = realloc(dest->val, (dest->len + src.len)*sizeof(uint32_t));
	memcpy(&(dest->val[dest->len]), src.val, (src.len*sizeof(uint32_t)));
	dest->len += src.len;
}

uint8_t bigint_parity(bigint a){ //1 if it's odd, 0 if it's even. 
	return a.val[a.len-1]&1;
}

uint8_t bii_sigbits32(uint32_t a){ //count the number of significant bits in $a
	uint8_t i;
	uint32_t t=0x80000000;
	for(i=0; (a&t)==0 && i<32; i++) t>>=1;
	return 32-i;
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

void bii_stripzeroes(bigint *a){ // reallocate a->val so that a->val[0] != 0. 
	size_t i=0;
	while(a->val[i]==0 && i < a->len) i++;
	
	if(i==a->len){
		bigint_free(a);
		*a = bigint_init32(0);
		return;
	}
	bigint A = bigint_initlen(a->len-i);
	memcpy(A.val, &(a->val[i]), (a->len-i)*sizeof(uint32_t));
	A.sign = a->sign;
	bigint_cpy(a,A);
	bigint_free(&A);
}

void bigint_cpy(bigint *dest, bigint src){ //copy src to dest.
	if(dest->len!=src.len){
		dest->val = realloc(dest->val, src.len*sizeof(uint32_t));
		dest->len = src.len;
	}
	memcpy(dest->val, src.val, sizeof(uint32_t)*src.len);
	dest->sign = src.sign;
}

char bigint_isval(bigint a, uint32_t val){ //test whether $a is a len-1 bigint equal to val. return 1 if true, else 0. 
	if(a.val[0]==val && a.len==1) return 1;
	return 0;
}

void bigint_plusequals(bigint *a, bigint b){ //wrapper for bigint_add, $a += $b.
	bigint tmp = bigint_init();
	bigint_cpy(&tmp, *a);
	bigint_add(tmp,b,a);
	bigint_free(&tmp);
}

void bigint_minusequals(bigint *a, bigint b){ //wrapper for bigint_subtract, $a -= $b.
	bigint tmp = bigint_init();
	bigint_cpy(&tmp, *a);
	bigint_subtract(tmp,b,a);
	bigint_free(&tmp);

}

bigint bigint_getsigbits(bigint a, uint32_t nbits){ // return $a % 2 ^ $nbits, as a bigint
	if (nbits > a.len<<5){
		return a;
	}
	if(nbits == 0){
		return bigint_init32(0);
	}
	uint32_t mask = (((uint64_t)1<<(nbits&31))-1)&0xFFFFFFFF;
	bigint b = bigint_initlen(((nbits-1)>>5)+1);

	for(uint32_t i = 0; i<b.len-1; i++){
		b.val[b.len-i-1] = a.val[a.len-i-1];
	}

	b.val[0] = a.val[a.len-b.len]&mask;
	return b;
}

void bigint_rightshift(bigint *a, uint32_t nbits){ //bitshift
	if(nbits == 0){
		return;
	}
	if(nbits > bii_sigbits32(a->val[0]) + ((a->len-1)<<5)){
		bigint_setval32(a,0);
		return;
	}
	
	uint32_t words = nbits >> 5;
	uint32_t bits = nbits & 31;
	uint32_t rmask = (1<<bits)-1;
	uint32_t lmask = ~rmask;
	
	size_t i;
	
	for(i=a->len-1; i>words; i--){
		a->val[i] = ((a->val[i-words] & lmask) >> bits) | ((a->val[i-words-1] & rmask) << ((32-bits)&31));
	}
	a->val[i] = (a->val[i-words] & lmask) >> bits;
	for( ; i>0; i--){
		a->val[i-1]=0;
	}
	
	bii_stripzeroes(a);
}

void bigint_leftshift(bigint *a, uint32_t nbits){ //bitshift
	if(nbits==0){
		return;
	}
	
	uint32_t wrds = nbits >> 5;
	uint32_t bits = nbits & 31;
	
	uint32_t *old = a->val;
	uint32_t *new;
	size_t newlen= a->len;
	
	//count the number of leading 0 bits
	uint8_t numz=0;
	while(((old[0] & (0x80000000 >> numz))) == 0 && (numz<32)){
		 numz++;
	 }
	if(numz == 32){
		bii_stripzeroes(a);
		bigint_leftshift(a, nbits);
		return;
	}
	
	//if necessary, allocate a new uint32_t array
	if(wrds == 0 && bits <= numz){
		new = old;
	} else{
		uint32_t newbits = (a->len<<5) + nbits - numz;
		newlen = ((newbits-1)>>5)+1;
		new = calloc(newlen, sizeof(uint32_t));
	}
	
	//shift as necessary.
	if(bits == 0){
		for(uint32_t i=0; i<a->len; i++){
			new[i] = old[i];
		}
	} else if (bits <= numz){
		for(uint32_t i=0; i<a->len-1; i++){
			new[i] = (old[i] << bits) | (old[i+1] >> (32-bits));
		}
		new[a->len-1] = old[a->len-1] << bits;
	} else if (bits > numz){
		new[0] = old[0] >> (32-bits);
		for(uint32_t i=1; i<a->len; i++){
			new[i] = (old[i-1] << bits) | (old[i] >> (32-bits));
		}
		new[a->len] = old[a->len-1] << bits; //this is ok, because if bits > numz, new is longer than old
	}
	
	if(old!=new){
		free(old);
	}
	
	a->val = new;
	a->len = newlen;
}

void bii_add(bigint a, bigint b, bigint *sum){//assumes a, b positive
	if(sum->len != MAX(a.len, b.len)){
		sum->val = realloc(sum->val, MAX(a.len, b.len)*sizeof(uint32_t));
		sum->len = MAX(a.len, b.len);
	}
	uint64_t tempsum;
	uint32_t carry = 0;
	
	bigint *shorter = (a.len < b.len ? &a : &b);
	bigint *longer = (a.len < b.len ? &b : &a);
	uint32_t i,si,li,sui;
	for(i=0; i<shorter->len; i++){
		si = shorter->len-i-1;
		li = longer->len-i-1;
		sui = sum->len - i-1;
		tempsum = (uint64_t)shorter->val[si] + (uint64_t)longer->val[li] + (uint64_t)carry;
		sum->val[sui] = (uint32_t)(tempsum & 0xFFFFFFFF);
		carry = (uint32_t)((tempsum >> 32) & 0xFFFFFFFF);
	}
	for( ; i<longer->len; i++){
		li = longer->len-i-1;
		sui = sum->len-i-1;
		tempsum = (uint64_t)longer->val[li] + (uint64_t)carry;
		sum->val[sui] = (uint32_t)(tempsum & 0xFFFFFFFF);
		carry = (uint32_t)((tempsum >> 32) & 0xFFFFFFFF);
	}
	if(carry){
		bigint bicarry = bigint_init32(carry);
		bii_cat(&bicarry, *sum);
		bigint_cpy(sum,bicarry);
		bigint_free(&bicarry);
	}	
	bii_stripzeroes(sum);
}

void bii_subtract(bigint a, bigint b, bigint *diff){ //assume a, b positive
	char cmp = bigint_cmp(a,b);	
	if(cmp==0){ //if they're equal return 0.
		bigint_setval32(diff,0);
		return;
	}
	
	bigint *smaller = (cmp==1 ? &b : &a);
	bigint *larger = (cmp==1 ? &a : &b); 
	
	bigint_cpy(diff, *larger);
	
	uint32_t i,j,si,di,sv,dv;
	uint8_t carry;
	for(i=0; i<smaller->len; i++){ //basic subtraction in a column, not much to explain. 
		si = smaller->len-i-1;
		di = diff->len-i-1;
		sv = smaller->val[si];
		dv = diff->val[di];
		if(dv >= sv){
			diff->val[di] -= sv;
			carry=0;
		} else {
			diff->val[di] = ~(sv-dv);
			diff->val[di]++;
			carry = 1;
		}
		j = di;
		while(carry == 1){
			j--;
			if(diff->val[j] >= carry){
				diff->val[j]-=carry;
				carry=0;
			} else{
				diff->val[j] = 0xFFFFFFFF;
			}
		}
	}
	if(cmp==-1) bigint_setneg(diff);
	bii_stripzeroes(diff);
}

void bii_divide(bigint a, bigint b, bigint *q, bigint *r){ //assume a, b positive.
	if(bigint_isval(b,0)){
		bigint_setval32(r,0);
		return;
	}
	int p = bii_ispowerof2(b);
	if(p != -1){	//if dividing by a power of 2, bitshift right. This catches the case of dividing by 1 as well.
		bigint tmp = bigint_getsigbits(a,p);
		bigint_cpy(r, tmp);

		bigint_cpy(q,a);
		bigint_rightshift(q,(uint32_t)p);
		return;
	}
	
	bigint tmpq = bigint_init();
	bigint tmpr = bigint_init();
	bigint tmp = bigint_initlen(1);
		
		
	bigint divisor = bigint_init32(a.val[0]); //doing long division, drop down the most significant word of a. 
	bigint quotient = bigint_init32(0);
	
		
	
	int i = 1;
	do{
		//build up the long division divisor: while it's smaller than b, keep appending less significant words of a.
		while(bii_cmp(divisor,b)==-1 && i<a.len){
			tmp.val[0] = a.val[i];
			bii_cat(&divisor, tmp);
			i++;
		}
		if(i==a.len && bii_cmp(divisor, b)==-1){ //if there are no more digits of a to drop down
			break;
		}
		bii_findquotient32(divisor, b, &tmpq, &tmpr); // do the uint32_t division
		bii_cat(&quotient, tmpq); //append the result to the quotient
		bigint_cpy(&divisor, tmpr); //the remainder becomes the divisor
	}while(i>=0);
	
	bigint_cpy(q,quotient);
	bigint_cpy(r,divisor);
	bigint_free(&quotient);
	bigint_free(&divisor);
}

void bii_findquotient32(bigint a, bigint b,  bigint *q, bigint *r){ //an O(log n) division 
	bigint p = bigint_init();
	
	bigint qi   = bigint_init32(0x80000000);
	bigint diff = bigint_init32(0x40000000);
	
	bigint_multiply(b,qi,&p);
	
	while(!bigint_isval(diff,0)){
		if(bii_cmp(a, p) == 1){ // if a > b *q, then q is too small.
			bigint_plusequals(&qi,diff);
		}
		else  if(bii_cmp(a, p) == -1){ // if a < b * q, then q is too big.
			bigint_minusequals(&qi,diff);
		}
		else if(bii_cmp(a, p)==0){
			bigint_cpy(q,qi);
			bigint_cpy(r,bigint_init32(0));
			bigint_free(&p);
			return;
		}
		bigint_multiply(b,qi,&p);
		bigint_rightshift(&diff,1);
	}
	while(bii_cmp(a,p)==1){ // if a > b*q, q is too small, so increment it. Afterwards, it will be 1 too big.
		bigint_incr(&qi);
		bigint_multiply(qi,b,&p);
	}
	while(bii_cmp(a,p) == -1){ // while a < b*q, decrement q
		bigint_decr(&qi);
		bigint_multiply(qi,b,&p);
	}
	bigint_cpy(q,qi);
	bigint_subtract(a,p,r);
	
	bigint_free(&qi);
	bigint_free(&p);
}

void bigint_add(bigint a, bigint b, bigint *sum){ //wrapper for bii_add
	if(a.sign == BII_ZER){
		bigint_cpy(sum, b);
	} else if(b.sign == BII_ZER){
		bigint_cpy(sum, a);
	} else if(a.sign == BII_POS && b.sign == BII_NEG) {
		bii_subtract(a,b,sum);
	} else if(a.sign == BII_NEG && b.sign == BII_POS){
		bii_subtract(b,a,sum);
	} else if(a.sign == BII_NEG && b.sign == BII_NEG){
		bii_add(a, b, sum);
		bigint_setneg(sum);
	} else {
		bii_add(a, b, sum);
	}
}

void bigint_subtract(bigint a, bigint b, bigint *diff){ //wrapper for bii_subtract
	//if(subt_printing) printf("BIGINT_SUBTRACT diff address is %p\n", diff);fflush(stdout);
	
	if(a.sign == BII_ZER){
		bigint_cpy(diff, b);
		bigint_negate(diff);
	} else if(b.sign == BII_ZER){
		bigint_cpy(diff,a);
	} else if(a.sign == BII_POS && b.sign == BII_NEG){
		bii_add(a,b,diff);
		bigint_setpos(diff);
	} else if(a.sign == BII_NEG && b.sign == BII_POS){
		bii_add(a,b,diff);
		bigint_setneg(diff);
	} else if(a.sign == BII_NEG && b.sign == BII_NEG){
		bii_subtract(b,a,diff);
	} else {
		bii_subtract(a,b,diff);
	}
	if(subt_printing) printf("BIGINT_SUBTRACT diff address is %p, value is %s. Signs are %i %i.\n", diff, bigint_tohex(*diff), a.sign, b.sign);fflush(stdout);

}

void bigint_multiply(bigint a, bigint b, bigint *prod){
	
	bigint_setval32(prod,0);
	if(bigint_isval(a,0) || bigint_isval(b,0)){
		return;
	}
	
	int p = bii_ispowerof2(a);
	if(p!=-1){
		bigint_cpy(prod, b);
		bigint_leftshift(prod,p);
		return;
	}
	p = bii_ispowerof2(b);
	if(p!=-1){
		bigint_cpy(prod,a);
		bigint_leftshift(prod,p);
		return;
	}

	bigint A = bigint_init();
	bigint B = bigint_init();
	
	bigint_cpy(&A,a);
	bigint_cpy(&B,b);
	
	bigint_setpos(&A);
	bigint_setpos(&B);

	uint64_t l = a.len << 5;
	bigint temp = bigint_init();
	for(uint64_t i=0; i<l; i++){
		if(bigint_parity(A)==1){
			bigint_cpy(&temp, *prod);
			bigint_add(temp,B,prod);
		}
		bigint_rightshift(&A,1);
		bigint_leftshift(&B,1);
	}
	prod->sign = a.sign * b.sign;
	bigint_free(&temp);
	bigint_free(&A);
	bigint_free(&B);
}

void bigint_divide(bigint a, bigint b, bigint *q, bigint *r){
	bii_divide(a, b, q, r);
	
	bigint tmp = bigint_init();
	if(a.sign == BII_NEG && b.sign == BII_NEG){
		bigint_incr(q);
		bigint_cpy(&tmp, *r);
		bigint_subtract(b,tmp,r);
	}
	else if(a.sign == BII_POS && b.sign == BII_NEG){
		bigint_negate(q);
	}
	else if(a.sign == BII_NEG && b.sign == BII_POS){
		bigint_incr(q);
		bigint_setneg(q);
		bigint_cpy(&tmp, *r);
		bigint_subtract(b,tmp,r);
	}
	bigint_free(&tmp);
}

void bigint_incr(bigint *a){ //increment a
	bigint one = bigint_init32(1);
	bigint_plusequals(a,one);
	bigint_free(&one);
}

void bigint_decr(bigint *a){ //decrement a
	bigint one = bigint_init32(1);
	bigint_minusequals(a,one);
	bigint_free(&one);
}
 
 void bigint_egcd(bigint a, bigint b, bigint c, bigint *x, bigint *y){
	
	bigint snew = bigint_init32(0);
	bigint sold = bigint_init32(1);
	bigint tnew = bigint_init32(1);
	bigint told = bigint_init32(0);
	bigint rnew = bigint_init();
	bigint rold = bigint_init();
	bigint tmp1 = bigint_init();
	bigint tmp2 = bigint_init();
	bigint q = bigint_init();
	bigint r = bigint_init();
	bigint ratio = bigint_init();

	printf("\tcalling egcd(0x%s, 0x%s, 0x%s)\n", bigint_tohex(a), bigint_tohex(b),bigint_tohex(c));
	
	uint8_t revflag = 0;
	if(bigint_cmp(a,b)==-1){
		bigint_cpy(&rnew,a);
		bigint_cpy(&rold,b);
		revflag = 1;
	} else{
		bigint_cpy(&rnew, b);
		bigint_cpy(&rold, a);
	}
		
	while(!bigint_isval(rnew, 0)){
		subt_printing = 0;
		bigint_divide(rold,rnew,&q,&r);
		subt_printing=1;
		printf("\t%s = %s * %s + %s\n", bigint_tohex(rold), bigint_tohex(rnew), bigint_tohex(q), bigint_tohex(r));fflush(stdout);

		bigint_cpy(&tmp1, rnew); 
		//printf("\tupdated r = %s - %s * %s = \n", bigint_tohex(rold), bigint_tohex(q), bigint_tohex(rnew));fflush(stdout);

		bigint_multiply(q,rnew,&tmp2); 
		printf("\tcalling bigint_subtract(rold,tmp2,&rnew).  &rold = %p, &tmp2 = %p, &rnew = %p\n", &rold, &tmp2, &rnew);fflush(stdout);
		//printf("\t%s - %s = (%p) \n", bigint_tohex(rold), bigint_tohex(tmp2), &rnew);fflush(stdout);

		bigint_subtract(rold,tmp2,&rnew); 
		printf("\trnew = %s at address %p. rold is at address %p.\n", bigint_tohex(rnew),&rnew, &rold);fflush(stdout);
		bigint_cpy(&rold, tmp1);
		
		bigint_cpy(&tmp1, snew); 
		bigint_multiply(q,snew,&tmp2); 
		bigint_subtract(sold,tmp2,&snew); 
		bigint_cpy(&sold, tmp1);
		
	
		bigint_cpy(&tmp1, tnew); 
		//printf("\tupdated t = %s - %s * %s = ", bigint_tohex(told), bigint_tohex(q), bigint_tohex(tnew));fflush(stdout);
		bigint_multiply(q,tnew,&tmp2); 
		//printf("%s - %s = ", bigint_tohex(told), bigint_tohex(tmp2));fflush(stdout);
		bigint_subtract(told,tmp2,&tnew); 
		//printf("%s at address %p\n", bigint_tohex(tnew),&tnew);
		bigint_cpy(&told, tmp1);
		
		//printf("\n");
		//sleep(1);
	}

	//at this point, sold*a + told*b = rold = gcd(A,B), and tnew, snew are quotients of a,b by the gcd.
	
	bigint_divide(c,rold, &ratio,&r);
	if(!bigint_isval(r,0)){
		x = NULL;
		y = NULL;
		return;
	}
	if(revflag==1){
		bigint_cpy(&tmp1, sold);
		bigint_cpy(&sold, told);
		bigint_cpy(&told, tmp1);
	}
	
	//normalise s, t so that 0 <= s < b
	if(sold.sign == -1){
		bigint_cpy(&tmp1, sold);
		bigint_setpos(&tmp1);
		bigint_cpy(&tmp2,b);
		bigint_setpos(&tmp2);
		bigint_divide(tmp1,tmp2,&q,&r);
		bigint_multiply(q,tmp2,&tmp1);
		bigint_cpy(&tmp2,sold);
		bigint_add(tmp1,tmp2,&sold);
		bigint_cpy(&tmp1,a);
		bigint_setpos(&tmp1);
		bigint_multiply(q,tmp1,&tmp2);
		bigint_cpy(&tmp1,told);
		bigint_subtract(tmp1,tmp2,&told);
	}
	
	//multiply by c/gcd to get the correct coefficients
	bigint_multiply(sold,ratio,x);
	bigint_multiply(told,ratio,y);
	
	bigint_free(&snew);
	bigint_free(&sold);
	bigint_free(&tnew);
	bigint_free(&told);
	bigint_free(&rnew);
	bigint_free(&rold);
	bigint_free(&tmp1);
	bigint_free(&tmp2);
	bigint_free(&q);
	bigint_free(&r);
	bigint_free(&ratio);
}

int main(void){
	printf("testing egcd\n");

	srand(time(NULL));
	

	bigint test1 = bigint_inithex("71edf247ef2f5f7029ff5e0041454192");
	bigint test2 = bigint_inithex("3bce654299afbf5a428de3725a0d4edb");
	bigint test3 = bigint_inithex("f4b53cef50afe2e4");
	bigint x = bigint_init();
	bigint y = bigint_init();

	bigint_egcd(test1,test2,test3,&x,&y);
	printf("0x%s * 0x%s + 0x%s * 0x%s == 0x%s\n", bigint_tohex(test1),bigint_tohex(x),bigint_tohex(test2),bigint_tohex(y),bigint_tohex(test3));
	return 1;
}

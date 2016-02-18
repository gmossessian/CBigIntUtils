#include "CBigInt_internal.h"

void bigint_rightshift(bigint *a, uint32_t nbits){
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

void bigint_leftshift(bigint *a, uint32_t nbits){
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

void bii_add(bigint *sum, bigint a, bigint b){//assumes a, b positive
	if(sum->len != MAX(a.len, b.len)){
		sum->val = realloc(sum->val, MAX(a.len, b.len)*sizeof(uint32_t));
		sum->len = MAX(a.len, b.len);
	}
	uint64_t tempsum;
	uint32_t carry = 0;
	
	bigint shorter = (a.len < b.len ? a : b);
	bigint longer = (a.len < b.len ? b : a);
	uint32_t i,si,li,sui;
	for(i=0; i<shorter.len; i++){
		si = shorter.len-i-1;
		li = longer.len-i-1;
		sui = sum->len - i-1;
		tempsum = (uint64_t)shorter.val[si] + (uint64_t)longer.val[li] + (uint64_t)carry;
		sum->val[sui] = (uint32_t)(tempsum & 0xFFFFFFFF);
		carry = (uint32_t)((tempsum >> 32) & 0xFFFFFFFF);
	}
	for( ; i<longer.len; i++){
		li = longer.len-i-1;
		sui = sum->len-i-1;
		tempsum = (uint64_t)longer.val[li] + (uint64_t)carry;
		sum->val[sui] = (uint32_t)(tempsum & 0xFFFFFFFF);
		carry = (uint32_t)((tempsum >> 32) & 0xFFFFFFFF);
	}
	if(carry > 0){
		bigint bicarry;
		bigint_init32(&bicarry, carry);
		bii_cat(&bicarry, *sum);
		bigint_setval(sum,bicarry);
		bigint_free(&bicarry);
	}	
	bii_stripzeroes(sum);
	bigint_setpos(sum);
}

void bii_subtract(bigint *diff, bigint a, bigint b){ //assume a, b positive
	char cmp = bii_cmp(a,b);	
	bigint smaller;
	bigint_init(&smaller);

	if(cmp==0){ //if they're equal return 0.
		bigint_setval32(diff,0);
		return;
	}else if(cmp==1){
		bigint_setval(diff,a);
		bigint_setval(&smaller, b);
	} else if(cmp==-1){
		bigint_setval(diff,b);
		bigint_setval(&smaller, a);
	} else{
		fprintf(stderr, "something went wrong in bii_subtract, bii_cmp returned an invalid value.\n");
		exit(1);
	}
		
	uint32_t i,j,si,di,sv,dv;
	uint8_t carry;
	for(i=0; i<smaller.len; i++){ //basic subtraction in a column, not much to explain. 
		si = smaller.len-i-1;
		di = diff->len-i-1;
		sv = smaller.val[si];
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
	bii_stripzeroes(diff);
	if(cmp==1) bigint_setpos(diff);
	if(cmp==-1)bigint_setneg(diff);
	bigint_free(&smaller);
}


//void bii_divide(bigint *q, bigint *r, bigint a, bigint b){ //assumes a, b positive
	/*
	 * This divisin takes log n steps, where n is the difference between the number of significant bits in a and in b.
	 * if sigbits(a)-sigbits(b) = n, the quotient must be between 2**(n+1) and 2**(n-1)
	 * Start at q = 2**n, and loop. 
	 * At the kth iteration of the loop, adjust q by 2**(n-k-1) depending on whether q is too small or too big.
	 * After n-1 iterations, we have the quotient, but it's possibly too large by 1. 
	 */
/* 
	if(bii_cmp(a,b)==-1){
		bigint_setval32(q,0);
		bigint_setval(r,a);
		return;
	}
	
	int lendiff = bii_sigbits(a)-bii_sigbits(b);
	
	if(lendiff == 0){
		bigint_setval32(q,1);
		bii_subtract(r,a,b);
		return;
	}
	
	bigint minq; 
	bigint maxq; 
	
	bigint_init32(&minq,1);
	bigint_init32(&maxq,1);
	
	bigint_leftshift(&minq, lendiff - 1);
	bigint_leftshift(&maxq, lendiff + 1);
	
	bigint qi;
	bigint_init32(&qi,1);
	
	
	bigint diff;
	bigint_subtract(&diff,maxq,minq);
	bigint_rightshift(&diff, 2);
	
	bigint p; 
	bigint_init(&p);
	bigint_multiply(&p,b,qi);

	while(!bigint_isval(diff,0)){	
		if(bii_cmp(a,p) == 1){ //if a > b*q, q may be too small
			bigint_plusequals(&qi, diff);
		} else if(bii_cmp(a,p) == -1){ // if a < b*q, q may be too big
			bigint_minusequals(&qi, diff);
		} else if(bii_cmp(a,p) == 0){
			break;
		}
		bigint_multiply(&p,b,qi);
		bigint_rightshift(&diff,1);
	}
	
	if(bii_cmp(a,p)==-1){
		bigint_decr(&qi);
		bigint_minusequals(&p,b);
	}
	
	bigint_setval(q,qi);
	bigint_subtract(r,a,p);
	
	bigint_free(&minq);
	bigint_free(&maxq);
	bigint_free(&qi);
	bigint_free(&diff);
	bigint_free(&p);
}*/

void bii_divide(bigint *q, bigint *r, bigint a, bigint b){
	if(bii_cmp(a,b)==-1){
		bigint_setval32(q,0);
		bigint_setval(r,a);
		return;
	}
	
	uint32_t lendiff = bii_sigbits(a)-bii_sigbits(b);
	
	if(lendiff == 0){
		bigint_setval32(q,1);
		bii_subtract(r,a,b);
		return;
	}
	
	bigint A;
	bigint_init(&A);
	bigint_setval(&A,a);
	bigint_setpos(&A);
	
	bigint B;
	bigint_init(&B);
	bigint_setval(&B,b);
	bigint_setpos(&B);
	
	bigint one;
	bigint_init(&one);

	char *abuf = NULL;
	char *bbuf = NULL;
	
	bigint_tohex(A,&abuf);
	bigint_tohex(B,&bbuf);
		
	bigint_setval32(q,0);
	
	while(bigint_cmp(A,b)==1){
		bigint_setval(&B,b);
		bigint_setpos(&B);
		bigint_setval32(&one,1);
		lendiff = bii_sigbits(A) - bii_sigbits(B);
		bigint_leftshift(&B,lendiff);

		if(bigint_cmp(A,B)==-1){
			bigint_rightshift(&B,1);
			lendiff--;
		}

		bigint_minusequals(&A,B);

		bigint_leftshift(&one,lendiff);
		bigint_plusequals(q,one);
	}
	bigint_setval(r,A);
	
	bigint_free(&A);
	bigint_free(&B);
	bigint_free(&one);
}

void bigint_add(bigint *sum, bigint a, bigint b){
	if(a.sign == BII_ZER){
		bigint_setval(sum, b);
	} else if(b.sign == BII_ZER){
		bigint_setval(sum, a);
	} else if(a.sign == BII_POS && b.sign == BII_NEG) {
		bii_subtract(sum,a,b);
	} else if(a.sign == BII_NEG && b.sign == BII_POS){
		bii_subtract(sum,b,a);
	} else if(a.sign == BII_NEG && b.sign == BII_NEG){
		bii_add(sum, a, b);
		bigint_setneg(sum);
	} else {
		bii_add(sum, a, b);
	}
}

void bigint_subtract(bigint *diff,bigint a, bigint b){ //wrapper for bii_subtract
	if(subt_printing){
		printf("\t\t%i*%08x - %i*%08x\n", a.sign,a.val[0],b.sign,b.val[0]);fflush(stdout);
	}
	if(a.sign == BII_ZER){
		bigint_setval(diff, b);
		bigint_negate(diff);
	} else if(b.sign == BII_ZER){
		bigint_setval(diff,a);
	} else if(a.sign == BII_POS && b.sign == BII_NEG){
		bii_add(diff,a,b);
		bigint_setpos(diff);
	} else if(a.sign == BII_NEG && b.sign == BII_POS){
		bii_add(diff,a,b);
		bigint_setneg(diff);
	} else if(a.sign == BII_NEG && b.sign == BII_NEG){
		bii_subtract(diff,b,a);
	} else {
		bii_subtract(diff,a,b);
	}
}

void bigint_multiply(bigint *prod, bigint a, bigint b){
	if(bigint_isval(a,0) || bigint_isval(b,0)){
		bigint_setval32(prod, 0);
		return;
	}
	/*int p = bii_ispowerof2(a);
	if(p!=-1){
		bigint_setval(prod, b);
		bigint_leftshift(prod,p);
		if(a.sign == b.sign) bigint_setpos(prod); else bigint_setneg(prod);
		return;
	}
	p = bii_ispowerof2(b);
	if(p!=-1){
		bigint_setval(prod,a);
		bigint_leftshift(prod,p);
		if(a.sign == b.sign) bigint_setpos(prod); else bigint_setneg(prod);
		return;
	}*/
	
	bigint_setval32(prod,0);

	bigint A;
	bigint B;
	bigint_init(&A);
	bigint_init(&B);
	
	bigint_setval(&A,a);
	bigint_setval(&B,b);
	
	bigint_setpos(&A);
	bigint_setpos(&B);

	while(!bigint_isval(A,0)){
		if(bigint_parity(A)==1){
			bigint_plusequals(prod, B);
		}
		bigint_rightshift(&A,1);
		bigint_leftshift(&B,1);
	}
	if(a.sign == b.sign) bigint_setpos(prod);
	else bigint_setneg(prod);
	
	bigint_free(&A);
	bigint_free(&B);
}
/*
void bigint_karatsuba(bigint *prod, bigint a, bigint b){
	if(a.len == 1 || b.len == 1){
		bigint_multiply(prod, a, b);
		return;
	}
	size_t m = MAX(a.len, b.len) >> 1;
	bigint high1, low1, high2, low2, z0, z1, z2, tmp1, tmp2;
	
	bigint_init(&high1);
	bigint_init(&low1);
	bigint_init(&high2);
	bigint_init(&low2);
	bigint_init(&z0);
	bigint_init(&z1);
	bigint_init(&z2); 
	
	bigint_getsigbits(&low1, a, m2<<5);
	bigint_subtract(&high1, a, low1);
	
	bigint_getsigbits(&low2, b, m2<<5);
	bigint_subtract(&high2, b, low2);
	
	bigint_karatsuba(&z0, low1, low2);
	bigint_karatsuba(&z1, 
	
	bigint_free(&high1);
	bigint_free(&low1);
	bigint_free(&high2);
	bigint_free(&low2);
}*/
//~ procedure karatsuba(num1, num2)
  //~ if (num1 < 10) or (num2 < 10)
    //~ return num1*num2
  //~ /* calculates the size of the numbers */
  //~ m = max(size_base10(num1), size_base10(num2))
  //~ m2 = m/2
  //~ /* split the digit sequences about the middle */
  //~ high1, low1 = split_at(num1, m2)
  //~ high2, low2 = split_at(num2, m2)
  //~ /* 3 calls made to numbers approximately half the size */
  //~ z0 = karatsuba(low1,low2)
  //~ z1 = karatsuba((low1+high1),(low2+high2))
  //~ z2 = karatsuba(high1,high2)
  //~ return (z2*10^(2*m2))+((z1-z2-z0)*10^(m2))+(z0)


void bigint_divide(bigint *q, bigint *r, bigint a, bigint b){
	bigint A;
	bigint_init(&A);
	bigint_setval(&A,a);
	bigint B;
	bigint_init(&B);
	bigint_setval(&B,b);
	bigint_setpos(&A);
	bigint_setpos(&B);
	
	bii_stripzeroes(&A);
	bii_stripzeroes(&B);
	
	bii_divide( q, r,A, B);
	bigint_free(&A);
	bigint_free(&B);
	
	bigint tmp;
	bigint_init(&tmp);
	if(a.sign == BII_NEG && b.sign == BII_NEG){
		bigint_incr(q);
		bigint_setval(&tmp, *r);
		bigint_subtract(r,b,tmp);
	}
	else if(a.sign == BII_POS && b.sign == BII_NEG){
		bigint_negate(q);
	}
	else if(a.sign == BII_NEG && b.sign == BII_POS){
		bigint_incr(q);
		bigint_setneg(q);
		bigint_setval(&tmp, *r);
		bigint_subtract(r,b,tmp);
	}
	bigint_free(&tmp);
	bii_stripzeroes(q);
	bii_stripzeroes(r);
}

int bii_test_divide(void){
	bigint a, b, r, q;
	
	bigint_init(&a);
	bigint_init(&b);
	bigint_init(&r);
	bigint_init(&q);
	
	bigint_setval32(&a,10);
	bigint_setval32(&b,3);
	
	bii_divide(&q,&r,a,b);
	
	if(!bigint_isval(q,3) || !bigint_isval(r,1)){
		printf("failed...");
		return 1;
	}
	
	bigint_free(&a);
	bigint_free(&b);
	bigint_free(&r);
	bigint_free(&q);
	return 0;
}

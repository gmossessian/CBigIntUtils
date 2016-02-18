#include "CBigInt_internal.h"

void bigint_egcd(bigint a, bigint b, bigint c, bigint *x, bigint *y){

	bigint r0, r1, r2, s1, s2, t1, t2, q, r, tmp1, tmp2;
	int revflag = 0;
	
	bigint_init(&r0);
	bigint_init(&r1);
	bigint_init(&r2);
	bigint_init(&s1);
	bigint_init(&s2);
	bigint_init(&t1);
	bigint_init(&t2);
	bigint_init(&q);
	bigint_init(&r);
	bigint_init(&tmp1);
	bigint_init(&tmp2);
	
	if(bigint_cmp(a,b)==-1){
		revflag = 1;
		bigint_setval(&r0, b);
		bigint_setval(&r1, a);
	} else{
		bigint_setval(&r0, a);
		bigint_setval(&r1, b);
	}
	bigint_divide(&q,&r2,r0,r1);
	
	bigint_setval32(&s1, 0);
	bigint_setval32(&s2, 1);
	
	bigint_setval32(&t1, 1);
	bigint_setval(&t2, q);
	bigint_negate(&t2);
	
	if(bigint_isval(r2,0)){
		bigint_divide(&tmp1,&tmp2,c,b);
		if(bigint_isval(tmp2,0)){
			if(revflag == 0){
				bigint_setval32(x,0);
				bigint_setval(y,tmp1);
				return;
			}
			bigint_setval(x,tmp1);
			bigint_setval32(y,0);
			return;
		}
		bigint_setval32(x,0);
		bigint_setval32(y,0);
		return;
	}
	while(!bigint_isval(r2,0)){
		bigint_divide(&q,&r,r1,r2);
		
		bigint_setval(&r1, r2);
		bigint_setval(&r2, r);
		
		bigint_multiply(&tmp1,s2,q);
		bigint_subtract(&tmp2,s1,tmp1);
		bigint_setval(&s1, s2);
		bigint_setval(&s2, tmp2);
		
		bigint_multiply(&tmp1,t2,q);
		bigint_subtract(&tmp2,t1,tmp1);
		bigint_setval(&t1, t2);
		bigint_setval(&t2, tmp2);
	}

	bigint_divide(&q,&r,c,r1);
	if(!bigint_isval(r,0)){
		bigint_setval32(x,0);
		bigint_setval32(y,0);
		return;
	}
	if(revflag==1){
		bigint_setval(&tmp1, s1);
		bigint_setval(&s1, t1);
		bigint_setval(&t1, tmp1);
	}
	
	if(s1.sign == BII_NEG){
		bigint_setval(&tmp1, s1);
		bigint_setpos(&tmp1);
		bigint_divide(&tmp2,&r,tmp1,b); 
		bigint_setval(&tmp1, tmp2); 
		if(!bigint_isval(r,0)) bigint_incr(&tmp1);
		bigint_multiply(&tmp2,tmp1,b);
		bigint_plusequals(&s1,tmp2);
		bigint_multiply(&tmp2,tmp1,a);
		bigint_minusequals(&t1,tmp2);
	}

	
	bigint_multiply(x,q,s1);
	bigint_multiply(y,q,t1);
	

	bigint_free(&r0);
	bigint_free(&r1);
	bigint_free(&r2);
	bigint_free(&s1);
	bigint_free(&s2);
	bigint_free(&t1);
	bigint_free(&t2);
	bigint_free(&q);
	bigint_free(&r);
	bigint_free(&tmp1);
	bigint_free(&tmp2);
}

int bii_test_egcd(void){
	bigint a;
	bigint b;
	bigint c;
	bigint x; 
	bigint y;
	
	bigint_initrand(&a, 128);
	bigint_initrand(&b, 128);
	bigint_initrand(&c, 64);
	bigint_init(&x);
	bigint_init(&y);
	
	do{
		bigint_egcd(a,b,c,&x,&y);
	}while(bigint_isval(x,0) && bigint_isval(y,0));
	
	bigint_timesequals(&x,a);
	bigint_timesequals(&y,b);
	bigint_minusequals(&c,x);
	bigint_minusequals(&c,y);
	
	if(!bigint_isval(c,0)){
		printf("egcd failed.\n");
		return 1;
	}
	
	bigint_setval32(&a,4);
	bigint_setval32(&b,3);
	bigint_setval32(&c,1);
	
	bigint_egcd(a,b,c,&x,&y);
	if(!bigint_isval(x,1) || !(bigint_isval(y,1) && y.sign == BII_NEG)){
		printf("egcd(4,3,1) failed.\n");
		return 1;
	}
	
	bigint_free(&a);
	bigint_free(&b);
	bigint_free(&c);
	bigint_free(&x);
	bigint_free(&y);
	return 0;
}
	
//printf("r = "); if(r2.sign==BII_NEG) printf("-"); for(size_t i=0; i<r2.len; i++) printf("%08x",r2.val[i]); printf("\n");
//printf("s = "); if(s2.sign==BII_NEG) printf("-"); for(size_t i=0; i<s2.len; i++) printf("%08x",s2.val[i]); printf("\n");
//printf("t = "); if(t2.sign==BII_NEG) printf("-"); for(size_t i=0; i<t2.len; i++) printf("%08x",t2.val[i]); 

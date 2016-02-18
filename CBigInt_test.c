#include "CBigInt_internal.h"
#include <stdio.h>

int subt_printing = 0;


int main (int argc, char **argv){
	printf("testing division...");fflush(stdout);
	if(bii_test_divide()==0) printf("OK\n");
	else return 1;

	printf("testing egcd...");fflush(stdout);
	if(bii_test_egcd()==0) printf("OK\n");
	else return 1;
	printf("testing modexp...");fflush(stdout);
	if(bii_test_modexp()==0) printf("OK\n");
	else return 1;
}


int bii_test_egcd(void){
	bigint a;
	bigint b;
	bigint c;
	bigint x; 
	bigint y;
	
	bigint_init(&a);
	bigint_init(&b);
	bigint_init(&c);
	bigint_init(&x);
	bigint_init(&y);
	
	bigint_setval32(&a,4);
	bigint_setval32(&b,3);
	bigint_setval32(&c,1);
	
	bigint_egcd(a,b,c,&x,&y);
	if(!bigint_isval(x,1) || !(bigint_isval(y,1) && y.sign == BII_NEG)){
		printf("egcd(4,3,1) failed.\n");
		return 1;
	}
	printf("test 1 OK...");fflush(stdout);

	bigint_setvalrand(&a, 128);
	bigint_setvalrand(&b, 128);
	bigint_setvalrand(&c, 64);
	
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
	printf("test 2 OK...");fflush(stdout);
	
	bigint_free(&a);
	bigint_free(&b);
	bigint_free(&c);
	bigint_free(&x);
	bigint_free(&y);
	return 0;
}

int bii_test_modexp(void){
	bigint dest, base, exp, mod;
	bigint_init(&dest);
	bigint_init(&base);
	bigint_init(&exp);
	bigint_init(&mod);
	
	bigint_setval32(&base, 375);
	bigint_setval32(&exp, 249);
	bigint_setval32(&mod, 388);
	
	bigint_modexp(&dest, base, exp, mod);
	if(!bigint_isval(dest, 175)){ 
		printf("Failed.\n");
		return 1;
	} 
	
	bigint_setval32(&base,2);
	bigint_setvalhex(&mod, "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7edee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3dc2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f83655d23dca3ad961c62f356208552bb9ed529077096966d670c354e4abc9804f1746c08ca237327ffffffffffffffff");
	bigint_setvalrand(&exp, 1600);
	bigint_modequals(&exp,mod);
	
	char *sbase = NULL;
	char *smod = NULL;
	char *sexp = NULL;
	char *sdest = NULL;
	
	bigint_tohex(base, &sbase);
	bigint_tohex(mod, &smod);
	bigint_tohex(exp, &sexp);
	
	printf("modexp( %s, %s, %s)\n", sbase, sexp, smod);fflush(stdout);
	
	bigint_modexp(&dest, base, exp, mod);
	
	bigint_tohex(dest, &sdest);
	printf("%s\n", sdest);
	return 0;
}

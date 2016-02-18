#include "CBigInt_internal.h"

static void bii_modexpodd(bigint *dest, bigint a, bigint e, bigint n);
static void bii_modexpeven(bigint *dest, bigint a, bigint e, bigint n);
static void bii_monproduct(bigint *dest, bigint abar, bigint bbar, bigint n, bigint nprime, uint32_t rpow);
static void bii_binarymodpow_powerof2(bigint *dest, bigint base, bigint exp, uint32_t modpow);

static char *buf;

void bigint_modexp(bigint *dest, bigint base, bigint exp, bigint mod){
	if(bigint_parity(mod)==1){
		bii_modexpodd(dest, base, exp, mod);
	} else bii_modexpeven(dest, base, exp, mod);
}

void bii_modexpodd(bigint *dest, bigint a, bigint e, bigint n){
	/*
	 * Modular exponentiation, returns a**e mod n
	 * Uses the Mongtomery Product algorithm to speed up a standard repeated-squaring routine
	 */
	 	 
	bigint r, nprime, tmp1, tmp2, abar, xbar;
	
	bigint_init(&r);
	bigint_init(&nprime);
	bigint_init(&tmp1);
	bigint_init(&tmp2);
	bigint_init(&abar);
	bigint_init(&xbar);

	//r is the least power of 2 which is larger than n. 
	bigint_setval32(&r,1);
	uint32_t rpow = bii_sigbits(n)-1;
	bigint_leftshift(&r, rpow);
	if(bigint_cmp(r,n)==-1){
		bigint_leftshift(&r,1);
		rpow++;
	}
	 
	//r * r^{-1} - n*n' = 1. Use the euclidean algorithm to find n'.
	bigint_setval32(&tmp1, 1);
	bigint_egcd(r,n, tmp1, &tmp2, &nprime);
	bigint_setpos(&nprime);
	 
	//abar = a * r mod n
	bigint_multiply(&tmp1,a,r);
	bigint_divide(&tmp2,&abar,tmp1,n);
	 
	bigint_divide(&tmp1,&xbar,r,n);
	 
	fflush(stdout);
	for(int i = bii_sigbits(e)-1; i>=0; i--){
		bii_monproduct(&tmp1, xbar, xbar, n, nprime, rpow);
		bigint_setval(&xbar, tmp1);
		bigint_setval(&tmp2,e);
		bigint_rightshift(&tmp2,i);
		if(bigint_parity(tmp2)==1){
			bii_monproduct(&tmp1,abar,xbar,n,nprime,rpow);
			bigint_setval(&xbar,tmp1);
		}
	}
	bigint_setval32(&tmp1, 1);
	bii_monproduct(dest, xbar, tmp1, n, nprime, rpow);
	 
	bigint_free(&r);
	bigint_free(&nprime);
	bigint_free(&tmp1);
	bigint_free(&tmp2);
	bigint_free(&abar);
	bigint_free(&xbar);
}

void bii_modexpeven(bigint *dest, bigint a, bigint e, bigint n){
	/*
	 * Returns a**e mod n for the case when n is even. 
	 * This algorithm is from the paper:
	 * Montgomery reduction with even modulus
	 * Koc,C.K.
	 * IEE Proceedings - Computers and Digital Techniques(1994),141(5):314
	 * http://dx.doi.org/10.1049/ip-cdt:19941291
	 */
	 
	bigint q, j, A, E, x1, x2, y,  qinv, tmp1, tmp2;
	bigint_init(&q);
	bigint_init(&j);
	bigint_init(&A);
	bigint_init(&E);
	bigint_init(&x1);
	bigint_init(&x2);
	bigint_init(&y);
	bigint_init(&qinv);
	bigint_init(&tmp1);
	bigint_init(&tmp2);
	
	//n = q * (2**jpow)
	bigint_setval(&q,n);
	uint32_t jpow = 0;
	bigint_setval32(&j,1);
	while(bigint_parity(q)==0){
		bigint_rightshift(&q,1);
		bigint_leftshift(&j,1);
		jpow++;
	}
	
	bigint_divide(&tmp1,&A,a,q);
	
	bii_modexpodd(&x1, A, e, q);
	
	bigint_getsigbits(&A,a,jpow);
	bigint_getsigbits(&E, e, jpow-1);
	
	bii_binarymodpow_powerof2(&x2,A,E,jpow);
	
	bigint_setval32(&tmp1, 1);
	bigint_egcd(q,j,tmp1, &qinv,&tmp2);
	
	bigint_subtract(&tmp1,x2,x1);
	
	bigint_multiply(&tmp2,tmp1,qinv);
	
	bigint_divide(&tmp1,&y,tmp2,j);

	bigint_multiply(&tmp1,q,y);
	bigint_add(dest,x1,tmp1);
	
	bigint_free(&q);
	bigint_free(&j);
	bigint_free(&A);
	bigint_free(&E);
	bigint_free(&x1);
	bigint_free(&x2);
	bigint_free(&y);
	bigint_free(&qinv);
	bigint_free(&tmp1);
	bigint_free(&tmp2);

}

void bii_monproduct(bigint *dest, bigint abar, bigint bbar, bigint n, bigint nprime, uint32_t rpow){
	bigint t, m, u, tmp1, tmp2;
	bigint_init(&t);
	bigint_init(&m);
	bigint_init(&u);
	bigint_init(&tmp1);
	bigint_init(&tmp2);
	
	bigint_multiply(&t, abar, bbar);
	
	bigint_multiply(&tmp1, t, nprime);
	bigint_getsigbits(&m, tmp1, rpow);
	bigint_multiply(&tmp1, m,n);
	bigint_add(&u,t,tmp1);
	bigint_rightshift(&u,rpow);
	
	if(bigint_cmp(u,n)>=0) bigint_subtract(dest,u,n);
	else bigint_setval(dest, u);
	
	bigint_free(&t);
	bigint_free(&m);
	bigint_free(&u);
	bigint_free(&tmp1);
	bigint_free(&tmp2);
}

void bii_binarymodpow_powerof2(bigint *dest, bigint base, bigint exp, uint32_t modpow){
	//if the modulus is a power of 2, division and modular arithmetic is very fast. 
	if(modpow==0){
		bigint_setval32(dest, 0);
		return;
	}
	bigint res, B, E, tmp1;
	bigint_init(&res);
	bigint_init(&B);
	bigint_init(&E);
	bigint_init(&tmp1);
	
	bigint_setval32(&res,1);
	bigint_getsigbits(&B,base, modpow);
	bigint_setval(&E, exp);
	
	while(!bigint_isval(E,0)){
		if(bigint_parity(E)==1){
			bigint_multiply(&tmp1, res, B);
			bigint_getsigbits(&res, tmp1, modpow);
		}
		bigint_rightshift(&E,1);
		bigint_multiply(&tmp1, B,B);
		bigint_getsigbits(&B,tmp1,modpow);
	}
	bigint_setval(dest, res);
	
	bigint_free(&res);
	bigint_free(&B);
	bigint_free(&E);
	bigint_free(&tmp1);
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
	buf = calloc(1, sizeof(char));
	
	bigint_modexp(&dest, base, exp, mod);
	if(!bigint_isval(dest, 175)){ 
		printf("Failed.\n");
		return 1;
	} 
	
	bigint_setval32(&base,2);
	bigint_setvalhex(&mod, "444291e51b3ea5fd16673e95674b01e7b");
	//bigint_setval32(&mod, 0x25);
	bigint_setvalrand(&exp, 400);
	bigint_modequals(&exp,mod);
	
	
	bigint_tohex(base,&buf);
	printf("modexp(%s, ",buf);
	
	bigint_tohex(exp, &buf);
	printf("%s, ", buf);
	
	bigint_tohex(mod, &buf);
	printf("%s)\n ", buf);
	fflush(stdout);
	
	bigint_modexp(&dest, base, exp, mod);
	printf("done.\n");fflush(stdout);
	
	bigint_tohex(base,&buf);
	printf("%s",buf);
	printf(" ** ");
	
	bigint_tohex(exp, &buf);
	printf("%s", buf);
	printf(" %% ");
	
	bigint_tohex(mod, &buf);
	printf("%s == ", buf);
	
	bigint_tohex(dest, &buf);
	printf("%s\n", buf);
	
	
	free(buf);
	
	return 0;
}

/*
 * 	//string p1 = charToS(0x25);
	//string p2 = base16Decode(newString("29bb3920ef5e958b9",0));
	string p3 = base16Decode(newString("444291e51b3ea5fd16673e95674b01e7b",0));
	//string p4 = base16Decode(newString("fb49eeac4dedd15d82be164ee3b0cbb22f7d79377",0));
	//my crappy old little laptop isn't powerful enough to do bigger primes than this...
	//string p5 = base16Decode(newString("6322dee2816b379bfd622fee57862827e9a941e5921f571e5d",0));
 	//string p6 = base16Decode(newString("efd19f2e8e87c453b59401661bb58f97b1ea71949ea3ae7b31359bfc34e7739c6776eedea9771ce830d8185e20d",0));
	//The math department computers were able to handle up to here. Interestingly, it doesn't take appreciably more *time* to compute these modexps, just more memory...
	//string p7 = base16Decode(newString("ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7edee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3dc2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f83655d23dca3ad961c62f356208552bb9ed529077096966d670c354e4abc9804f1746c08ca237327ffffffffffffffff",0));
	//string g0 = charToS(0x05);
	string g1 = charToS(0x02); 
	
	setDH_p(p3);
	setDH_g(g1);
	
	printf("Testing Diffie-Hellman with parameters\n");
	printf("p = ");printsint(getDH_p());PRINTNL;
	printf("g = ");printsint(getDH_g());PRINTNL;
	string a = bigIntDivide(randString(getDH_p().len+1),getDH_p())[1];
	printf("\trandom a mod p = ");printsint(a);PRINTNL;fflush(stdout);
	string A = bigIntModExp(getDH_g(),a,getDH_p());
	printf("\tA = g ** a mod p = ");printsint(A);PRINTNL;fflush(stdout);
	string b = bigIntDivide(randString(getDH_p().len+1),getDH_p())[1];
	printf("\trandom b mod p = ");printsint(b);PRINTNL;fflush(stdout);
	string B = bigIntModExp(getDH_g(),b,getDH_p());
	printf("\tB = g ** b mod p = ");printsint(B);PRINTNL;fflush(stdout);
	string s1 = bigIntModExp(B,a,getDH_p());
	printf("s1 = B ** a mod p = ");printsint(s1);PRINTNL;fflush(stdout);
	string s2 = bigIntModExp(A,b,getDH_p());
	printf("s2 = A ** b mod p = ");printsint(s2);PRINTNL;fflush(stdout);
	if(bigIntComp(s1,s2)==0){
		printf("Diffie-Hellman works!\n");
	}
	else{
		printf("Check your modExp, you fool.\n");
	}
}

*/

#include <stdio.h>
#include <gmp.h>

int main(int argc, char **argv){
	mpz_t prime;
	mpz_init(prime);
	mpz_set_str(prime, "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7edee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3dc2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f83655d23dca3ad961c62f356208552bb9ed529077096966d670c354e4abc9804f1746c08ca237327ffffffffffffffff", 16);
	
	mpz_t base;
	mpz_init(base);
	mpz_set_ui(base, 2);
	
	mpz_t exp;
	mpz_init(exp);
	mpz_set_str(exp, "16ffd3bddcd2076e9130eed97d6eb59dc4463f3d751ba7e953399e4ea9ada84a3714619b038d63761bcfa786cb0c1124ecdccdb4fd417f25b1458f5acd2d9088c1b5e4661139c8382e3b52841d3dd3f81855f0dbb0705a05304705a69709922e26c80042513c7cbfdbf6c3dbdb11c6f8d45a5d30dd5320a40d935d2cadb3ff2202719ce9fc197dacfee65f2ed5cb7147ee49a2aad18404cf16c9584ba56eef71ba085ad9cd9c61ff0f8dafa43f54e07b2c346389eebdc359afafd3e27d4d6a48",  16);
	
	mpz_t ans;
	mpz_init(ans);
	mpz_powm_sec(ans, base, exp, prime);
	
	gmp_printf("modexp(%#Zx, %#Zx, %#Zx) == %#Zx\n", base, exp, prime, 	ans);
	return 0;
}

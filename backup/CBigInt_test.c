#include "CBigInt_internal.h"
#include <stdio.h>

int subt_printing = 0;


int main (int argc, char **argv){
	//printf("testing egcd...");fflush(stdout);
	//if(bii_test_egcd()==0) printf("OK\n");
	//else return 1;
	fflush(stdout);
	printf("testing modexp...");fflush(stdout);
	if(bii_test_modexp()==0) printf("OK\n");
	else return 1;
}

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// For argument parsing
#include <errno.h>
#include <getopt.h>
#include <stdio.h>

#include "primes.h"

#define die(...) { fprintf(stderr, ##__VA_ARGS__); fprintf(stderr, "Call with -h or --help flag for more information\n"); exit(1); }


const char help_msg[] =
	"Usage:  primes [OPTION...]  [-n] RANGE\n"
	"   or:  primes [OPTION...]  -w WHEEL-SIZE [-n] RANGE\n"
	"   or:  primes [OPTION...]  -f [-n] RANGE\n"
	"   or:  primes [OPTION...]  -r FERMAT_PROP -w WHEEL-SIZE [-n] RANGE\n"
	"   or:  primes [OPTION...]  -m WITNESS,[WITNESSES...] [-n] RANGE\n"
	"\n"
	"Check primality of ranges of integers using various tests\n"
	"\n"
	"Options:\n"
	"  -n, --numbers [LOWER]:UPPER | NUMBER\n"
	"                         Give the range of numbers to check for primality\n"
	"  -w, --wheel WHEEL-SIZE Bound up to which primes will be used to create wheel\n"
	"                         (e.g. '-w 6' will create a modulus of 2 * 3 * 5 = 30)\n"
	"  -r, --fermat PROP      Use Fermat's algorithm to check N = a^2 - b^2 with\n"
	"                         `a` between sqrt(N) and sqrt(N) * (1 + PROP) where\n"
	"                         PROP is a positive float\n"
	"  -m, --miller-rabin WITNESS[,WITNESSES...]\n"
	"                         Use the miller-rabin primality test with the given\n"
	"                         witnesses (WARNING: probabilitistic, potentially wrong)\n"
	"  -d, --delim STRING     String used to separate the list of primes\n"
	"  -f, --factors          Factorize each number using given wheel, specified\n"
	"                         using -w. Defaulting to a wheel for -w 4.\n"
	"                         (NOTICE: can't be used with -q)\n"
	"  -q, --quiet            Don't display list of primes\n"
	"  -c, --count            Display the number of primes found in the range\n"
	"  -h, --help             Give this help list\n"
	"\n"
	"If no other method is selected then the Sieve of Eratosthenes is used.\n"
	"\n"
;


// Keep track of what method will be used to factor / check for primality
#define NO_METHOD 0
#define METHOD_ERATOS_SIEVE 1
#define METHOD_WHEEL 2
#define METHOD_FERMAT 3
#define METHOD_MILLER_RABIN 4
int method = NO_METHOD;

// String printed between found primes
const char *spacer = NULL;

// Lower and Upper bounds on integers
P_INT lower = 0, upper = 0;

int do_factors = 0;  // Whether to attempt to factorize the numbers
int quiet = 0;  // Whether to print out the primes
int show_count = 0;  // Whether to print the number of primes found


// Parameters for each method

// For Sieve of Eratosthenes
// None necessary

// For Wheel Factorization / Fermat Factorization
size_t wheel_size = 4;
pwheel_t whl;

// For Fermat Factorization
float fmt_prop;

// For Miller-Rabin test
// Dynamic Array of witness elements
size_t mr_wit_cap, mr_wit_len;
P_INT *mr_wits = NULL;


struct option longopts[] = {
	{"numbers", required_argument, NULL, 'n'},
	{"wheel", required_argument, NULL, 'w'},
	{"fermat", required_argument, NULL, 'r'},
	{"miller-rabin", required_argument, NULL, 'm'},
	{"delim", required_argument, NULL, 'd'},
	{"factors", no_argument, NULL, 'f'},
	{"quiet", no_argument, NULL, 'q'},
	{"count", no_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
	{0}
};

void parse_bounds(char *str);

void parse_opts(int key){
	char n;
	char *endptr = &n;
	switch(key){
		case 1:
		case 'n': parse_bounds(optarg);
		break;
		
		case 'w':
			if(method != NO_METHOD) die("Only one factorization / primality checking method may be specified\n");
			method = METHOD_WHEEL;
			
			errno = 0;
			wheel_size = (size_t)strtoul(optarg, &endptr, 10);
			if(errno || *endptr) die("Failed to parse wheel size \"%s\"\n", optarg);
		break;
		
		case 'r':
			if(method != NO_METHOD) die("Only one factorization / primality checking method may be specified\n");
			method = METHOD_FERMAT;
			
			// Parse fermat proportion
			errno = 0;
			fmt_prop = strtof(optarg, &endptr);
			if(errno || *endptr) die("Failed to parse Fermat proportion \"%s\"\n", optarg);
			if(fmt_prop <= 0) die("Propotion for Fermat factorization must be positive\n");
		break;
		
		case 'm':
			if(method != NO_METHOD) die("Only one factorization / primality checking method may be specified\n");
			method = METHOD_MILLER_RABIN;
			
			// Allocate space for witnesses
			mr_wit_len = 0;  mr_wit_cap = 4;
			mr_wits = malloc(mr_wit_cap * sizeof(P_INT));
			
			char *delim = ",";
			for(char *wit = strtok(optarg, delim); wit; wit = strtok(NULL, delim)){
				// Resize witness list if necessary
				if(mr_wit_len >= mr_wit_cap)
					mr_wits = realloc(mr_wits, (mr_wit_cap <<= 1) * sizeof(P_INT));
				
				// Try to parse witness
				errno = 0;
				mr_wits[mr_wit_len++] = strtoll(wit, &endptr, 10);
				if(errno || *endptr) die("Failed to parse Miller Rabin witness \"%s\"\n", wit);
			}
		break;
		
		
		// Set spacer characters
		case 'd': spacer = optarg;
		break;
		
		// Factorize each number instead of checking primality
		case 'f': do_factors = 1;
		break;
		// Suppress any print out
		case 'q': quiet = 1;
		break;
		// Show count of primes
		case 'c': show_count = 1;
		break;
		
		// Print help message
		case 'h':
			puts(help_msg);
			exit(0);
		
		// Unknown option
		case '?': die("Unknown Option -%c\n", optopt);
		break;
	}
}

/* Parse the boundary argument for the -n flag
 * Supports the formats
 *   NUMBER - Factorize or Check NUMBER for primality
 *   LOWER:UPPER - Factorize or Primality Check all x with LOWER <= x <= UPPER
 *   :UPPER - Factorize or Primality Check all x with 1 <= x <= UPPER
 */
void parse_bounds(char *arg){
	char *colon = strchr(arg, ':');
	
	if(colon){ // Format = LOWER:UPPER or :UPPER
		*colon = '\0';
		// Try to parse upper bound
		errno = 0;
		upper = strtoll(colon + 1, NULL, 10);
		if(errno) die("Failed to parse upper bound \"%s\"\n", colon + 1);
		
		// Try to parse lower bound
		if(arg == colon){
			lower = 1;
		}else{
			errno = 0;
			lower = strtoll(arg, NULL, 10);
			if(errno) die("Failed to parse lower bound \"%s\"\n", arg);
		}
	}else{ // Format = NUMBER
		errno = 0;
		upper = strtoll(arg, NULL, 10);
		if(errno) die("Failed to parse single number \"%s\"\n", arg);
		lower = upper;
	}
	
	if(upper == 0) die("Upper Bound must be greater than zero\n");
	if(lower == 0) die("Lower Bound must be greater than zero\n");
}




// Functions to check primality
// Store results of Sieve of Eratosthenes
BIT_TYPE *sieve_isprime = NULL;
int sieve_check(P_INT x){ return getbit(sieve_isprime, x); }

// Trial division with Wheel function
int wheel_check(P_INT x){ return is_prime_w(x, whl); }

// Fermat Factorization
int fermat_check(P_INT x){ return is_prime_fmt(x, whl, fmt_prop); }

// Miller Rabin Test
int miller_rabin_check(P_INT x){ return is_prime_mr(x, mr_wit_len, mr_wits); }


// Functions to factorize numbers
// Trial division with Wheel function
P_INT wheel_factors(P_INT x, int *pow){ return factorize_w(x, whl, pow); }



int main(int argc, char *argv[]){
	// Parse options
	int c;
	while((c = getopt_long(argc, argv, "-n:w:r:m:d:fqc", longopts, NULL)) >= 0) parse_opts(c);
	
	// Check for valid bounds
	if(upper < lower) die("Upper Bound must be greater than Lower Bound but %u < %u\n", upper, lower);
	if(upper == 0 || lower == 0) die("Bounds or Number must be provided\n");
	
	// Generate wheel from size
	if(wheel_size == 3 || wheel_size == 4) whl = PWHEEL_6;
	else if(wheel_size == 5 || wheel_size == 6) whl = PWHEEL_30;
	else whl = make_pwheel((unsigned char)(wheel_size > 30 ? 30 : wheel_size));
	
	// Set default separator
	if(!spacer) spacer = "\n";
	
	// Set default method to Sieve of Eratosthenes
	if(method == NO_METHOD) method = do_factors ? METHOD_WHEEL : METHOD_ERATOS_SIEVE;
	
	// Set functions to use according to method
	P_INT count = 0;
	if(do_factors){
		P_INT (*factors)(P_INT, int*);  // Pointer to method to use to factorize number with
		switch(method){
			case METHOD_WHEEL: factors = wheel_factors;
			break;
			
			case METHOD_ERATOS_SIEVE:
			case METHOD_FERMAT:
			case METHOD_MILLER_RABIN:
				die("Method cannot be used to factorize number(s)\n");
		}
		
		const char *sep = "";
		for(P_INT i = lower; i <= upper; i++){
			printf("%s%llu : ", sep, i);
			count++;
			sep = spacer;
			
			// Get and print factors
			const char *fac_sep = "";
			int pow = 0;
			for(P_INT fac = factors(i, &pow); fac; fac = factors(0, &pow)){
				printf(pow == 1 ? "%s%llu" : "%s%llu^%i", fac_sep, fac, pow);
				fac_sep = " * ";
			}
		}
	}else{
		int (*check)(P_INT);  // Pointeri to method to use to check for primality
		switch(method){
			case METHOD_WHEEL: check = wheel_check;
			break;
			case METHOD_ERATOS_SIEVE:
				sieve_isprime = malloc(byte_size(upper + 1));  // Allocate sieve
				prime_sieve_bs((bits_t)sieve_isprime, upper + 1);  // Perform sieving before printing
				check = sieve_check;
			break;
			case METHOD_FERMAT: check = fermat_check;
			break;
			case METHOD_MILLER_RABIN: check = miller_rabin_check;
			break;
		}
		
		// Check for primality on range
		const char *sep = "";
		for(P_INT i = lower; i <= upper; i++) if(check(i)){
			if(!quiet) printf("%s%llu", sep, i);
			count++;
			sep = spacer;
		}
	}
	
	if(!quiet) putchar('\n');
	
	// Print count if requested
	if(show_count) printf("Count: %llu\n", count);
	
	return 0;
}


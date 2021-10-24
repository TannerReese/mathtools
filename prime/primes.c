#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "primes.h"

P_INT prime_sieve(unsigned char *primality, P_INT size){
	P_INT n, i, count = 0;
	memset(primality, 1, size);
	
	primality[0] = 0;
	primality[1] = 0;
	for(n = 2; n < size; n++){
		if(primality[n]){
			count++;
			for(i = n + n; i < size; i += n) primality[i] = 0;
		}
	}
	
	return count;
}

// Bit array version
P_INT prime_sieve_bs(bits_t primality, P_INT size){
	P_INT n, i, count = 0;
	memset(primality, 0xffff, byte_size(size));
	
	setbit(primality, 0, 0);
	setbit(primality, 1, 0);
	for(n = 2; n < size; n++){
		if(getbit(primality, n)){
			count++;
			for(i = n + n; i < size; i += n) setbit(primality, i, 0);
		}
	}
	
	return count;
}



struct pwheel_s{
	unsigned char *first_prime, *last_prime;
	unsigned char *first_inc, *last_inc;
};

// Define often used simple wheels of size 6 and 30
static unsigned char W6_PRIMES[] = {2, 3}, W6_INCS[] = {4, 2};
static struct pwheel_s WHL_6_s = {W6_PRIMES, W6_PRIMES + 1, W6_INCS, W6_INCS + 1};
pwheel_t PWHEEL_6 = &WHL_6_s;

static unsigned char W30_PRIMES[] = {2, 3, 5}, W30_INCS[] = {6, 4, 2, 4, 2, 4, 6, 2};
static struct pwheel_s WHL_30_s = {W30_PRIMES, W30_PRIMES + 2, W30_INCS, W30_INCS + 7};
pwheel_t PWHEEL_30 = &WHL_30_s;



pwheel_t make_pwheel(unsigned char max){
	pwheel_t whl = malloc(sizeof(struct pwheel_s));
	
	char primality[max];
	unsigned char cnt = (unsigned char)prime_sieve(primality, max);
	whl->first_prime = malloc(sizeof(unsigned char) * cnt);
	whl->last_prime = whl->first_prime + cnt - 1;
	
	// Calculate product of primes
	cnt = 0; // Use cnt to index through primes
	P_INT product = 1;
	int n;
	for(n = 0; n < max; n++){
		if(primality[n]){
			whl->first_prime[cnt++] = n;
			product *= n;
		}
	}
	
	// Create buffer to record coprime elements
	char *buffer = malloc(sizeof(char) * product);
	for(n = 0; n < product; n++) buffer[n] = 1;
	buffer[0] = 0;
	
	// Find the coprime elements
	int inc_count = product - 1;
	unsigned char *pr;
	for(pr = whl->first_prime; pr <= whl->last_prime; pr++){
		for(n = *pr; n < product; n += *pr){
			if(buffer[n]){
				// Remove elements that are divisble by one of the primes
				inc_count--;
				buffer[n] = 0;
			}
		}
	}
	
	// Allocate space to store increments
	whl->first_inc = malloc(sizeof(unsigned char) * inc_count);
	whl->last_inc = whl->first_inc + inc_count - 1;
	
	cnt = 0; // Use cnt to index through increment sizes
	pr = whl->first_inc; // Use pr to index through increment list in wheel
	
	// Iterate through coprime list calculating increments between coprime elements
	for(n = 2; n < product; n++){
		cnt++;
		if(buffer[n]){
			*(pr++) = cnt;
			cnt = 0;
		}
	}
	
	// Make final increment loop back to 1 (mod product)
	*pr = cnt + 2;
	
	free(buffer);
	
	return whl;
}

void free_pwheel(pwheel_t whl){
	free(whl->first_inc);
	free(whl->first_prime);
	free(whl);
}

// Get the head element of the array of primes for whl
unsigned char *fstprm_w(pwheel_t whl){
	return whl->first_prime;
}

// Get the last element of the array of primes for whl
unsigned char *lstprm_w(pwheel_t whl){
	return whl->last_prime;
}

void nextnum_w(unsigned char **inc, P_INT *x, pwheel_t whl){
	if(!(*inc)){
		// Initialize inc and x if *inc == NULL
		*inc = whl->first_inc;
		*x = 1;
	}
	
	// Get next inc and x
	*x += *((*inc)++);
	if(*inc > whl->last_inc) *inc = whl->first_inc;
}




int is_prime_w(P_INT x, pwheel_t whl){
	if(x <= 1) return 0;
	
	// Iterate through primes to check
	unsigned char *p;  // Pointer to prime
	for_primes_w(p, whl){
		if(x == *p) return 1;
		if(x % *p == 0) return 0;
	}
	
	// Iterate through increments to check
	// Use uchr as increment indexer
	P_INT i = 1;
	unsigned char *inc;  // Pointer to increment
	for_nums_w(inc, i, whl, i * i <= x){
		if(x != i && x % i == 0) return 0;
	}
	return 1;
}

// When called repeatedly it will return each prime factor
// in ascending order along with its corresponding power.
// Returns 0 if no number given to factor or the prior number has been completely factored
P_INT factorize_w(P_INT x, pwheel_t wheel, int *pow){
	static P_INT factor, work = 0;
	
	// Initialize static variables in complete mode
	static pwheel_t whl = NULL;
	// Use num to track prime or increment
	static unsigned char *num = NULL;
	static int do_wheel_primes = 1;
	
	// Reset values when new number is given
	if(x && wheel){
		if(x <= 1){
			whl = NULL; // Ensure that function is set to complete mode
			return x == 1;
		}
		
		work = x;
		whl = wheel;
		
		// Setup to check through primes
		num = whl->first_prime;
		// Indicate to function to check trial divide by the wheel primes
		do_wheel_primes = 1;
	}else if(!whl){
		// Return 0 if prior task has completed
		return 0;
	}
	
	// Prevent Segfaults due to dereferencing NULL
	int pow_sub;
	if(!pow) pow = &pow_sub;
	
	// Iterate through base primes
	if(do_wheel_primes){
		// Use num to index through the primes
		for(; num <= whl->last_prime && work >= *num; num++){
			if(work == *num){
				whl = NULL;  // Return to complete mode
				*pow = 1;
				return work;  // Return prime with power 1
			}else if(work % *num == 0){
				// If work is divisible by num remove all powers
				*pow = 0;
				do{
					work /= *num;
					(*pow)++;
				}while(work % *num == 0);
				return *num;
			}
		}
		
		do_wheel_primes = 0;
		num = NULL;
		nextnum_w(&num, &factor, whl); // Use num to index through increments
	}
	
	// Iterate through increments
	while(factor * factor <= work){
		if(work % factor == 0){
			// If work is divisible by current factor remove power of factor from work
			*pow = 0;
			do{
				work /= factor;
				(*pow)++;
			}while(work % factor == 0);
			return factor;
		}
		
		nextnum_w(&num, &factor, whl);
	}
	
	whl = NULL;  // Set whl to NULL to indicate number is completely factored
	// Only return work if its prime
	if(work > 1){
		*pow = 1;
		return work;
	}else return 0;
}



static P_INT mod_pow(P_INT x, P_INT pow, P_INT modulo){
	// Create bit mask to obtain MSB
	P_INT mask = (P_INT)1 << sizeof(pow) * 8 - 1;
	
	P_INT work = 1;
	for(; pow > 0; pow <<= 1){
		// Check if top bit is set
		// If yes then multiply by the base
		if(pow & mask){
			work *= x;
			work %= modulo;
		}
		
		work *= work;
		work %= modulo;
	}
	return work;
}

int is_prime_mr(P_INT x, size_t wits_len, P_INT *wits){
	if(x < 2){ // 0 and 1 can cause errors
		return 0;
	}
	
	// x - 1 == odd_base * 2 ^ two_pow
	P_INT odd_base = x - 1;
	unsigned short two_pow = 0;
	while(odd_base % 2 == 0){
		odd_base /= 2;
		two_pow++;
	}
	
	P_INT witpow;
	unsigned short pow_cnt;
	for(; wits_len > 0; wits_len--, wits++){
		if(*wits % x == 0) continue; // if witness == 0 (mod tested) then test will be erroneous
		
		// Calculate: witpow <- wit ^ odd_base (mod x) 
		witpow = *wits;
		witpow = mod_pow(witpow, odd_base, x);
		
		// Check if: witpow == 1 (mod x)
		if(witpow == 1 || witpow == x - 1) continue;
		
		// Check if: there exists natural number n s.t.  witpow ^ (2 ^ n) == -1 (mod x)
		int probable_prime = 0;
		for(pow_cnt = two_pow; pow_cnt > 0; pow_cnt--){
			witpow *= witpow;
			witpow %= x;
			
			if(witpow == x - 1){
				probable_prime = 1;
				break;
			}
		}
		
		// If no n exists x is composite
		if(!probable_prime) return 0;
	}
	
	return 1;
}



int is_prime_fmt(P_INT x, pwheel_t whl, float above_sqrt){
	// 0 and 1 can cause errors
	if(x < 2) return 0;
	
	// First, iterate through wheel primes to check
	unsigned char *p;  // Pointer to prime
	for_primes_w(p, whl){
		if(x == *p) return 1;
		if(x % *p == 0) return 0;
	}
	
	// Second perform check using Fermat's algorithm if greater than 1024
	P_INT maxi = (P_INT)sqrt(x) + 1;
	if(x >> 10){
		P_INT a, b2, maxa = (P_INT)(sqrt(x) * (1 + above_sqrt));
		for(a = (P_INT)sqrt(x); a <= maxa ; a++){
			P_INT b, b2;
			b2 = a * a - x;
			if(b < 0) continue;
			
			b = (P_INT)(sqrt(b2) + 0.5);
			if(b * b == b2) return 0;
		}
		
		// Find new upper bound on divisor
		maxi = maxa - (P_INT)sqrt(maxa * maxa - x);
	}
	
	// Third do trial division using wheel increments
	P_INT i;  // Potential divisor
	unsigned char *inc;  // Pointer to increment
	for_nums_w(inc, i, whl, i <= maxi){
		if(x != i && x % i == 0) return 0;
	}
	return 1;
}



#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "bn.h"


// BN_TYPE value with all ones bits
#define BN_ALL_ONES (~(BN_TYPE)(0))

// Macros to extract the upper and lower parts of a BN_CALC_TYPE
#define calc_upper(bn_calc) ((BN_TYPE) ((bn_calc) >> (8 * sizeof(BN_TYPE))))
#define calc_lower(bn_calc) ((BN_TYPE) ((bn_calc) & BN_ALL_ONES))

// Bit mask for most significant bit of BN_TYPE
#define BN_TOP (1 << (8 * sizeof(BN_TYPE) - 1))
// Return the sign of a bn_t
#define bn_isneg_sp(len, digs) ((digs) && (len) > 0 && ((digs)[(len) - 1] & BN_TOP))
#define bn_isneg(num) bn_isneg_sp(bn_split(num))


// Access the nth digit of a big number
// If greater than the maximum digit use sign extension
#define get_digit_sp(len, digs, i) ((i) < (len) ? (digs)[i] : (bn_isneg_sp(len, digs) ? BN_ALL_ONES : 0))
#define get_digit(num, i) get_digit_sp(bn_split(num), i)

#define set_digit_sp(len, digs, i, val) if((i) < (len)) (digs)[i] = (BN_TYPE)(val)
#define set_digit(num, i, val) set_digit_sp(bn_split(num), i, val)

// Check if two numbers digits overlap in memory
#define is_overlap_sp(len1, digs1, len2, digs2) ((digs1) < (digs2) ? (digs1) + (len1) > (digs2) : (digs2) + (len2) > (digs1))
#define is_overlap(num1, num2) is_overlap_sp(bn_split(num1), bn_split(num2), num2)



bn_t bn_new(size_t len, BN_TYPE value){
	BN_TYPE *digs = malloc(sizeof(BN_TYPE) * len);
	// Set the value of the number to `value`
	return bn_set_sp(len, digs, value);
}

bn_t bn_copy_sp(size_t len, const BN_TYPE *src){
	struct bn_s new_num;
	new_num.length = len;
	new_num.digits = malloc(sizeof(BN_TYPE) * len);
	memcpy(new_num.digits, src, sizeof(BN_TYPE) * len);  // Copy digits from original
	return new_num;
}

bn_t bn_move_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src){
	size_t min = dlen < slen ? dlen : slen;
	memcpy(dest, src, sizeof(BN_TYPE) * min);  // Copy digits
	
	// Fill in any extension digits needed
	if(dlen > min){
		int extend = bn_isneg_sp(slen, src) ? 0xff : 0x00;
		memset(dest + min, extend, sizeof(BN_TYPE) * (dlen - min));
	}
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_set_sp(size_t len, BN_TYPE *digs, BN_TYPE value){
	*digs = value;  // Set lowest value
	
	// Fill in extension digits if needed
	if(len > 1){
		int extend = BN_TOP & value ? 0xff : 0x00;
		memset(digs + 1, extend, sizeof(BN_TYPE) * (len - 1));
	}
	
	struct bn_s num = {len, digs};
	return num;
}

void bn_free_sp(size_t len, BN_TYPE *src){
	free(src);
}



int bn_iszero_sp(size_t len, const BN_TYPE *num){
	for(size_t i = 0; i < len; i++) if(num[i]) return 0;
	return 1;
}

int bn_cmp_sp(size_t len1, const BN_TYPE *num1, size_t len2, const BN_TYPE *num2){
	int neg;
	if((neg = bn_isneg_sp(len1, num1)) != bn_isneg_sp(len2, num2)){
		return neg ? -1 : 1;
	}
	neg = neg ? -1 : 1;  // Replace the sign with the parity
	
	size_t max = len1 < len2 ? len2 : len1;
	for(size_t i = max - 1; i >= 0; i--){
		BN_TYPE dig1, dig2;
		dig1 = get_digit_sp(len1, num1, i);
		dig2 = get_digit_sp(len2, num2, i);
		
		// Return ordering depending on what the sign of num1 and num2 are
		if(dig1 < dig2) return -neg;
		if(dig1 > dig2) return neg;
	}
	
	return 0;
}



bn_t bn_not_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src){
	// Apply a bitwise not to every digit
	for(size_t i = 0; i < dlen; i++) dest[i] = ~(BN_TYPE)get_digit_sp(slen, src, i);
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_and_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2){
	for(size_t i = 0; i < dlen; i++){
		dest[i] = (BN_TYPE)get_digit_sp(slen1, src1, i) & (BN_TYPE)get_digit_sp(slen2, src2, i);
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_or_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2){
	for(size_t i = 0; i < dlen; i++){
		dest[i] = (BN_TYPE)get_digit_sp(slen1, src1, i) | (BN_TYPE)get_digit_sp(slen2, src2, i);
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_xor_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2){
	for(size_t i = 0; i < dlen; i++){
		dest[i] = (BN_TYPE)get_digit_sp(slen1, src1, i) ^ (BN_TYPE)get_digit_sp(slen2, src2, i);
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_shl_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src, int shift){
	// Positive shift = Left bit shift
	if(shift > 0){
		// Portion of shift that requires bit manipulation
		int remd = shift % (sizeof(BN_TYPE) * 8);
		int cremd = 8 * sizeof(BN_TYPE) - remd;  // Complement of bit shift
		shift /= 8 * sizeof(BN_TYPE);
		for(size_t i = dlen - 1; i >= 0; i--){
			// Use lower digit if available
			if(i >= shift) dest[i] = src[i - shift] << remd;
			else dest[i] = 0;  // Set to zero if none remaining
			
			// Also include even lower bits if available
			if(i >= shift + 1) dest[i] |= src[i - shift - 1] >> cremd;
		}
		
	// Negative shift = Right bit shift (with sign extension)
	}else if(shift < 0){
		shift = -shift;  // Make shift positive
		
		// Sign extension digit used to represent digits above len
		BN_TYPE extend = bn_isneg_sp(slen, src) ? BN_ALL_ONES : 0;
		
		// Portion of shift that requires bit manipulation
		int remd = shift % (sizeof(BN_TYPE) * 8);
		int cremd = 8 * sizeof(BN_TYPE) - remd; // Complement of bit shift
		// Portion of shift that requires moving digits
		shift /= 8 * sizeof(BN_TYPE);
		
		for(size_t i = 0; i < dlen; i++){
			if(i + shift < dlen) dest[i] = src[i + shift] >> remd;
			else dest[i] = extend >> remd;  // Use sign extension digit
			
			// Include even higher bits if available
			if(i + shift + 1 < dlen) dest[i] |= src[i + shift + 1] << cremd;
			else dest[i] |= extend << cremd;
		}
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}



bn_t bn_neg_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src){
	BN_CALC_TYPE calc = 1;
	for(size_t i = 0; i < dlen; i++){
		calc += (BN_CALC_TYPE)~(BN_TYPE)get_digit_sp(slen, src, i);
		dest[i] = calc_lower(calc);  // Set new digit
		calc = calc_upper(calc);  // Shift upper part down
	}
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_addc_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2, BN_TYPE carry){
	BN_CALC_TYPE calc = carry;
	
	// Iterate through digits of `dest`
	for(size_t i = 0; i < dlen; i++){
		calc += (BN_CALC_TYPE)get_digit_sp(slen1, src1, i);
		calc += (BN_CALC_TYPE)get_digit_sp(slen2, src2, i);
		
		// Use the lower part of `calc` as new digit
		dest[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_subc_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2, BN_TYPE carry){
	BN_CALC_TYPE calc = 2 + (BN_CALC_TYPE)~(BN_TYPE)carry;
	
	// Iterate through digits of `dest`
	for(size_t i = 0; i < dlen; i++){
		calc += (BN_CALC_TYPE)get_digit_sp(slen1, src1, i);
		calc += (BN_CALC_TYPE)~(BN_TYPE)get_digit_sp(slen2, src2, i);
		
		// Use the lower part of `calc` as new digit
		dest[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
		calc += BN_ALL_ONES;  // Add the sign extension bits for `carry`
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}



bn_t bn_muli_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src, BN_TYPE scale){
	BN_CALC_TYPE calc = 0;
	
	// Iterate through digits
	for(size_t i = 0; i < dlen; i++){
		calc += ((BN_CALC_TYPE)scale) * (BN_CALC_TYPE)get_digit_sp(slen, src, i);
		
		// Use the lower part of `calc` as new digit
		dest[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
	}
	
	struct bn_s num = {dlen, dest};
	return num;
}

bn_t bn_mul_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2){
	// `dest` and `num1` / `num2` cannot overlap
	if(is_overlap_sp(dlen, dest, slen1, src1) || is_overlap_sp(dlen, dest, slen2, src2)){
		struct bn_s num = {0, NULL};
		return num;
	}
	
	// Iterate through each digit of num1
	for(size_t i = 0; i < dlen; i++){
		BN_CALC_TYPE calc = 0;
		BN_CALC_TYPE scale = get_digit_sp(slen1, src1, i);
		
		BN_TYPE *dg = dest + i;
		for(size_t j = 0; i + j < dlen; j++, dg++){
			calc += *dg;
			calc += scale * (BN_CALC_TYPE)get_digit_sp(slen2, src2, j);
			
			*dg = calc_lower(calc);  // Set new digit
			calc = calc_upper(calc);  // Move upper part down
		}
	}
	
	struct bn_s dest_num = {dlen, dest};
	return dest_num;
}



bn_t bn_divi_sp(size_t qlen, BN_TYPE *quot, BN_TYPE *remd, size_t slen, BN_TYPE *src, BN_TYPE divis){
	// Represents the remainder from the higher order digits
	BN_CALC_TYPE calc = 0;
	
	for(int i = qlen - 1; i >= 0; i--){
		// Include the current digit
		calc = (calc << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)get_digit_sp(slen, src, i);
		
		BN_CALC_TYPE div = (BN_TYPE)(calc / (BN_CALC_TYPE)divis);
		set_digit_sp(qlen, quot, i, div);  // New digit is quotient
		// Store remainder back into calc
		calc -= ((BN_CALC_TYPE)divis) * div;
	}
	
	// Store remainder into pointer if given
	if(remd) *remd = (BN_TYPE)calc;
	
	struct bn_s num = {qlen, quot};
	return num;
}

// Get the number of leading zeros
static inline int clz(BN_TYPE dig){
	int count;
#ifdef __GNUC__ // If GNU compiler then use builtins
	count = 8 * sizeof(unsigned long long) - __builtin_clzll((unsigned long long)dig);
#else
	// If no builtins available use loop instead
	for(count = 0; dig; dig >>= 1, count++);
#endif
	return 8 * sizeof(BN_TYPE) - count;
}

bn_t bn_div_sp(size_t qlen, BN_TYPE *quot, size_t rlen, BN_TYPE *remd, size_t slen, const BN_TYPE *src, size_t dvlen, const BN_TYPE *divis){
	struct bn_s num = {0, NULL};
	if(
		qlen < slen ||  // `quot` must have enough space for `src` (dividend)
		rlen < dvlen ||  // Remainder may be as large as divisor so `remd` must be at least as big as `divis`
		is_overlap_sp(qlen, quot, dvlen, divis)  // `quot` is used during calculation cannot overlap with `divis`
	) return num;
	
	size_t lead_idx = dvlen - 1;  // Index of leading digit of divis
	while(lead_idx > 0 && divis[lead_idx] == 0) lead_idx--;
	BN_CALC_TYPE lead_dig = divis[lead_idx];  // Leading digit of divis

	
	// If divisor is zero leave
	if(lead_dig == 0) return num;
	
	// Figure out how much we can shift `lead_dig` left by
	int shift = clz(lead_dig);
	// Shift lead_dig and add bit from lower digit
	lead_dig <<= shift;
	if(lead_idx > 0) lead_dig |= divis[lead_idx - 1] >> (8 * sizeof(BN_TYPE) - shift);
	lead_dig++;  // Add one to compensate for any lower bits that weren't included
	
	// Move source into quotient
	bn_move_sp(qlen, quot, slen, src);
	
	// Main Loop: Subtracting multiples of `divis` from `quot`
	BN_CALC_TYPE calc = 0;  // Intermediate value for calculations
	BN_TYPE top = 0;  // Store the digit immediately higher than the ith
	for(size_t i = qlen - 1; i >= dvlen - 1; i--){
		// Have `calc` store the top two digits of the remaining value
		calc = (((BN_CALC_TYPE)top) << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)(quot[i]);
		
		// Calculate multiple of `divis` that fits in `digs`
		// This is an underestimate (of at most 2) of the maximum possible multiple
		BN_CALC_TYPE div = (calc << shift) / lead_dig;
		BN_TYPE new_quot = 0;  // Store the new digit of the quotient
		
		// Remove multiple of `divis` from `quot`
		// Starting from lowest digit of `divis`
		while(calc >= divis[lead_idx]){  // This needs to be looped since `div` could be 2 less than what is needed
			// The loop below performs the operation `quot -= div * divis`
			
			// Track carries for the subtraction and multiplication separately
			BN_CALC_TYPE sub_carry = 1, dv_carry = 0;
			BN_TYPE *qd = quot + i - dvlen + 1;  // Pointer to Digit in the quotient to manipulate
			if(div > 0) for(size_t j = 0; j < dvlen; j++, qd++){
				// Add scaled divis to the carry from the previous digit's multiplication
				dv_carry += div * divis[j];
				
				// Subtract and set quotient digit
				sub_carry += *qd;
				sub_carry += ~(BN_TYPE)calc_lower(dv_carry);
				*qd = calc_lower(sub_carry);
				
				// Move carry trackers down
				sub_carry = calc_upper(sub_carry);
				dv_carry = calc_upper(dv_carry);
			}
			// Recalculate top two bytes given reduction
			sub_carry += top;
			sub_carry += ~(BN_TYPE)dv_carry;
			top = (BN_TYPE)calc_lower(sub_carry);
			calc = ((BN_CALC_TYPE)top << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)(quot[i]);
			
			// Add `div` into the new digit of the quotient
			new_quot += div;
			
			// After first loop iteration div need only be 1
			// For subsequent loops
			div = 1;
		}
		
		// Place remainder in highest digit into `top`
		top = quot[i];
		// Update new digit of quotient
		quot[i] = new_quot;
	}
	
	// Move remaining value from `quot` into `remd`
	bn_move_sp(rlen, remd, dvlen - 1, quot);
	// Place remainder from `top` into most significant digit of `remd`
	remd[dvlen - 1] = top;
	
	// Move the digits of `quot` down covering the old remainder values
	memmove(quot, quot + dvlen - 1, sizeof(BN_TYPE) * (qlen - dvlen + 1));
	// Clear out the old quotient digits left after the move
	memset(quot + qlen - dvlen + 1, 0x00, sizeof(BN_TYPE) * (dvlen - 1));
	
	num.length = qlen;
	num.digits = quot;
	return num;
}



// Convert big number to string
// If `size == 0` then no limit is imposed
// If there is insufficient space the *least significant* n digits are printed
int bn_tostrn_sp(char *str, size_t n, size_t len, BN_TYPE *num){
	int count = 0;  // Count number of characters printed
	// Start at 1 to account for null at the end
	
	// Check for sign
	char *cap = str + n;
	if(bn_isneg_sp(len, num)){
		// Add negative sign
		count++;  *(str++) = '-';
		
		// And undo negation on num
		bn_nega_sp(len, num);
	}
	
	char *start = str;  // Store original of string
	while(!bn_iszero_sp(len, num) && (!n || str < cap)){
		// Digits are calculated in little endian fashion
		// But are printed in blocks in big endian
		BN_TYPE remd = 0;
		bn_divai_sp(len, num, &remd, BN_TENPOW);
		
		// Print the remainder to get the digits
		int new_count;
		for(size_t i = BN_TENPOW_LEN; i > 0 && (!n || str < cap); i--, str++, count++){
			*str = (remd % 10) + '0';
			remd /= 10;
		}
	}
	
	// Move str backward to remove leading zeros
	while(str > start && *(str - 1) == '0'){
		str--;  count--;
	}
	*str = '\0';  count++;  // Place null character
	
	// Reverse all of the digits back to big endian
	char *first = start, *last = str - 1;
	for(; first < last; first++, last--){
		char tmp = *first;
		*first = *last;
		*last = tmp;
	}
	
	return count;
}

// Calculate big number from string
bn_t bn_frmstrn_sp(size_t len, BN_TYPE *digs, const char *str, size_t n){
	// Set digs to zero
	memset(digs, 0, sizeof(BN_TYPE) * len);
	// Mark end of string
	const char *cap = str + n;
	
	// Check for negative at beginning
	int neg = 0;
	if(*str == '-'){
		neg = 1;
		str++;
	}
	
	while(isdigit(*str) && str < cap){
		// Accumulate a single BN_TYPE digit
		BN_TYPE accm = 0, tenpow = 1;
		for(size_t i = BN_TENPOW_LEN; i > 0 && isdigit(*str) && str < cap; str++, i--){
			accm *= 10;  tenpow *= 10;
			accm += (*str) - '0';
		}
		
		// Place digit into `digs`
		bn_mulai_sp(len, digs, tenpow);
		bn_addai_sp(len, digs, accm);
	}
	
	// Negate if number is negative
	if(neg) bn_nega_sp(len, digs);
	
	struct bn_s num = {len, digs};
	return num;
}

bn_t bn_new_frmstrn(const char *str, size_t n){
	const char *cap = str + n, *tmp = str;
	if(*tmp == '-') tmp++;  // Skip leading negative
	
	// Calculate length necessary to store number
	size_t len = 0;
	while(isdigit(*tmp) && tmp < cap) tmp++, len++;
	
	// Multiply len by approximation of log_2 (10) to get number of bits
	len *= 2136;
	len /= 643;
	// Divide number of bits by `8 * sizeof(BN_TYPE)` to get number of digits
	len /= 8 * sizeof(BN_TYPE);
	len++;
	
	// Allocate space for digits
	BN_TYPE *digs = malloc(sizeof(BN_TYPE) * len);
	return bn_frmstrn_sp(len, digs, str, n);
}


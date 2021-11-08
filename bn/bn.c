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
#define bn_isneg(num) ((num).digits && (num).length > 0 && ((num).digits[(num).length - 1] & BN_TOP))


// Access the nth digit of a big number
// If greater than the maximum digit use sign extension
#define get_digit(num, i) ((BN_TYPE) ((i) < (num).length ? (num).digits[i] : (bn_isneg(num) ? BN_ALL_ONES : 0)))
#define set_digit(num, i, val) if((i) < (num).length) (num).digits[i] = (BN_TYPE)(val)

// Calculate extension digit for a digit
#define extend(dig) (BN_TOP & (dig) ? BN_ALL_ONES : 0)

// Check if two nbers digits overlap in memory
#define is_overlap(n1, n2) ((n1).digits < (n2).digits ? (n1).digits + (n1).length > (n2).digits : (n2).digits + (n2).length > (n1).digits)



bn_t bn_new(size_t len, BN_SIGNED value){
	struct bn_s num;
	num.length = len;
	num.digits = malloc(sizeof(BN_TYPE) * len);
	// Set the value of the number to `value`
	return bn_set(num, value);
}

bn_t bn_copy(const bn_t src){
	struct bn_s new_num;
	new_num.length = src.length;
	new_num.digits = malloc(sizeof(BN_TYPE) * src.length);
	
	// Copy digits from original
	memcpy(new_num.digits, src.digits, sizeof(BN_TYPE) * src.length);
	return new_num;
}

bn_t bn_move(bn_t dest, const bn_t src){
	// Don't move anything if `src` and `dest` are the same
	if(src.digits != dest.digits){
		size_t min = dest.length < src.length ? dest.length : src.length;
		memmove(dest.digits, src.digits, sizeof(BN_TYPE) * min);  // Copy digits
	}
	
	// Fill in any extension digits needed
	if(dest.length > src.length){
		int extend = bn_isneg(src) ? 0xff : 0x00;
		memset(dest.digits + src.length, extend, sizeof(BN_TYPE) * (dest.length - src.length));
	}
	
	return dest;
}

bn_t bn_set(bn_t dest, BN_SIGNED value){
	dest.digits[0] = (BN_TYPE)value;  // Set lowest value
	
	// Fill in extension digits if needed
	if(dest.length > 1){
		int extend = BN_TOP & value ? 0xff : 0x00;
		memset(dest.digits + 1, extend, sizeof(BN_TYPE) * (dest.length - 1));
	}
	
	return dest;
}

void bn_free(bn_t num){
	free(num.digits);
}



int bn_iszero(const bn_t num){
	for(size_t i = 0; i < num.length; i++) if(num.digits[i]) return 0;
	return 1;
}

int bn_cmp(const bn_t num1, const bn_t num2){
	int neg;
	if((neg = bn_isneg(num1)) != bn_isneg(num2)){
		return neg ? -1 : 1;
	}
	neg = neg ? -1 : 1;  // Replace the sign with the parity
	
	size_t max = num1.length < num2.length ? num2.length : num1.length;
	for(size_t i = max - 1; i >= 0; i--){
		BN_TYPE dig1, dig2;
		dig1 = get_digit(num1, i);
		dig2 = get_digit(num2, i);
		
		// Return ordering depending on what the sign of num1 and num2 are
		if(dig1 < dig2) return -neg;
		if(dig1 > dig2) return neg;
	}
	
	return 0;
}



bn_t bn_not(bn_t dest, const bn_t src){
	// Apply a bitwise not to every digit
	for(size_t i = 0; i < dest.length; i++) dest.digits[i] = ~get_digit(src, i);
	return dest;
}

bn_t bn_and(bn_t dest, const bn_t src1, const bn_t src2){
	for(size_t i = 0; i < dest.length; i++) dest.digits[i] = get_digit(src1, i) & get_digit(src2, i);
	return dest;
}

bn_t bn_or(bn_t dest, const bn_t src1, const bn_t src2){
	for(size_t i = 0; i < dest.length; i++) dest.digits[i] = get_digit(src1, i) | get_digit(src2, i);
	return dest;
}

bn_t bn_xor(bn_t dest, const bn_t src1, const bn_t src2){
	for(size_t i = 0; i < dest.length; i++) dest.digits[i] = get_digit(src1, i) ^ get_digit(src2, i);
	return dest;
}


bn_t bn_shl(bn_t dest, const bn_t src, int shift){
	// Positive shift = Left bit shift
	if(shift > 0){
		// Portion of shift that requires bit manipulation
		int remd = shift % (sizeof(BN_TYPE) * 8);
		int cremd = 8 * sizeof(BN_TYPE) - remd;  // Complement of bit shift
		shift /= 8 * sizeof(BN_TYPE);
		
		// Iterate downwards to avoid overwriting digits before use
		for(int i = dest.length - 1; i >= 0; i--){
			// Use lower digit if available
			if(i >= shift) dest.digits[i] = get_digit(src, i - shift) << remd;
			else dest.digits[i] = 0;  // Set to zero if none remaining
			
			// Also include even lower bits if available
			if(i >= shift + 1) dest.digits[i] |= get_digit(src, i - shift - 1) >> cremd;
		}
		
	// Negative shift = Right bit shift (with sign extension)
	}else if(shift < 0){
		// Store sign extension for `src`
		BN_TYPE ext = extend(src.digits[src.length - 1]);
		
		shift = -shift;  // Make shift positive
		
		// Portion of shift that requires bit manipulation
		int remd = shift % (sizeof(BN_TYPE) * 8);
		int cremd = 8 * sizeof(BN_TYPE) - remd; // Complement of bit shift
		// Portion of shift that requires moving digits
		shift /= 8 * sizeof(BN_TYPE);
		
		// Iterate upwards to avoid overwriting digits before use
		for(size_t i = 0; i < dest.length; i++){
			if(i + shift < src.length) dest.digits[i] = src.digits[i + shift] >> remd;
			else dest.digits[i] = ext >> remd;
			
			// Include even higher bits if available
			if(i + shift + 1 < src.length) dest.digits[i] |= src.digits[i + shift + 1] << cremd;
			else dest.digits[i] |= ext << cremd;
		}
	}
	
	return dest;
}



bn_t bn_neg(bn_t dest, const bn_t src){
	BN_CALC_TYPE calc = 1;
	for(size_t i = 0; i < dest.length; i++){
		calc += (BN_CALC_TYPE)~get_digit(src, i);
		
		dest.digits[i] = calc_lower(calc);  // Set new digit
		calc = calc_upper(calc);  // Shift upper part down
	}
	return dest;
}

bn_t bn_addi(bn_t dest, const bn_t src, BN_SIGNED shift){
	BN_CALC_TYPE calc = (BN_TYPE)shift, ext = extend(shift);
	int neg = !!(BN_TOP & shift);  // Store sign of shift
	
	// Move `src` into `dest`
	bn_move(dest, src);
	
	BN_TYPE *dg = dest.digits, *end = dg + dest.length;
	for(; dg < end; dg++){
		calc += *dg;
		// Move `calc` down
		*dg = (BN_TYPE)calc_lower(calc);
		calc = calc_upper(calc);
		
		// Leave if no more carries are necessary
		if(calc == neg) break;
		calc += ext;  // Add extension digit for `shift`
	}
	return dest;
}

bn_t bn_addc(bn_t dest, const bn_t src1, const bn_t src2, BN_SIGNED carry){
	BN_CALC_TYPE calc = (BN_TYPE)carry;
	
	// Sign extension for carry
	BN_CALC_TYPE ext = extend(carry);
	
	// Iterate through digits of `dest`
	for(size_t i = 0; i < dest.length; i++){
		calc += (BN_CALC_TYPE)get_digit(src1, i);
		calc += (BN_CALC_TYPE)get_digit(src2, i);
		
		// Use the lower part of `calc` as new digit
		dest.digits[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
		
		// Include sign extension for `carry`
		calc += ext;
	}
	
	return dest;
}

bn_t bn_subc(bn_t dest, const bn_t src1, const bn_t src2, BN_SIGNED carry){
	BN_CALC_TYPE calc = 2 + (BN_CALC_TYPE)~(BN_TYPE)carry;
	
	// Sign extension for carry
	BN_CALC_TYPE ext = extend(~carry);
	
	// Iterate through digits of `dest`
	for(size_t i = 0; i < dest.length; i++){
		calc += (BN_CALC_TYPE)get_digit(src1, i);
		calc += (BN_CALC_TYPE)~get_digit(src2, i);
		
		// Use the lower part of `calc` as new digit
		dest.digits[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
		
		// Include sign extension for `carry`
		calc += ext;
	}
	
	return dest;
}



bn_t bn_muli(bn_t dest, const bn_t src, BN_SIGNED scale){
	BN_CALC_TYPE calc = 0, neg = 0;
	// If scale is negative flip it
	// And negate `src` as you go
	if(scale < 0){
		scale = -scale;
		// Serves as +1 necessary after bitwise not in order to make `src` negative
		calc = (BN_TYPE)scale;
		neg = BN_ALL_ONES;
	}
	
	// Iterate through digits
	for(size_t i = 0; i < dest.length; i++){
		calc += ((BN_CALC_TYPE)scale) * (neg ^ get_digit(src, i));
		
		// Use the lower part of `calc` as new digit
		dest.digits[i] = calc_lower(calc);
		// Shift remainder of `calc` down
		calc = calc_upper(calc);
	}
	
	return dest;
}

bn_t bn_mul(bn_t dest, const bn_t src1, const bn_t src2){
	// `dest` and `src1` / `src2` cannot overlap
	if(is_overlap(dest, src1) || is_overlap(dest, src2)){
		struct bn_s num = {0, NULL};
		return num;
	}
	
	// Iterate through each digit of num1
	for(size_t i = 0; i < dest.length; i++){
		BN_CALC_TYPE calc = 0, scale = get_digit(src1, i);
		
		BN_TYPE *dg = dest.digits + i;
		for(size_t j = 0; i + j < dest.length; j++, dg++){
			calc += *dg;
			calc += scale * (BN_CALC_TYPE)get_digit(src2, j);
			
			*dg = calc_lower(calc);  // Set new digit
			calc = calc_upper(calc);  // Move upper part down
		}
	}
	
	return dest;
}



bn_t bn_divi(bn_t quot, BN_SIGNED *remd, const bn_t src, BN_SIGNED divis){
	// Quotient must be big enough to hold `src`
	if(quot.length < src.length){
		struct bn_s num = {0, NULL};
		return num;
	}
	
	// If `divis` is negative flip it
	// And negate afterwards
	int dvneg = divis < 0;
	if(dvneg) divis = -divis;
	
	// If `src` is negative flip it
	// And negate afterwards
	int sneg = bn_isneg(src);
	if(sneg) bn_neg(quot, src);
	// Otherwise put `src` into `quot`
	else bn_move(quot, src);
	
	// Represents the remainder from the higher order digits
	BN_CALC_TYPE calc = 0;
	
	for(int i = quot.length - 1; i >= 0; i--){
		// Include the current digit
		calc = (calc << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)quot.digits[i];
		
		BN_TYPE div = (BN_TYPE)(calc / (BN_CALC_TYPE)divis);
		quot.digits[i] = div;  // New digit is quotient
		// Remove multiple of divis to get remainder
		calc -= ((BN_CALC_TYPE)divis) * div;
	}
	
	// Negate the quotient if necessary
	if(sneg ^ dvneg){
		// Bit-wise not quotient `quot` -> `-quot - 1`
		for(size_t i = 0; i < quot.length; i++) quot.digits[i] ^= BN_ALL_ONES;
		calc -= divis;  // Swap remainder
	}
	
	// Store remainder into pointer if given
	if(remd) *remd = (sneg ? -1 : 1) * (BN_SIGNED)calc;
	
	return quot;
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

bn_t bn_div(bn_t quot, bn_t remd, const bn_t src, const bn_t divis){
	struct bn_s num = {0, NULL};
	if(
		// `quot` must have enough space for `src` (dividend)
		quot.length < src.length ||
		// Remainder may be as large as divisor so `remd` must be at least as big as `divis`
		remd.length < divis.length ||
		// `quot` is used during calculation cannot overlap with `divis`
		is_overlap(quot, divis)
	) return num;
	
	size_t lead_idx = divis.length - 1;  // Index of leading digit of divis
	while(lead_idx > 0 && divis.digits[lead_idx] == 0) lead_idx--;
	BN_CALC_TYPE lead_dig = divis.digits[lead_idx];  // Leading digit of divis
	
	
	// If divisor is zero leave
	if(lead_dig == 0) return num;
	
	// Figure out how much we can shift `lead_dig` left by
	int shift = clz(lead_dig);
	// Shift lead_dig and add bits from lower digit
	lead_dig <<= shift;
	if(lead_idx > 0) lead_dig |= divis.digits[lead_idx - 1] >> (8 * sizeof(BN_TYPE) - shift);
	lead_dig++;  // Add one to compensate for any lower bits that weren't included
	
	// Move source into quotient
	bn_move(quot, src);
	
	// Main Loop: Subtracting multiples of `divis` from `quot`
	BN_CALC_TYPE calc = 0;  // Intermediate value for calculations
	BN_TYPE top = 0;  // Store the digit immediately higher than the ith
	for(size_t i = quot.length - 1; i >= divis.length - 1; i--){
		// Have `calc` store the top two digits of the remaining value
		calc = (((BN_CALC_TYPE)top) << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)(quot.digits[i]);
		
		// Calculate multiple of `divis` that fits in `digs`
		// This is an underestimate (of at most 2) of the maximum possible multiple
		BN_CALC_TYPE div = (calc << shift) / lead_dig;
		BN_TYPE new_quot = 0;  // Store the new digit of the quotient
		
		// This needs to be looped since `div` could be 2 less than what is needed
		while(calc >= divis.digits[lead_idx]){
			// The loop below performs the operation `quot -= div * divis`
			
			// Remove multiple of `divis` from `quot`
			// Starting from lowest digit of `divis`
			
			// Track carries for the subtraction and multiplication separately
			BN_CALC_TYPE sub_carry = 1, dv_carry = 0;
			BN_TYPE *qd = quot.digits + i - divis.length + 1;  // Pointer to Digit in the quotient to manipulate
			if(div > 0) for(size_t j = 0; j < divis.length; j++, qd++){
				// Add scaled divis to the carry from the previous digit's multiplication
				dv_carry += div * divis.digits[j];
				
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
			calc = ((BN_CALC_TYPE)top << (8 * sizeof(BN_TYPE))) | (BN_CALC_TYPE)(quot.digits[i]);
			
			// Add `div` into the new digit of the quotient
			new_quot += div;
			
			// After first loop iteration div need only be 1
			// For subsequent loops
			div = 1;
		}
		
		// Place remainder in highest digit into `top`
		top = quot.digits[i];
		// Update new digit of quotient
		quot.digits[i] = new_quot;
	}
	
	// Move remaining value from `quot` into `remd`
	memmove(remd.digits, quot.digits, sizeof(BN_TYPE) * (divis.length - 1));
	// Place remainder from `top` into most significant digit of `remd`
	remd.digits[divis.length - 1] = top;
	// Zero out remainder of `remd`
	memset(remd.digits, 0x00, remd.length - divis.length);
	
	// Move the digits of `quot` down covering the old remainder values
	memmove(
		quot.digits,
		quot.digits + divis.length - 1,
		sizeof(BN_TYPE) * (quot.length - divis.length + 1)
	);
	// Clear out the old quotient digits left after the move
	memset(
		quot.digits + quot.length - divis.length + 1,
		0x00,
		sizeof(BN_TYPE) * (divis.length - 1)
	);
	
	return quot;
}



// Convert big number to string
// If `size == 0` then no limit is imposed
// If there is insufficient space the *least significant* n digits are printed
int bn_tostrn(char *str, size_t n, const bn_t src){
	int count = 0;  // Count number of characters printed
	// Start at 1 to account for null at the end
	
	// Create temporary big number to manipulate while printing
	BN_TYPE digits[src.length];
	struct bn_s num = {src.length, digits};
	memcpy(digits, src.digits, sizeof(BN_TYPE) * src.length);
	
	// Check for sign
	char *cap = str + n;
	if(bn_isneg(num)){
		// Add negative sign
		count++;  *(str++) = '-';
		
		// And undo negation on num
		bn_nega(num);
	}
	
	char *start = str;  // Store original of string
	while(!bn_iszero(num) && (!n || str < cap)){
		// Digits are calculated in little endian fashion
		// But are printed in blocks in big endian
		BN_SIGNED remd = 0;
		bn_divai(num, &remd, BN_TENPOW);
		
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
bn_t bn_frmstrn(bn_t dest, const char *str, size_t n){
	// Set digs to zero
	memset(dest.digits, 0, sizeof(BN_TYPE) * dest.length);
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
		bn_mulai(dest, tenpow);
		bn_addai(dest, accm);
	}
	
	// Negate if number is negative
	if(neg) bn_nega(dest);
	
	return dest;
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
	struct bn_s num = {len, digs};
	return bn_frmstrn(num, str, n);
}


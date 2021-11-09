#ifndef __BN_H
#define __BN_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef BN_TYPE_SIZE
// Minimum size in bytes of the representation to use for the digits
#define BN_TYPE_SIZE 4
#endif

// Constants used to convert big number to and from strings
#if BN_TYPE_SIZE >= 4
	// Type used to represent digits in the big number
	#define BN_TYPE uint32_t
	// Signed version of BN_TYPE
	#define BN_SIGNED int32_t
	// Type used to perform intermediate calculations
	#define BN_CALC_TYPE uint64_t
	
	// Greatest power of ten less than maximum of BN_TYPE
	#define BN_TENPOW (BN_TYPE)(1000000000)
	// Exponent of greatest power of ten
	#define BN_TENPOW_LEN (9)
#elif BN_TYPE_SIZE >= 2
	#define BN_TYPE uint16_t
	#define BN_SIGNED int16_t
	#define BN_CALC_TYPE uint32_t
	#define BN_TENPOW (BN_TYPE)(10000)
	#define BN_TENPOW_LEN (4)
#elif BN_TYPE_SIZE >= 1
	#define BN_TYPE uint8_t
	#define BN_SIGNED int8_t
	#define BN_CALC_TYPE uint16_t
	#define BN_TENPOW (BN_TYPE)(100)
	#define BN_TENPOW_LEN (2)
#endif



typedef struct bn_s {
	size_t length;
	BN_TYPE *digits;
} bn_t;

/* Variants:
 * The standard form of each function takes `bn_t` as arguments
 * with the location to store the result as the first argument
 * and the subsequent arguments as the inputs.
 * 
 * The 'a' suffix indicates that the location to store the result
 * is the same as the first argument to the operation.
 * This is not available for some operations due to their need
 * to preserve the arguments while calculating.
 * 
 * The 'i' suffix indicates that the function takes an argument
 * of type BN_TYPE instead of full list of digits.
 * These variants avoid having to construct a bignum to hold a small number.
 * 
 * The 'c' suffix indicates that the function takes both
 * a BN_TYPE argument as well as a Big Number.
 * This allows for standard operation and an 'i' variant to be done at once.
 */

/* Const qualifier:
 * The `const` qualifier is applied to several of the `bn_t` parameters below.
 * Although this does not directly prevent modification to the digits,
 * it is used to indicate that the function does not
 * modify any of the elements of the digits field.
 */


// Allocation and Deallocation of Big Numbers
bn_t bn_new(size_t len, BN_SIGNED value);
bn_t bn_copy(const bn_t src);
bn_t bn_move(bn_t dest, const bn_t src);
bn_t bn_set(bn_t dest, BN_SIGNED value);
void bn_free(bn_t num);


// Comparison on Big Numbers
int bn_iszero(const bn_t num);
int bn_cmp(const bn_t num1, const bn_t num2);

// Bit-wise Operations on Big Numbers
bn_t bn_not(bn_t dest, const bn_t src);
#define bn_nota(num) bn_not(num, num)
bn_t bn_and(bn_t dest, const bn_t src1, const bn_t src2);
#define bn_anda(dest, src) bn_and(dest, dest, src)
bn_t bn_or(bn_t dest, const bn_t src1, const bn_t src2);
#define bn_ora(dest, src) bn_or(dest, dest, src)
bn_t bn_xor(bn_t dest, const bn_t src1, const bn_t src2);
#define bn_xora(dest, src) bn_xor(dest, dest, src)

// Shift Left (`shift` > 0)
// Shift Right (`shift` < 0)
bn_t bn_shl(bn_t dest, const bn_t src, int shift);
#define bn_shla(dest, shift) bn_shl(dest, dest, shift)
#define bn_shr(dest, src, shift) bn_shl(dest, src, -(shift))
#define bn_shra(dest, shift) bn_shr(dest, dest, shift)

// Negation of Big Numbers
bn_t bn_neg(bn_t dest, const bn_t src);
#define bn_nega(dest) bn_neg(dest, dest)

// Addition of Big Numbers
bn_t bn_addi(bn_t dest, const bn_t src, BN_SIGNED shift);
#define bn_addai(dest, shift) bn_addi(dest, dest, shift)
bn_t bn_addc(bn_t dest, const bn_t src1, const bn_t src2, BN_SIGNED carry);
#define bn_addac(dest, src, carry) bn_addc(dest, dest, src, carry)
#define bn_add(dest, src1, src2) bn_addc(dest, src1, src2, 0)
#define bn_adda(dest, src) bn_addc(dest, dest, src, 0)

// Subtraction of Big Numbers
#define bn_subi(dest, src, shift) bn_addi(dest, src, -(shift))
#define bn_subai(dest, shift) bn_subi(dest, dest, shift)
bn_t bn_subc(bn_t dest, const bn_t src1, const bn_t src2, BN_SIGNED carry);
#define bn_subac(dest, src, carry) bn_subc(dest, dest, src, carry)
#define bn_sub(dest, src1, src2) bn_subc(dest, src1, src2, 0)
#define bn_suba(dest, src) bn_subc(dest, dest, src, 0)

// Multiplication of Big Numbers
bn_t bn_muli(bn_t dest, const bn_t src, BN_SIGNED scale);
#define bn_mulai(dest, scale) bn_muli(dest, dest, scale)
bn_t bn_mul(bn_t dest, const bn_t src1, const bn_t src2);

// Division of Big Numbers
bn_t bn_divi(bn_t quot, BN_SIGNED *remd, const bn_t src, BN_SIGNED divis);
#define bn_divai(quot, remd, divis) bn_divi(quot, remd, quot, divis)
bn_t bn_div(bn_t quot, bn_t remd, const bn_t src, const bn_t divis);
#define bn_diva(quot, remd, divis) bn_div(quot, remd, quot, divis)


// Convert Big Number into String
int bn_tostrn(char *str, size_t n, const bn_t num);
#define bn_tostr(str, num) bn_tostrn(str, 0, num)

// Convert String into Big Number
bn_t bn_frmstrn(bn_t num, const char *str, size_t n);
#define bn_frmstr(num, str) bn_frmstrn(num, str, strlen(str))

// Allocate space for new Big Number parsed from String
bn_t bn_new_frmstrn(const char *str, size_t n);
#define bn_new_frmstr(str) bn_new_frmstrn(str, strlen(str))

#endif


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
	// Type used to perform intermediate calculations
	#define BN_CALC_TYPE uint64_t
	
	// Greatest power of ten less than maximum of BN_TYPE
	#define BN_TENPOW (BN_TYPE)(1000000000)
	// Exponent of greatest power of ten
	#define BN_TENPOW_LEN (9)
#elif BN_TYPE_SIZE >= 2
	#define BN_TYPE uint16_t
	#define BN_CALC_TYPE uint32_t
	#define BN_TENPOW (BN_TYPE)(10000)
	#define BN_TENPOW_LEN (4)
#elif BN_TYPE_SIZE >= 1
	#define BN_TYPE uint8_t
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
 * The '_sp' suffix indicates that the function takes a length
 * and an array of digits instead of a `struct bn_s`.
 */

#define bn_split(num) (num).length, (num).digits


bn_t bn_new(size_t len, BN_TYPE value);
bn_t bn_copy_sp(size_t len, const BN_TYPE *src);
#define bn_copy(num) bn_copy(bn_split(num))
bn_t bn_move_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src);
#define bn_move(dest, src) bn_move(bn_split(dest), bn_split(src))
bn_t bn_set_sp(size_t len, BN_TYPE *digs, BN_TYPE value);
#define bn_set(num, value) bn_set(bn_split(num), value)
void bn_free_sp(size_t len, BN_TYPE *src);
#define bn_free(num) bn_free(bn_split(num))


int bn_iszero_sp(size_t len, const BN_TYPE *num);
#define bn_iszero(num) bn_iszero_sp(bn_split(num))
int bn_cmp_sp(size_t len1, const BN_TYPE *num1, size_t len2, const BN_TYPE *num2);
#define bn_cmp(num1, num2) bn_cmp_sp(bn_split(num1), bn_split(num2))

bn_t bn_not_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src);
#define bn_not(dest, src) bn_not_sp(bn_split(dest), bn_split(src))
#define bn_nota(num) bn_not_sp(bn_split(num), bn_split(num))
bn_t bn_and_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2);
#define bn_and(dest, src1, src2) bn_and_sp(bn_split(dest), bn_split(src1), bn_split(src2))
#define bn_anda(dest, src) bn_and_sp(bn_split(dest), bn_split(dest), bn_split(src))
bn_t bn_or_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2);
#define bn_or(dest, src1, src2) bn_or_sp(bn_split(dest), bn_split(src1), bn_split(src2))
#define bn_ora(dest, src) bn_or_sp(bn_split(dest), bn_split(dest), bn_split(src))
bn_t bn_xor_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2);
#define bn_xora_sp(dlen, dest, slen, src) bn_xor_sp(dlen, dest, dlen, dest, slen, src)
#define bn_xor(dest, src1, src2) bn_xor_sp(bn_split(dest), bn_split(src1), bn_split(src2))
#define bn_xora(dest, src) bn_xor_sp(bn_split(dest), bn_split(dest), bn_split(src))

bn_t bn_shl_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src, int shift);
#define bn_shla_sp(dlen, dest, slen, src) bn_shl_sp(dlen, dest, dlen, dest, slen, src)
#define bn_shl(dest, src, shift) bn_shl_sp(bn_split(dest), bn_split(src), shift)
#define bn_shla(num, shift) bn_shl_sp(bn_split(num), bn_split(num), shift)

bn_t bn_neg_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src);
#define bn_nega_sp(len, num) bn_neg_sp(len, num, len, num)
#define bn_neg(dest, src) bn_neg_sp(bn_split(dest), bn_split(src))
#define bn_nega(dest) bn_neg_sp(bn_split(num), bn_split(num))

// Addition of big numbers and all its variants
bn_t bn_addc_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2, BN_TYPE carry);
#define bn_addac_sp(dlen, dest, slen, src, carry) bn_addc_sp(dlen, dest, dlen, dest, slen, src, carry)
#define bn_addi_sp(dlen, dest, slen, src, shift) bn_addc_sp(dlen, dest, slen, src, 0, NULL, shift)
#define bn_addai_sp(len, num, shift) bn_addc_sp(len, num, len, num, 0, NULL, shift)
#define bn_add_sp(dlen, dest, slen1, src1, slen2, src2) bn_addc_sp(dlen, dest, slen1, src1, slen2, src2, 0)
#define bn_adda_sp(dlen, dest, slen, src) bn_addc_sp(dlen, dest, dlen, dest, slen, src, 0)

#define bn_addc(dest, src1, src2, carry) bn_addc_sp(bn_split(dest), bn_split(src1), bn_split(src2), carry)
#define bn_addac(dest, src, carry) bn_addc_sp(bn_split(dest), bn_split(dest), bn_split(src), carry)
#define bn_addi(dest, src, shift) bn_addc_sp(bn_split(dest), bn_split(src), 0, NULL, shift)
#define bn_addai(dest, shift) bn_addc_sp(bn_split(dest), bn_split(dest), 0, NULL, shift)
#define bn_add(dest, src1, src2) bn_add_sp(bn_split(dest), bn_split(src1), bn_split(src2), 0)
#define bn_adda(dest, src) bn_add_sp(bn_split(dest), bn_split(dest), bn_split(src), 0)


// Subtraction of big numbers and all its variants
bn_t bn_subc_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2, BN_TYPE carry);
#define bn_subac_sp(dlen, dest, slen, src, shift) bn_subc_sp(dlen, dest, dlen, dest, slen, src, shift)
#define bn_subi_sp(dlen, dest, slen, src, shift) bn_subc_sp(dlen, dest, slen, src, 0, NULL, shift)
#define bn_subai_sp(dlen, dest, shift) bn_subc_sp(dlen, dest, dlen, dest, 0, NULL, shift)
#define bn_sub_sp(dlen, dest, slen1, src1, slen2, src2) bn_subc_sp(dlen, dest, slen1, src1, slen2, src2, 0)
#define bn_suba_sp(dlen, dest, slen, src) bn_subc_sp(dlen, dest, dlen, dest, slen, src, 0)

#define bn_subc(dest, src1, src2, carry) bn_subc_sp(bn_split(dest), bn_split(src1), bn_split(src2), carry)
#define bn_subac(dest, src, carry) bn_subc_sp(bn_split(dest), bn_split(dest), bn_split(src), carry)
#define bn_subi(dest, src, shift) bn_subc_sp(bn_split(dest), bn_split(src), 0, NULL, shift)
#define bn_subai(num, shift) bn_subc_sp(bn_split(num), bn_split(num), 0, NULL, shift)
#define bn_sub(dest, src1, src2) bn_subc_sp(bn_split(dest), bn_split(src1), bn_split(src2), 0)
#define bn_suba(dest, src) bn_subc_sp(bn_split(dest), bn_split(dest), bn_split(src), 0)


bn_t bn_muli_sp(size_t dlen, BN_TYPE *dest, size_t slen, const BN_TYPE *src, BN_TYPE scale);
#define bn_mulai_sp(len, num, scale) bn_muli_sp(len, num, len, num, scale)
#define bn_muli(dest, src, scale) bn_muli(bn_split(dest), bn_split(src), scale)
#define bn_mulai(num, scale) bn_muli(bn_split(num), bn_split(num), scale)
bn_t bn_mul_sp(size_t dlen, BN_TYPE *dest, size_t slen1, const BN_TYPE *src1, size_t slen2, const BN_TYPE *src2);
#define bn_mul(dest, src1, src2) bn_mul_sp(bn_split(dest), bn_split(src1), bn_split(src2))


bn_t bn_divi_sp(size_t qlen, BN_TYPE *quot, BN_TYPE *remd, size_t slen, BN_TYPE *src, BN_TYPE divis);
#define bn_divai_sp(len, num, remd, divis) bn_divi_sp(len, num, remd, len, num, divis)
#define bn_divi(quot, remd, src, divis) bn_divi_sp(bn_split(quot), remd, bn_split(src), divis)
#define bn_divai(num, remd, divis) bn_divi_sp(bn_split(num), remd, bn_split(num), divis)
bn_t bn_div_sp(size_t qlen, BN_TYPE *quot, size_t rlen, BN_TYPE *remd, size_t slen, const BN_TYPE *src, size_t dvlen, const BN_TYPE *divis);
#define bn_div(quot, remd, src, divis) bn_div_sp(bn_split(quot), bn_split(remd), bn_split(src), bn_split(divis))
#define bn_diva(quot, divis) bn_div_sp(bn_split(quot), bn_split(divis), bn_split(quot), bn_split(divis))


int bn_tostrn_sp(char *str, size_t n, size_t len, BN_TYPE *digs);
#define bn_tostr_sp(str, len, digs) bn_tostrn_sp(str, 0, len, digs)
#define bn_tostrn(str, n, num) bn_tostrn_sp(str, n, bn_split(num))
#define bn_tostr(str, num) bn_tostrn_sp(str, 0, bn_split(num))

bn_t bn_frmstrn_sp(size_t len, BN_TYPE *digs, const char *str, size_t n);
#define bn_frmstr_sp(len, digs, str) bn_frmstrn_sp(len, digs, str, strlen(str))
#define bn_frmstrn(num, str, n) bn_frmstrn_sp(bn_split(num), str, n)
#define bn_frmstr(num, str) bn_frmstrn_sp(bn_split(num), str, strlen(str))

bn_t bn_new_frmstrn(const char *str, size_t n);
#define bn_new_frmstr(str) bn_new_frmstrn(str, strlen(str))

#endif


#include <stdio.h>

// 32-bit Big Numbers
#define BN_TYPE_SIZE 4
#include "bn.h"

// Macro and function for checking if results are correct
#define equal(num1, num2) ((num1).length == (num2).length && memcmp((num1).digits, (num2).digits, sizeof(BN_TYPE) * (num1).length) == 0)
int check(bn_t target, bn_t result, const char *str);

// Test bn_new, bn_copy, bn_move, bn_set, bn_free
int test_allocs();
// Test all variants of bn_not, bn_and, bn_or, bn_xor, & bn_shl
int test_bitwise();
// Test all variants of bn_neg, bn_add, & bn_sub
int test_addsub();
// Test bn_muli, bn_mulai, & bn_mul
int test_mul();
// Test bn_divi, bn_divai, bn_div, & bn_diva
int test_div();
// Test all variants of bn_frmstr, bn_new_frmstr, & bn_tostr
int test_str();

// Set of example big numbers
bn_t nums[] = {
	// 0 : 0
	{6, (BN_TYPE[]){0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000000}},
	// 1 : 1
	{1, (BN_TYPE[]){0x00000001}},
	// 2 : 22065257309404380781069803870289815
	{4, (BN_TYPE[]){0x7668ff97, 0x4be8ad17, 0xb3a15552, 0x00043fe6}},
	// 3 : -7458285370550
	{2, (BN_TYPE[]){0x7b8cc34a, 0xfffff937}},
	// 4 : 12785540905508522460293889528283776876463849906378861918835686732743425766188
	{8, (BN_TYPE[]){0x8571b32c, 0x7668ff97, 0x5b7887c2, 0xb62768a3, 0x96a50ea8, 0xe755ae18, 0xaa70731f, 0x1c445c15}},
	// 5 : -204974990812697179449311353
	{3, (BN_TYPE[]){0xf77e3f87, 0x7b97b945, 0xff5672db}},
	// 6 : -6719846781726058436505554736982499562898233926
	{5, (BN_TYPE[]){0x22b965ba, 0xfe9c2a69, 0xc4197dcb, 0x2c99f569, 0xfed2abf1}},
	// 7 : 1014573320
	{1, (BN_TYPE[]){0x3c792908}},
	// 8 : -339986191827177683663286
	{4, (BN_TYPE[]){0x3ade464a, 0x4fc5b13a, 0xffffb801, 0xffffffff}},
	{0, NULL}
};


// Digit Buffers
BN_TYPE dg_buf1[1];
BN_TYPE dg_buf2[2];
BN_TYPE dg_buf3[3];
BN_TYPE dg_buf4[4];
BN_TYPE dg_buf5[5];
BN_TYPE dg_buf6[6];
BN_TYPE dg_buf8[8];
BN_TYPE dg_buf10[10];
BN_TYPE dg_buf16[16];
BN_TYPE dg_buf32[32];

// Big Number Registers
bn_t regs[] = {
	{1, dg_buf1}, {2, dg_buf2}, {3, dg_buf3}, {4, dg_buf4}, {5, dg_buf5}, {6, dg_buf6},
	{8, dg_buf8}, {10, dg_buf10}, {16, dg_buf16}, {32, dg_buf32},
	{0, NULL}
};



int main(int argc, char *argv[]){
	int (*tests[])(void) = {
		test_allocs, test_bitwise, test_addsub, test_mul, test_div, test_str, NULL
	};
	
	// Perform Tests
	int fails = 0;
	for(size_t i = 0; tests[i]; i++){
		fails += tests[i]();
		puts("+--------------------------+");
	}
	
	printf("%i Failures\n", fails);
	return fails;
}



int check(bn_t target, bn_t result, const char *str){
	int eq = equal(target, result);
	printf("%s: %s\n", str, eq ? "Success" : "FAILURE");
	if(!eq){
		// Print out `target`
		printf("Target: length=%u  0x", target.length);
		for(int i = target.length - 1; i >= 0; i--)
			printf("%08lx", target.digits[i]);
		putchar('\n');
		
		// Print out `result`
		printf("Result: length=%u  0x", result.length);
		for(int i = result.length - 1; i >= 0; i--)
			printf("%08lx", result.digits[i]);
		putchar('\n');
	}
	return !eq;
}



int test_allocs(){
	int fails = 0;
	
	// Test bn_set
	bn_set(regs[1], 0x894df734);
	bn_t tgt_set = {2, (BN_TYPE[]){0x894df734, 0xffffffff}};
	fails += check(tgt_set, regs[1], "bn_set");
	
	// Test bn_move
	bn_move(regs[3], nums[7]);
	bn_t tgt_move = {4, (BN_TYPE[]){0x3c792908, 0x00000000, 0x00000000}};
	fails += check(tgt_move, regs[3], "bn_move");
	
	// Test bn_new
	bn_t res = bn_new(1, 0x006744de);
	bn_t tgt_new = {1, (BN_TYPE[]){0x006744de}};
	fails += check(tgt_new, res, "bn_new");
	
	// Test bn_free
	bn_free(res);
	printf("bn_free: Success\n");
	
	return fails;
}

int test_bitwise(){
	int fails = 0;
	
	// Bitwise Not
	bn_not(regs[7], nums[4]);
	bn_t tgt_not = {10, (BN_TYPE[]){0x7a8e4cd3, 0x89970068, 0xa487783d, 0x49d8975c, 0x695af157, 0x18aa51e7, 0x558f8ce0, 0xe3bba3ea, 0xffffffff, 0xffffffff}};
	fails += check(tgt_not, regs[7], "bn_not");
	
	bn_nota(regs[7]);
	bn_t tgt_nota = {10, (BN_TYPE[]){0x8571b32c, 0x7668ff97, 0x5b7887c2, 0xb62768a3, 0x96a50ea8, 0xe755ae18, 0xaa70731f, 0x1c445c15, 0x00000000, 0x00000000}};
	fails += check(tgt_nota, regs[7], "bn_nota");
	
	// Bitwise And
	bn_and(regs[2], nums[3], nums[5]);
	bn_t tgt_and = {3, (BN_TYPE[]){0x730c0302, 0x7b97b905, 0xff5672db}};
	fails += check(tgt_and, regs[2], "bn_and");
	
	bn_anda(regs[2], nums[7]);
	bn_t tgt_anda = {3, (BN_TYPE[]){0x30080100, 0x00000000, 0x00000000}};
	fails += check(tgt_anda, regs[2], "bn_anda");
	
	// Bitwise Or
	bn_or(regs[2], nums[2], nums[5]);
	bn_t tgt_or = {3, (BN_TYPE[]){0xf77eff97, 0x7bffbd57, 0xfff777db}};
	fails += check(tgt_or, regs[2], "bn_or");
	
	bn_ora(regs[2], nums[7]);
	bn_t tgt_ora = {3, (BN_TYPE[]){0xff7fff9f, 0x7bffbd57, 0xfff777db}};
	fails += check(tgt_ora, regs[2], "bn_ora");
	
	// Bitwise Xor
	bn_xor(regs[2], nums[2], nums[6]);
	bn_t tgt_xor = {3, (BN_TYPE[]){0x54d19a2d, 0xb574877e, 0x77b82899}};
	fails += check(tgt_xor, regs[2], "bn_xor");
	
	bn_xora(regs[2], nums[5]);
	bn_t tgt_xora = {3, (BN_TYPE[]){0xa3afa5aa, 0xcee33e3b, 0x88ee5a42}};
	fails += check(tgt_xora, regs[2], "bn_xora");
	
	// Bitwise Shift Left
	bn_shl(regs[3], nums[5], 17);
	bn_t tgt_shl = {4, (BN_TYPE[]){0x7f0e0000, 0x728beefc, 0xe5b6f72f, 0xfffffeac}};
	fails += check(tgt_shl, regs[3], "bn_shl");
	
	bn_shla(regs[3], 45);
	bn_t tgt_shla = {4, (BN_TYPE[]){0x00000000, 0xc0000000, 0x7ddf8fe1, 0xdee5ee51}};
	fails += check(tgt_shla, regs[3], "bn_shla");
	
	// Bitwise Shift Right
	bn_shr(regs[3], nums[5], 17);
	bn_t tgt_shr = {4, (BN_TYPE[]){0xdca2fbbf, 0x396dbdcb, 0xffffffab, 0xffffffff}};
	fails += check(tgt_shr, regs[3], "bn_shr");
	
	bn_shra(regs[3], 45);
	bn_t tgt_shra = {4, (BN_TYPE[]){0xfd59cb6d, 0xffffffff, 0xffffffff, 0xffffffff}};
	fails += check(tgt_shra, regs[3], "bn_shra");
	
	return fails;
}

int test_addsub(){
	int fails = 0;
	
	// Negation
	bn_neg(regs[6], nums[4]);
	bn_t tgt_neg = {8, (BN_TYPE[]){0x7a8e4cd4, 0x89970068, 0xa487783d, 0x49d8975c, 0x695af157, 0x18aa51e7, 0x558f8ce0, 0xe3bba3ea}};
	fails += check(tgt_neg, regs[6], "bn_neg");
	
	bn_nega(regs[6]);
	fails += check(nums[4], regs[6], "bn_nega");
	
	// Addition
	bn_addi(regs[4], nums[6], 555356591);
	bn_t tgt_addi = {5, (BN_TYPE[]){0x43d37769, 0xfe9c2a69, 0xc4197dcb, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_addi, regs[4], "bn_addi");
	
	bn_addai(regs[4], -1216253080);
	bn_t tgt_addai = {5, (BN_TYPE[]){0xfb54ead1, 0xfe9c2a68, 0xc4197dcb, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_addai, regs[4], "bn_addai");
	
	bn_add(regs[4], nums[5], nums[6]);
	bn_t tgt_add = {5, (BN_TYPE[]){0x1a37a541, 0x7a33e3af, 0xc36ff0a7, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_add, regs[4], "bn_add");
	
	bn_adda(regs[4], nums[7]);
	bn_t tgt_adda = {5, (BN_TYPE[]){0x56b0ce49, 0x7a33e3af, 0xc36ff0a7, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_adda, regs[4], "bn_adda");
	
	// Subtraction
	bn_subi(regs[4], nums[6], 987927591);
	bn_t tgt_subi = {5, (BN_TYPE[]){0xe7d6d193, 0xfe9c2a68, 0xc4197dcb, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_subi, regs[4], "bn_subi");
	
	bn_subai(regs[4], -717462829);
	bn_t tgt_subai = {5, (BN_TYPE[]){0x129a6ec0, 0xfe9c2a69, 0xc4197dcb, 0x2c99f569, 0xfed2abf1}};
	fails += check(tgt_subai, regs[4], "bn_subai");
	
	bn_sub(regs[3], nums[5], nums[2]);
	bn_t tgt_sub = {4, (BN_TYPE[]){0x81153ff0, 0x2faf0c2e, 0x4bb51d89, 0xfffbc019}};
	fails += check(tgt_sub, regs[3], "bn_sub");
	
	bn_suba(regs[3], nums[6]);
	bn_t tgt_suba = {4, (BN_TYPE[]){0x5e5bda36, 0x3112e1c5, 0x879b9fbd, 0xd361caaf}};
	fails += check(tgt_suba, regs[3], "bn_suba");
	
	return fails;
}

int test_mul(){
	int fails = 0;
	
	// Multiplication by BN_SIGNED
	bn_muli(regs[4], nums[5], 1000637424);
	bn_t tgt_muli = {5, (BN_TYPE[]){0x7059a390, 0x52e924c2, 0xc7e922e8, 0xffd87f82, 0xffffffff}};
	fails += check(tgt_muli, regs[4], "bn_muli");
	
	bn_mulai(regs[4], -1233590391);
	bn_t tgt_mulai = {5, (BN_TYPE[]){0x0e0f7810, 0xc59ae02c, 0x8f32c9cd, 0x3d6f9091, 0x000b587c}};
	fails += check(tgt_mulai, regs[4], "bn_mulai");
	
	// Multiplication of two Big Numbers
	bn_mul(regs[8], nums[4], nums[5]);
	bn_t tgt_mul = {16, (BN_TYPE[]){
		0xfdb55034, 0x3bad8b6f, 0x8142a20a, 0x76669e3a,
		0x9c6925f7, 0xf86c067d, 0x6904a710, 0xd5c9d343,
		0x483f0191, 0x86bed6c8, 0xffed4749, 0xffffffff,
		0xffffffff,	0xffffffff,	0xffffffff,	0xffffffff
	}};
	fails += check(tgt_mul, regs[8], "bn_mul");
	
	return fails;
}

int test_div(){
	int fails = 0;
	
	// Division by BN_SIGNED
	bn_divi(regs[3], regs[0].digits, nums[2], -738654473);
	bn_t tgt_divi_q = {4, (BN_TYPE[]){0xba4aac1e, 0x17e6300a, 0xffe74a4f, 0xffffffff}};
	fails += check(tgt_divi_q, regs[3], "bn_divi (Quotient)");
	bn_t tgt_divi_r = {1, (BN_TYPE[]){0xd5263aa5}};
	fails += check(tgt_divi_r, regs[0], "bn_divi (Remainder)");
	
	bn_divai(regs[3], regs[0].digits, 235894800);
	bn_t tgt_divai_q = {4, (BN_TYPE[]){0x199b57de, 0xfe3e1b52, 0xffffffff, 0xffffffff}};
	fails += check(tgt_divai_q, regs[3], "bn_divai (Quotient)");
	bn_t tgt_divai_r = {1, (BN_TYPE[]){0x5631e3e}};
	fails += check(tgt_divai_r, regs[0], "bn_divai (Remainder)");
	
	// Division by Big Number
	bn_div(regs[6], regs[3], nums[4], nums[8]);
	bn_t tgt_div_q = {8, (BN_TYPE[]){0xe3b803b4, 0x83664634, 0xb3f6bf05, 0xfc54b63c, 0xe3d967fb, 0xffff9b7c, 0xffffffff, 0xffffffff}};
	fails += check(tgt_div_q, regs[6], "bn_div (Quotient)");
	bn_t tgt_div_r = {4, (BN_TYPE[]){0x63256924, 0x6e6cc8d7, 0xffffccc2, 0xffffffff}};
	fails += check(tgt_div_r, regs[3], "bn_div (Remainder)");
	
	bn_diva(regs[6], regs[2], nums[3]);
	bn_t tgt_diva_q = {8, (BN_TYPE[]){0x6fb7530e, 0x59674b80, 0x7b89511a, 0xd151a12c, 0x0000000e, 0x00000000, 0x00000000, 0x00000000}};
	fails += check(tgt_diva_q, regs[6], "bn_diva (Quotient)");
	bn_t tgt_diva_r = {3, (BN_TYPE[]){0xcece57a8, 0xfffffad0, 0xffffffff}};
	fails += check(tgt_diva_r, regs[2], "bn_diva (Remainder)");
	
	return fails;
}

int test_str(){
	int eq, fails = 0;
	bn_t res;
	char buf[256];
	
	const char str1[] = "-6719846781726058436505554736982499562898233926";
	const char str2[] = "22065257309404380781069803870289815";
	const char str3[] = "00001";
	const char str4[] = "12785540905508522460293889528283776876463849906378861918835686732743425766188";
	
	// From String
	bn_frmstrn(regs[4], str1, strlen(str1));
	fails += check(nums[6], regs[4], "bn_frmstrn");
	
	bn_frmstr(regs[3], str2);
	fails += check(nums[2], regs[3], "bn_frmstr");
	
	res = bn_new_frmstrn(str3, 5);
	fails += check(nums[1], res, "bn_new_frmstrn");
	bn_free(res);
	
	res = bn_new_frmstr(str4);
	fails += check(nums[4], res, "bn_new_frmstr");
	bn_free(res);
	
	// To String
	bn_tostrn(buf, strlen(str1), nums[6]);
	eq = strcmp(buf, str1) == 0;
	printf("bn_tostrn: %s\n", eq ? "Success" : "FAILURE");
	if(!eq){
		fails++;
		printf("Target: %s\nResult: %s\n", str1, buf);
	}
	
	bn_tostr(buf, nums[4]);
	eq = strcmp(buf, str4) == 0;
	printf("bn_tostr: %s\n", eq ? "Success" : "FAILURE");
	if(!eq){
		fails++;
		printf("Target: %s\nResult: %s\n", str4, buf);
	}
	
	return fails;
}


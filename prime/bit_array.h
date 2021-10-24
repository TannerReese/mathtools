#ifndef _BIT_ARRAY_H
#define _BIT_ARRAY_H

#ifndef BIT_TYPE
#define BIT_TYPE unsigned char
#endif

#define BIT_SIZE (8 * sizeof(BIT_TYPE))

/* Converts number of bits in bit array into number of elements
 * 
 * Arguments:
 *   unsigned int n : number of bits in bit array
 * 
 * Returns:
 *   unsigned int : number of BIT_TYPEs needed to store n bits
 */
#define elem_size(n) (((n) % BIT_SIZE == 0 ? 0 : 1) + (n) / BIT_SIZE)

/* Convert number of bits in bit array into number of bytes
 * 
 * Arguments:
 *   unsinged int n : number of bits in bit array
 * 
 * Returns:
 *   unsigned int : number of bytes needed to store n bits
 */
#define byte_size(n) (((n) % 8 == 0 ? 0 : 1) + (n) / 8)

/* Get ith bit from bs
 * 
 * Arguments:
 *   bits_t bs : array of bits
 *   unsinged int i : index of desired bit
 * 
 * Returns:
 *   BIT_TYPE : desired bit at index i
 */
#define getbit(bs, i) (((bs)[(i) / BIT_SIZE] >> ((i) % BIT_SIZE)) & 0x1)

/* Set ith bit of bs to v
 * 
 * Arguments:
 *   bits_t bs : array of bits
 *   unsigned int i : index of bit to set
 *   BIT_TYPE v : value to change bit to
 *     NOTE: only the LSB of v will be used
 */
#define setbit(bs, i, v) if(0x01 & (v)){(bs)[(i) / BIT_SIZE] |= 1 << ((i) % BIT_SIZE);}else{(bs)[(i) / BIT_SIZE] &= ~(BIT_TYPE)(1 << ((i) % BIT_SIZE));}

/* Toggle value of ith bit in bs
 * 
 * Arguments:
 *   bits_t bs : array of bits
 *   unsigned int i : index of bit to toggle
 */
#define toggle_bit(bs, i) ((bs)[(i) / BIT_SIZE] ^= 1 << ((i) % BIT_SIZE))

typedef BIT_TYPE *bits_t;
#endif

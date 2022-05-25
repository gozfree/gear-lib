/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TOOLS_LINUX_ASM_GENERIC_BITOPS_FIND_H_
#define _TOOLS_LINUX_ASM_GENERIC_BITOPS_FIND_H_

#ifndef find_next_bit
/**
 * find_next_bit - find the next set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number for the next set bit
 * If no bits are set, returns @size.
 */
extern unsigned long find_next_bit(const unsigned long *addr, unsigned long
		size, unsigned long offset);
#endif

#ifndef find_next_and_bit
/**
 * find_next_and_bit - find the next set bit in both memory regions
 * @addr1: The first address to base the search on
 * @addr2: The second address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number for the next set bit
 * If no bits are set, returns @size.
 */
extern unsigned long find_next_and_bit(const unsigned long *addr1,
		const unsigned long *addr2, unsigned long size,
		unsigned long offset);
#endif

#ifndef find_next_zero_bit

/**
 * find_next_zero_bit - find the next cleared bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number of the next zero bit
 * If no bits are zero, returns @size.
 */
unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
				 unsigned long offset);
#endif

#ifndef find_first_bit

/**
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum number of bits to search
 *
 * Returns the bit number of the first set bit.
 * If no bits are set, returns @size.
 */
extern unsigned long find_first_bit(const unsigned long *addr,
				    unsigned long size);

#endif /* find_first_bit */

#ifndef find_first_zero_bit

/**
 * find_first_zero_bit - find the first cleared bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum number of bits to search
 *
 * Returns the bit number of the first cleared bit.
 * If no bits are zero, returns @size.
 */
unsigned long find_first_zero_bit(const unsigned long *addr, unsigned long size);
#endif

#endif /*_TOOLS_LINUX_ASM_GENERIC_BITOPS_FIND_H_ */

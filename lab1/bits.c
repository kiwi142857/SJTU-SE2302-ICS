/*
 * CS:APP Data Lab
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#include "btest.h"
#include <limits.h>

/*
 * Instructions to Students:
 *
 * STEP 1: Fill in the following struct with your identifying info.
 */
team_struct team =
    {
        /* Team name: Replace with either:
           Your login ID if working as a one person team
           or, ID1+ID2 where ID1 is the login ID of the first team member
           and ID2 is the login ID of the second team member */
        "522031910299",
        /* Student name 1: Replace with the full name of first team member */
        "CHEN QiWei",
        /* Login ID 1: Replace with the login ID of first team member */
        "522031910299",

        /* The following should only be changed if there are two team members */
        /* Student name 2: Full name of the second team member */
        "",
        /* Login ID 2: Login ID of the second team member */
        ""};

#if 0
/*
 * STEP 2: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.
#endif

/*
 * STEP 3: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the btest test harness to check that your solutions produce
 *      the correct answers. Watch out for corner cases around Tmin and Tmax.
 */
/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x)
{
  // as int is considered as 32 bits, we can divide it into 8 parts, each part has 4 bits
  // we can use 0x11111111 to get the sum of each part
  // then we can use >> to get the sum of each 2 parts
  // then we can use >> to get the sum of each 4 parts
  // then we can use >> to get the sum of each 8 parts

  int mask2 = 0x11 | (0x11 << 8);
  int mask = mask2 | (mask2 << 16);
  int sum = x & mask;
  int mask3 = 0xff;
  int mask4 = 0x0f;
  sum += (x >> 1) & mask;
  sum += (x >> 2) & mask;
  sum += (x >> 3) & mask; // sum of each part is stored in sum

  mask3 = mask3 | (mask3 << 8);
  // since sum is 4 bits, we can use 0x0f to get the sum of each 2 parts
  sum = (sum & mask3) + ((sum >> 16) & mask3);

  // now sum is the sum of each 2 parts
  //  the max of sum is 8, and if we use this method again, the sum could be 16 which is beyond the range of 4-bit
  //  so we need to use 0x0f to get the sum of each 4 parts
  mask = mask4 | (mask4 << 8);
  sum = (sum & mask) + ((sum >> 4) & mask);

  // now sum is the sum of each 4 parts
  // now we have to add it again to get the final result
  mask = 0xff;
  sum = (sum & mask) + ((sum >> 8) & mask);
  // now sum is the final result
  sum = sum & 0x3f;
  // the max of sum is 32, so we need to use 0x3f to get the last 6 bits
  return sum;
}
/*
 * copyLSB - set all bits of result to least significant bit of x
 *   Example: copyLSB(5) = 0xFFFFFFFF, copyLSB(6) = 0x00000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int copyLSB(int x)
{
  int mask = 0x00000001;
  return ~(~0 + (mask & x));
}
/*
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 2
 */
int evenBits(void)
{
  int mask = 0x55;
  mask = mask | (mask << 8);
  mask = mask | (mask << 16);
  return mask;
}
/*
 * fitsBits - return 1 if x can be represented as an
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n)
{
  int shift = 32 + (~n + 1);           // calculate the number of bits to shift
  int shifted = (x << shift) >> shift; // shift x to keep only the n most significant bits
  return !(shifted ^ x);               // return 1 if x can be represented as an n-bit, two's complement integer, and 0 otherwise
}
/*
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n)
{
  int mask = 0x000000ff & (x >> (n << 3));
  return mask;
}
/*
 * isGreater - if x > y  then return 1, else return 0
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y)
{
  int sx = x >> 31;
  int sy = y >> 31;
  int temp = (x ^ y);
  y = ~y + 1;

  return ((!(((x + y) & (1 << 31)) >> 31) & !(sx & !sy) & !(!(x ^ y))) | (!(sx) & sy)) & !!temp;
}
/*
 * isNonNegative - return 1 if x >= 0, return 0 otherwise
 *   Example: isNonNegative(-1) = 0.  isNonNegative(0) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 3
 */
int isNonNegative(int x)
{
  return !(x >> 31);
}
/*
 * isNotEqual - return 0 if x == y, and 1 otherwise
 *   Examples: isNotEqual(5,5) = 0, isNotEqual(4,5) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNotEqual(int x, int y)
{
  return !(!(x ^ y));
}
/*
 * leastBitPos - return a mask that marks the position of the
 *               least significant 1 bit. If x == 0, return 0
 *   Example: leastBitPos(96) = 0x20
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 4
 */
int leastBitPos(int x)
{
  return x & (~x + 1);
}
/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 1 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int logicalShift(int x, int n)
{
  // how to express 0x7fffffff
  // 0x7fffffff = 0x80000000 - 1
  return x >> n & ((1 << 31) + ~0) >> (n + ~0);
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          the maximum value, and when negative overflow occurs,
 *          it returns the minimum value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y)
{
  int sum = x + y;
  int sx = x >> 31;
  int sy = y >> 31;
  int ssum = sum >> 31;
  int pos_over = !sx & !sy & ssum;
  int neg_over = sx & sy & !ssum;
  int pos = (pos_over | neg_over) + ~0;
  pos_over = ~pos_over + 1;
  neg_over = ~neg_over + 1;
  return (pos_over & ((1 << 31) + ~0)) | (neg_over & (1 << 31)) | (pos & sum);
}

/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int sign = x >> 31; // get the sign of x
  int absX = (sign & ~x) | (~sign & x); // get the absolute value of x
  int absX2 = absX;
  int bits = 0; // initialize the number of bits to 0
  // find the first 1 in x
  bits = bits + (!!(absX >> 16) << 4); // if the first 1 is in the first 16 bits, then we need at least 16 bits to represent x
  absX = absX2 >> (bits & 0x1f); 
  bits = bits + (!!(absX >> 8) << 3); // if the first 1 is in the first 8 bits, then we need at least 8 bits to represent x
  absX = absX2 >> (bits & 0x1f); 
  bits = bits + (!!(absX >> 4) << 2); // if the first 1 is in the first 4 bits, then we need at least 4 bits to represent x
  absX = absX2 >> (bits & 0x1f); 
  bits = bits + (!!(absX >> 2) << 1); // if the first 1 is in the first 2 bits, then we need at least 2 bits to represent x
  absX = absX2 >> (bits & 0x1f); 
  bits = bits + (!!(absX >> 1)); 
  absX = absX2 >> (bits & 0x1f); // shift absX to the right by bits
  bits = bits + absX + 1; // if the first 1 is in the first 1 bit, then we need at least 1 bit to represent x
  return bits; // return the number of bits
}

/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x)
{
  // eight of +x and -x is negative , so we can use the sign bit to judge whether x is 0
  return ((x | (~x + 1)) >> 31) + 1;
}

/*
 * dividePower2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: dividePower2(15,1) = 7, dividePower2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int dividePower2(int x, int n)
{
  int sx = x >> 31;
  int isadd = !!(x & ((1 << n) + ~0));
  x = x >> n;

  x = x + (sx & isadd);
  return x;
}

/*
 * bang - Convert from two's complement to sign-magnitude
 *   where the MSB is the sign bit
 *   You can assume that x > TMin
 *   Example: bang(-5) = 0x80000005.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 4
 */
int bang(int x)
{
  int k = ~(1 << 31);
  int a = x >> 31;
  a = ~a;
  // printf("0xffffffff+0 %.8x\n",0xffffffff+0);
  // printf("(0xffffffff + (0x80000000 & x)>>31)  %.8x\n",(0xffffffff + (0x80000000 & x)>>31));
  // printf("(0x80000000 & x)>>31  %.8x\n",(0x80000000 & x)>>31);
  // printf("a=%.8x\n",a);

  x = x & k;
  // printf("x=%.8x\n",x);

  // printf("~x+1=%.8x\n",~x+1);
  // printf("~x+1=%.d\n",~x+1);
  return (x & a) | ((~x + 1) & ~a);
}
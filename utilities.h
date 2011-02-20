#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdint.h>

#define INT_BITS(TYPE, VAL, BITS_START, BITS_SZ)    ( ( (VAL) & ( ( ( (TYPE)1 << (BITS_SZ) ) - 1 ) << (BITS_START) ) ) >> (BITS_START) )

#define FASTCALL	__attribute__((fastcall))

#endif

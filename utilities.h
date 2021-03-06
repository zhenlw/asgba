#ifndef UTILITIES_H_
#define UTILITIES_H_

#define TRACE_ON

#ifdef TRACE_ON
#define PRINT_TRACE(...) PrintTrace(__VA_ARGS__)
#else
#define PRINT_TRACE(...)
#endif

extern bool bDyncTrace;

#ifdef _WIN32

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#ifdef _MSC_VER

#define FASTCALL

#include <stdio.h>
#include <stdarg.h>

inline void PrintTrace(const char* szFormat, ...)
{
	if ( bDyncTrace ){
		char szBuff[1024];
		va_list arg;
		va_start(arg, szFormat);
		_vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
		va_end(arg);

		::OutputDebugString(szBuff);
	}
}

#if ( _MSC_VER <= 1500 )

typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;

#define ASGBA_NO_STD_INT_H

#endif

#endif

#else

#define FASTCALL	__attribute__((fastcall))
//windows headers will define this by default, no matter mingw or vc

#endif

#ifndef ASGBA_NO_STD_INT_H
#include <stdint.h>
#endif

#define INT_BITS(TYPE, VAL, BITS_START, BITS_SZ)    ( ( (TYPE)(VAL) & ( ( ( (TYPE)1 << (BITS_SZ) ) - 1 ) << (BITS_START) ) ) >> (BITS_START) )

enum SystemEvent{EVT_NONE = 0, EVT_VBLK, EVT_HBLK, EVT_FIFO, EVT_VSTART, EVT_HSTART, EVT_FIFO_FULL};

typedef FASTCALL bool (*AsgbaEvtHdlr)(SystemEvent evt, void *pParam);

#endif

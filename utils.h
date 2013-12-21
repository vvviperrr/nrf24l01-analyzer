#pragma once

#ifdef _WINDOWS
# include <windows.h>
#endif

#include <string>
#include <LogicPublicTypes.h>

//std::string int2str(const U8 i);
std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits = 8);
inline std::string int2str(const U64 i)
{
	return int2str_sal(i, Decimal, 64);
}

// debugging helper functions -- Windows only!!!
inline void debug(const std::string& str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	::OutputDebugStringA(("----- " + str + "\n").c_str());
#endif
}

inline void debug(const char* str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	debug(std::string(str));
#endif
}

#include <AnalyzerHelpers.h>

#include "utils.h"

std::string int2str(const U8 i)
{
	char number_str[8];
	AnalyzerHelpers::GetNumberString(i, Decimal, 8, number_str, sizeof(number_str));
	return number_str;
}

std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits)
{
	char number_str[256];
	AnalyzerHelpers::GetNumberString(i, base, max_bits, number_str, sizeof(number_str));
	return number_str;
}

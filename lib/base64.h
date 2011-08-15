#include <string>

#ifndef _OPENINSTRUMENT_LIB_BASE64_H_
#define _OPENINSTRUMENT_LIB_BASE64_H_

std::string Base64Encode(unsigned char const *, unsigned int len);
std::string Base64Decode(std::string const &s);

#endif  // _OPENINSTRUMENT_LIB_BASE64_H_

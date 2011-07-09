#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>

std::string WideCharToMultiByte(const std::wstring& from);
wchar_t* GetMvPreviewPath();

#endif  // _UTIL_H_
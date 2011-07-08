#ifndef _COMMON_H_
#define _COMMON_H_

#include <afx.h>

const wchar_t* GetBaseQuery();

bool IsNumber(CString str);

int NumberCompare(CString str1, CString str2);
#endif
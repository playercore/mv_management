#include "util.h"

#include <memory>
#include <windows.h>

using std::string;
using std::wstring;
using std::unique_ptr;

string WideCharToMultiByte(const wstring& from)
{
    int size = ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, NULL, 0, NULL,
                                     NULL);
    if (1 > size)
        return string();

    unique_ptr<char[]> dst(new char[size]);
    ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, dst.get(), size, NULL,
                          NULL);
    return string(dst.get());
}

wchar_t* GetMvPreviewPath()
{
    return L"\\\\192.168.0.200\\mv_preview\\";
}

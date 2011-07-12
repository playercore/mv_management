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

wstring Utf8ToWideChar(const string& from)
{
    // TODO: 是否在程序初始化时而不是每次调用本函数时调用setlocale会更好。
    setlocale(LC_ALL, "chs");
    wstring result;
    do
    {
        const int bufSize = mbstowcs(NULL, from.c_str(), 0) + 1;
        if (bufSize < 2)
            break;

#if (_MSC_VER >= 1600)
        unique_ptr<wchar_t[]> buf(new wchar_t[bufSize]);
#else
        scoped_array<wchar_t> buf(new wchar_t[bufSize]);
#endif
        if (mbstowcs(buf.get(), from.c_str(), bufSize) < 2)
            break;

        buf[bufSize - 1] = '\0';
        result = buf.get();
    } while (0);

    setlocale(LC_ALL, "C");
    return result;
}


wchar_t* GetMvPreviewPath()
{
    return L"\\\\192.168.0.200\\mv_preview\\";
}

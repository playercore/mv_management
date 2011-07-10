#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>

std::string WideCharToMultiByte(const std::wstring& from);
wchar_t* GetMvPreviewPath();

template <typename T, typename F>
class SmartInvokation
{
public:
    SmartInvokation(const T& t, const F& f) : t_(t), f_(f) {}
    ~SmartInvokation() { f_(t_); }

private:
    const T& t_;
    const F& f_;
};

#endif  // _UTIL_H_
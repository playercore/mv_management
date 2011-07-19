#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>

class CBitmap;

std::string WideCharToMultiByte(const std::wstring& from);
std::wstring MultiByteToWideChar(const std::string& from);
wchar_t* GetMvPreviewPath();
void LoadNotificationImage(int id, CBitmap* target, CBitmap* mask);

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
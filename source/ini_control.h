#ifndef _INI_CONTROL_H_
#define  _INI_CONTROL_H_

#include <string>
#include <vector>

#include "third_party/chromium/base/singleton.h"

class CIniControl : public Singleton<CIniControl>
{
public:
    void Init(const wchar_t* path);
    std::wstring GetIDFrom();
    std::wstring GetIDTo();
    std::vector<std::wstring> GetMVType();
    std::wstring GetKmplayer();
    std::wstring GetServerIP();
    std::wstring GetUserName();
    std::wstring GetPassword();
    std::wstring GetDatabaseName();
    void SetIDFrom(wchar_t* id);
    void SetIDTo(wchar_t* id);

    CIniControl();
    ~CIniControl();

private:
    std::wstring m_path;
};

#endif // _INI_CONTROL_H_
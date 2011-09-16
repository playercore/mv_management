#include "ini_control.h"

#include <memory>

using std::vector;
using std::wstring;
using std::unique_ptr;

CIniControl::CIniControl()
    : m_profilePath()
{
}

CIniControl::~CIniControl()
{
}

void CIniControl::Init(const wchar_t* path)
{
    m_profilePath = path;
}

wstring CIniControl::GetIDFrom()
{
    return GetProfileString(L"ID", L"START", L"0");
}

wstring CIniControl::GetIDTo()
{
    return GetProfileString(L"ID", L"END", L"0");
}

vector<wstring> CIniControl::GetMVType()
{
    wstring keys = GetProfileString(L"type", NULL, L"");

    const int bufSize = keys.size();
    const wchar_t* p = &keys[0];
    const wchar_t* stringEnd = p + bufSize;
    vector<wstring> types;
    while (p < stringEnd)
    {
        wstring key(p);
        if (key.empty()) {
            p++;
            continue;
        }

        wstring value = GetProfileString(L"type", key.c_str(), L"");
        types.push_back(value);

        p += key.length() + 1;
    }

    return types;    
}

wstring CIniControl::GetPlayerPathName()
{
    return GetProfileString(L"setup", L"PlayerPathName", L"");
}

wstring CIniControl::GetServerIP()
{
    return GetProfileString(L"databaseSetup", L"ServerIP", L"0");
}

wstring CIniControl::GetUserName()
{
    return GetProfileString(L"databaseSetup", L"UserName", L"");
}

wstring CIniControl::GetPassword()
{
    return GetProfileString(L"databaseSetup", L"Password", L"");
}

wstring CIniControl::GetDatabaseName()
{
    return GetProfileString(L"databaseSetup", L"DatabaseName", L"");
}

void CIniControl::SetIDFrom(wchar_t* id)
{
    WritePrivateProfileString(L"ID", L"START", id, m_profilePath.c_str());
}

void CIniControl::SetIDTo(wchar_t* id)
{
    WritePrivateProfileString(L"ID", L"END", id, m_profilePath.c_str());
}

wstring CIniControl::GetProfileString(const wchar_t* appName,
                                      const wchar_t* keyName,
                                      const wchar_t* defaultValue)
{
    int bufSize = 128;
    unique_ptr<wchar_t[]> buf;
    int charCopied;
    const int terminatorLength = (!appName || !keyName) ? 2 : 1;
    do {
        bufSize *= 2;
        buf.reset(new wchar_t[bufSize]);
        charCopied = GetPrivateProfileString(appName, keyName, defaultValue, 
                                             buf.get(), bufSize,
                                             m_profilePath.c_str());
    } while (charCopied >= (bufSize - terminatorLength));
    wstring result;
    result.resize(charCopied);
    memcpy(&result[0], buf.get(), charCopied * sizeof(buf[0]));
    return result;
}
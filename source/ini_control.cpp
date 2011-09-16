#include "ini_control.h"

using std::vector;
using std::wstring;

void CIniControl::Init(const wchar_t* path)
{
    m_path = path;
}

wstring CIniControl::GetIDFrom()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"ID", L"START", L"0", buf, 32767, 
        m_path.c_str());

    return wstring(buf);
}

wstring CIniControl::GetIDTo()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"ID", L"END", L"0", buf, 32767, 
        m_path.c_str());
    
    return wstring(buf);
}

vector<wstring> CIniControl::GetMVType()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"type", NULL, L"", buf, 32767, 
        m_path.c_str());

    vector<wstring> vec;
    wstring str;
    for(int i = 0; i < len; i++)
    {
        if(buf[i] != '\0') 
        {
            str = str + buf[i];
        } 
        else
        {
            if(str != L"") 
                vec.push_back(str);

            str = L"";
        }
    }

    return vec;    
}

wstring CIniControl::GetPlayerPathName()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"setup", L"PlayerPathName", L"", buf,
                                      32767, m_path.c_str());

    return wstring(buf);
}

wstring CIniControl::GetServerIP()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"databaseSetup", L"ServerIP", L"0", buf, 
        32767, m_path.c_str());

    return wstring(buf);
}

wstring CIniControl::GetUserName()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"databaseSetup", L"UserName", L"0", buf,
        32767, m_path.c_str());

    return wstring(buf);
}

wstring CIniControl::GetPassword()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"databaseSetup", L"Password", L"0", 
        buf, 32767, m_path.c_str());

    return wstring(buf);
}

wstring CIniControl::GetDatabaseName()
{
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"databaseSetup", L"DatabaseName", L"0", 
        buf, 32767, m_path.c_str());

    return wstring(buf);
}

void CIniControl::SetIDFrom(wchar_t* id)
{
    WritePrivateProfileString(L"ID", L"START", id, m_path.c_str());
}

void CIniControl::SetIDTo(wchar_t* id)
{
    WritePrivateProfileString(L"ID", L"END", id, m_path.c_str());
}

CIniControl::CIniControl()
    : m_path()
{

}

CIniControl::~CIniControl()
{

}




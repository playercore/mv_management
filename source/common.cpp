#include "common.h"

#include <afx.h>
#include <winsock2.h>

#include "util.h"

bool IsNumber(CString str)
{
    for(int i = 0; i < str.GetLength(); i++)
    {
        if(!_istdigit(str[i]))
            return false;
    }

    return true;
}

int NumberCompare(CString str1, CString str2)
{
    const int number1 = _wtoi((LPTSTR)(LPCTSTR)str1);
    const int number2 = _wtoi((LPTSTR)(LPCTSTR)str2);

    if (number1 < number2)
        return -1;

    if (number1 > number2)
        return 1;

    return 0;
}

std::wstring GetLocalIP()
{
    WORD wVersionRequested;  
    wVersionRequested = MAKEWORD(1, 1);//版本号1.1

    WSADATA  wsaData;
    //1.加载套接字库 
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
        return L"";

    //判断是否我们请求的winsocket版本，如果不是
    //则调用WSACleanup终止winsocket的使用并返回            
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) 
    {
        WSACleanup();
        return L""; 
    }

    char name[255];  
    std::wstring ip;
    PHOSTENT hostinfo;  
    if(gethostname(name, sizeof(name)) == 0)  
    {  
        if((hostinfo = gethostbyname(name)) != NULL)  
        {  
            std::string t = inet_ntoa(*(struct in_addr*)*hostinfo->h_addr_list);  
            ip = MultiByteToWideChar(t);
        }  
    }  

    WSACleanup();  

    return ip;
}

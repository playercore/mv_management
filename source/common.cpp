#include "common.h"

#include <afx.h>
#include <winsock2.h>

#include "util.h"

const wchar_t* GetBaseQuery()
{
    return L"SELECT �������,�ļ�·��,�ɹ�ϣֵ,�༭������,�Ƿ��н���,"
        L"����״̬,��ע,ԭ������,��������,�¹�ϣֵ,��������,���ʼ���,"
        L"�ͻ���ip��ַ,�ύʱ��,ͼƬ����ip��ַ,ͼƬ���,��ͼʱ��,ͼƬ��ע,ͼƬ�ϴ�ʱ��,"
        L"ͼƬ·�� FROM CHECKED_ENCODE_FILE_INFO WHERE 1 = 1";
}

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
    wVersionRequested = MAKEWORD(1, 1);//�汾��1.1

    WSADATA  wsaData;
    //1.�����׽��ֿ� 
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
        return L"";

    //�ж��Ƿ����������winsocket�汾���������
    //�����WSACleanup��ֹwinsocket��ʹ�ò�����            
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

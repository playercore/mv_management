#include "common.h"

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
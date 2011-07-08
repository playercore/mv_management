#include "common.h"

const wchar_t* GetBaseQuery()
{
    return L"SELECT 歌曲编号,文件路径,旧哈希值,编辑重命名,是否有交错,"
        L"歌曲状态,备注,原唱音轨,歌曲类型,新哈希值,总音轨数,画质级别,"
        L"客户端ip地址,提交时间,图片生成ip地址,图片编号,截图时间,图片备注,图片上传时间,"
        L"图片路径 FROM CHECKED_ENCODE_FILE_INFO WHERE 1 = 1";
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
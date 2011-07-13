#include "sql_control.h"

#include <string>

#include <boost/lexical_cast.hpp>

#include "common.h"

using std::wstring;
using boost::lexical_cast;

bool CSQLControl::initConnection()
{
    wstring path;
    wchar_t curPath[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH,curPath);
    path = curPath;
    path += L"\\config.ini";
    try
    {   
        m_connection.CreateInstance(__uuidof(Connection));

        wchar_t buf1[32767];
        int len1 = GetPrivateProfileString(L"databaseSetup", L"ServerIP", L"0", 
            buf1, 32767, path.c_str());
        wstring server = buf1;
        len1 = GetPrivateProfileString(L"databaseSetup", L"UserName", L"0", 
            buf1, 32767, path.c_str());
        wstring userName = buf1;

        len1 = GetPrivateProfileString(L"databaseSetup", L"Password", L"0", 
            buf1, 32767, path.c_str());
        wstring password = buf1;

        len1 = GetPrivateProfileString(L"databaseSetup", L"DatabaseName", L"0", 
            buf1, 32767, path.c_str());
        wstring databaseName = buf1;

        wstring strSQL = L"Driver={SQL Server};Server=";
        strSQL += server;
        strSQL += L";Database=";
        strSQL += databaseName;
        strSQL += L";UID=";
        strSQL += userName;
        strSQL += L";PWD=";
        strSQL += password;

        m_connection->Open((_bstr_t)strSQL.c_str(), "","",adConnectUnspecified);
    }
    catch (_com_error e)
    {
        return false;
    }

    return true;
}

void CSQLControl::closeConnection()
{
    m_connection->Close(); 
}

_RecordsetPtr CSQLControl::BaseSelect(int idFrom, int idTo, int flag)
{
    wstring query = L"SELECT 歌曲编号,文件路径,旧哈希值,编辑重命名,是否有交错,"
        L"歌曲状态,备注,原唱音轨,歌曲类型,新哈希值,总音轨数,画质级别,"
        L"客户端ip地址,提交时间,图片生成ip地址,图片编号,截图时间,图片备注,"   
        L"图片上传时间,图片路径 FROM CHECKED_ENCODE_FILE_INFO WHERE 1 = 1";

    if ((idFrom != -1) && (idTo != -1))
    {
        wstring strIdFrom = lexical_cast<wstring>(idFrom);
        wstring strIdTo = lexical_cast<wstring>(idTo);
        query += L" and 歌曲编号 between ";
        query += strIdFrom + L" and ";
        query += strIdTo;
    }

    if (flag != -1 && flag != 3)
    {
        wstring strFlag = lexical_cast<wstring>(flag);
        query += L" and 标识=";
        query += flag;
    }

    try
    {
        _RecordsetPtr recordSet = 
            m_connection->Execute((_bstr_t)query.c_str(), NULL,
            adConnectUnspecified);
        return recordSet;
    }
    catch (_com_error e)
    {
        return NULL;
    }
}

CSQLControl::CSQLControl()
{
    if (!initConnection())
        MessageBox(NULL, L"连接数据库失败", L"", MB_OK);   
}

CSQLControl::~CSQLControl()
{
    closeConnection();
}

_RecordsetPtr CSQLControl::SelectByString(wchar_t* str)
{
    try
    {
        _RecordsetPtr recordSet = m_connection->Execute(
            (_bstr_t)str, NULL, adConnectUnspecified);

        return recordSet;
    }
    catch (_com_error)
    {
        return NULL;
    }
}

bool CSQLControl::UpdateByString(wchar_t* str)
{
    try
    {
        m_connection->Execute((_bstr_t)str, NULL, adConnectUnspecified);
        return true;
    }
    catch (_com_error e)
    {
        return false;
    }
}

bool CSQLControl::UpdatePreviewInfo(int id, int previewTime)
{
    wstring query = L"SELECT COUNT(0) FROM CHECKED_ENCODE_FILE_INFO WHERE"
        L" 图片编号 IS NOT NULL AND 歌曲编号 = ";
    wstring strID = lexical_cast<wstring>(id);
    query += strID;

    _RecordsetPtr recordSet;
    try
    {
        recordSet = m_connection->Execute(
            (_bstr_t)query.c_str(), NULL, adConnectUnspecified);
    }
    catch (_com_error e)
    {
        return false;
    }

    _variant_t t = recordSet->GetCollect(0L);
    if (t.vt == VT_NULL)
        return false;

    int count = static_cast<int>(t);
    if (count < 0)
        return false;
      
    wstring strPreviewTime = lexical_cast<wstring>(previewTime);
    if (count == 0)
    {
        query = L"INSERT INTO MV_PREVIEW_DIAGRAM(UPLOAD_TIME,INTERCEPT_TIME,"
            L"CHECK_IP_ADDRESS,MV_ID) VALUES(GETDATE(), ";
        query += strPreviewTime;
        query += L",'";
        query += GetLocalIP();
        query += L"',";
        query += strID;
        query += L")";
        try
        {
            m_connection->Execute((_bstr_t)query.c_str(), NULL, 
                                  adConnectUnspecified);
            return true;
        }
        catch (_com_error e)
        {
            return false;
        }
    }
    else
    {
        query = L"UPDATE MV_PREVIEW_DIAGRAM SET UPLOAD_TIME = GETDATE(),"
            L"INTERCEPT_TIME = ";
        query += strPreviewTime;
        query += L", CHECK_IP_ADDRESS = '";
        query += GetLocalIP();
        query += L"' WHERE MV_ID = ";
        query += strID;
        try
        {
            m_connection->Execute((_bstr_t)query.c_str(), NULL, 
                                  adConnectUnspecified);
            return true;
        }
        catch (_com_error e)
        {
            return false;
        }
    }
}

_RecordsetPtr CSQLControl::SelectByLeftListView(wchar_t* name, wchar_t* oldHash)
{
    wstring query = L"SELECT 歌曲编号,文件路径,旧哈希值,编辑重命名,是否有交错,"
        L"歌曲状态,备注,原唱音轨,歌曲类型,新哈希值,总音轨数,画质级别,"
        L"客户端ip地址,提交时间,图片生成ip地址,图片编号,截图时间,图片备注,"   
        L"图片上传时间,图片路径 FROM CHECKED_ENCODE_FILE_INFO WHERE 1 = 1";

    query += L" AND 编辑重命名 LIKE '%";
    query += name;
    query += L"%'";
    query += L" AND 旧哈希值 <> '";
    query += oldHash;
    query += L"'";

    try
    {
        _RecordsetPtr recordSet = m_connection->Execute(
            (_bstr_t)query.c_str(), NULL, adConnectUnspecified);

        return recordSet;
    }
    catch (_com_error)
    {
        return NULL;
    }
}


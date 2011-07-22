#include "sql_control.h"

#include <string>

#include <boost/lexical_cast.hpp>

#include "common.h"
#include "util.h"
#include "ini_control.h"

using std::wstring;
using std::string;
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

        wstring server = CIniControl::get()->GetServerIP();
        wstring userName = CIniControl::get()->GetUserName();
        wstring password = CIniControl::get()->GetPassword();
        wstring databaseName = CIniControl::get()->GetDatabaseName();

        wstring strSQL = L"Driver={SQL Server};Server=";
        strSQL += server;
        strSQL += L";Database=";
        strSQL += databaseName;
        strSQL += L";UID=";
        strSQL += userName;
        strSQL += L";PWD=";
        strSQL += password;

        //m_connection->PutCursorLocation(adUseClient);
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
        L"图片上传时间,图片路径,序号 FROM CHECKED_ENCODE_FILE_INFO WHERE 1 = 1";

    if ((idFrom != -1) && (idTo != -1))
    {
        wstring strIdFrom = lexical_cast<wstring>(idFrom);
        wstring strIdTo = lexical_cast<wstring>(idTo);
        query += L" and 序号 between ";
        query += strIdFrom + L" and ";
        query += strIdTo;
    }

    if (flag != -1 && flag != 3)
    {
        wstring strFlag = lexical_cast<wstring>(flag);
        query += L" and 标识=";
        query += strFlag;
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
    : m_connection(NULL)
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

void CSQLControl::StatusStoreProcForSong(
    int from, int to, int flag, int* curReviewed, int* todayReviewed, 
    int* needReview,  int* totalSong)
{
    try
    {
        m_connection->PutCursorLocation(adUseClient);
        _CommandPtr command;
        command.CreateInstance(__uuidof(Command));
        command->ActiveConnection = m_connection;
        command->CommandType = adCmdStoredProc;
        command->CommandText = _bstr_t(L"dbo.GET_SUBMIT_STATE");

        _ParameterPtr paramFromIn = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamInput, sizeof(int));
        paramFromIn->Value = _variant_t(from);
        command->Parameters->Append(paramFromIn);

        _ParameterPtr paramToIn = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamInput, sizeof(int));
        paramToIn->Value = _variant_t(to);
        command->Parameters->Append(paramToIn);
        
        string ip = WideCharToMultiByte(GetLocalIP());
        _ParameterPtr paramIPIn = command->CreateParameter(
            _bstr_t(L""), adVarChar, adParamInput, ip.length() + 1);
        paramIPIn->Value = _variant_t(ip.c_str());
        command->Parameters->Append(paramIPIn);

        _ParameterPtr paramFlagIn = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamInput, sizeof(int));
        paramFlagIn->Value = _variant_t(flag);
        command->Parameters->Append(paramFlagIn);

        _ParameterPtr paramCurReviewedOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramCurReviewedOut);

        _ParameterPtr paramTodayReviewedOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramTodayReviewedOut);
        
        _ParameterPtr paramTotalSongOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramTotalSongOut);

        _ParameterPtr paramNeedReviewOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramNeedReviewOut);
       
        _RecordsetPtr rs;
        rs = command->Execute(NULL, NULL, adCmdStoredProc);

        *curReviewed = (int)paramCurReviewedOut->Value;
        *todayReviewed = (int)paramTodayReviewedOut->Value;
        *needReview = (int)paramNeedReviewOut->Value;
        *totalSong = (int)paramTotalSongOut->Value;

        command.Detach();
        m_connection->PutCursorLocation(adUseServer);
    }
    catch (_com_error e)
    {
    	MessageBox(NULL, e.Description(), L"", MB_OK);
    }
 

}

void CSQLControl::StatusStoreProcForPic( 
    int from, int to, int* curReviewed, int* todayReviewed, 
    int* needReview, int* totalSong )
{
    try
    {
        m_connection->PutCursorLocation(adUseClient);
        _CommandPtr command;
        command.CreateInstance(__uuidof(Command));
        command->ActiveConnection = m_connection;
        command->CommandType = adCmdStoredProc;
        command->CommandText = _bstr_t(L"dbo.GET_MV_DIAGRAM_CHECK_STATE");

        string ip = WideCharToMultiByte(GetLocalIP());
        _ParameterPtr paramIPIn = command->CreateParameter(
            _bstr_t(L""), adVarChar, adParamInput, ip.length() + 1);
        paramIPIn->Value = _variant_t(ip.c_str());
        command->Parameters->Append(paramIPIn);

        _ParameterPtr paramFromIn = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamInput, sizeof(int));
        paramFromIn->Value = _variant_t(from);
        command->Parameters->Append(paramFromIn);

        _ParameterPtr paramToIn = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamInput, sizeof(int));
        paramToIn->Value = _variant_t(to);
        command->Parameters->Append(paramToIn);

        _ParameterPtr paramCurReviewedOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramCurReviewedOut);

        _ParameterPtr paramTotalSongOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramTotalSongOut);

        _ParameterPtr paramNeedReviewOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramNeedReviewOut);

        _ParameterPtr paramTodayReviewedOut = command->CreateParameter(
            _bstr_t(L""), adInteger, adParamOutput, sizeof(int));

        command->Parameters->Append(paramTodayReviewedOut);


        _RecordsetPtr rs;
        rs = command->Execute(NULL, NULL, adCmdStoredProc);

        *curReviewed = (int)paramCurReviewedOut->Value;
        *todayReviewed = (int)paramTodayReviewedOut->Value;
        *needReview = (int)paramNeedReviewOut->Value;
        *totalSong = (int)paramTotalSongOut->Value;

        command.Detach();
        m_connection->PutCursorLocation(adUseServer);
    }
    catch (_com_error e)
    {
        MessageBox(NULL, e.Description(), L"", MB_OK);
    }
}


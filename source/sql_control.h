#ifndef _SQL_CONTROL_H_
#define _SQL_CONTROL_H_

#include <string>

#include "third_party/chromium/base/singleton.h"
#import "msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

class CSQLControl : public Singleton<CSQLControl>
{
public:
    static std::wstring GetBaseQuery();

    CSQLControl();
    ~CSQLControl();

    _RecordsetPtr BaseSelect(int idFrom = -1, int idTo = -1, int flag = -1);
    _RecordsetPtr SelectByString(const wchar_t* str);
    bool UpdateByString(wchar_t* str);
    bool UpdatePreviewInfo(int id, int previewTime);
    _RecordsetPtr SelectByLeftListView(wchar_t* name, wchar_t* oldHash);
    void StatusStoreProcForSong(int from, int to, int flag, int* curReviewed, 
                         int* todayReviewed, int* needReview, int* totalSong);

    void StatusStoreProcForPic(int from, int to, int* curReviewed, 
        int* todayReviewed, int* needReview, int* totalSong);

private:
    bool initConnection();
    void closeConnection();

    _ConnectionPtr m_connection;   
};

#endif  // _SQL_CONTROL_H_
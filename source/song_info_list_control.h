#ifndef _SONG_INFO_LIST_CONTROL_H_
#define _SONG_INFO_LIST_CONTROL_H_

#include <memory>
#include <map>
#include <string>

struct SongInfo;
class CWnd;
class CMyListCtrl;
class CRect;
class SongInfoListControl
{
public:
    static int GetDisplaySwitchMessage();

    SongInfoListControl();
    ~SongInfoListControl();

    bool Create(CWnd* parent, const CRect& rect, int resourceId);
    int InsertColumn(int index, const wchar_t* heading, int format, int width,
                     int subItem);
    int AddItem(const wchar_t* text, int songId, int previewTime,
                const std::wstring& md5);
    std::wstring GetItemText(int item, int subItem);
    bool SetItemText(int item, int subItem, const wchar_t* text);
    bool DeleteAllItems();
    void SetRedraw(bool redraw);
    void ShowWindow(int showCommand);
    void MoveWindow(const CRect& rect);
    bool HasBeenCreated();
    void SelectItem(int index);
    bool IsReportView();

private:
    friend class CMyListCtrl;

    void UpdateMapping();
    int GetItemIndexBySongId(int songId);
    int GetPreviewTimeBySongId(int songId);
    void GetSongInfoBySongId(int songId, SongInfo* info);
    void SetPreviewTimeToBeBySongId(int songId, int previewTime);
    void ConfirmPreviewTime(int songId);

    std::unique_ptr<CMyListCtrl> impl_;
    std::map<int, SongInfo> songIdToItem_;
};

#endif  // _SONG_INFO_LIST_CONTROL_H_
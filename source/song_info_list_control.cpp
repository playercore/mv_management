#include "song_info_list_control.h"

#include <sstream>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "mfc_predefine.h"
#include "list_item_define.h"
#include "preview_dialog.h"
#include "util.h"
#include "preview_upload.h"
#include "field_column_mapping.h"
#include "jpeg_tool.h"
#include "sql_control.h"
#include "ini_control.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using std::pair;
using std::less;
using boost::filesystem3::path;
using boost::lexical_cast;

namespace {
enum CustomMessage { kUploadDone = WM_USER + 177 };

void LoadJPEG(CBitmap* bitmap, const wstring& jpegPath)
{
    assert(bitmap);
    void* decoded = Jpeg::LoadFromJPEGFile(jpegPath);
    if (!decoded) {
        bitmap->LoadBitmap(IDB_BITMAP_NONE);
        return;
    }

    unique_ptr<int8> autoRelease(reinterpret_cast<int8*>(decoded));
    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(decoded);
    void* bits = NULL;
    HBITMAP j = CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(header),
                                 DIB_RGB_COLORS, &bits, NULL, 0);
    if (!j || !bits)
        return;

    memcpy(
        bits, header + 1,
        (header->biWidth * header->biBitCount + 31) / 32 * 4 *
            abs(header->biHeight));
    bitmap->Attach(j);
    return;
}

bool InitImageList(CImageList* imageList, CBitmap* bitmap)
{
    assert(imageList);
    assert(bitmap);
    if (!bitmap->GetSafeHandle())
        return false;
    
    BITMAP i;
    bitmap->GetBitmap(&i);
    return !!imageList->Create(i.bmWidth, i.bmHeight, ILC_COLOR24, 0, 100);
}
}

struct SongInfo
{
    int ItemIndex;
    int ImageIndex;
    int64 PreviewTime;
    int64 PreviewTimeToBe;
};

class CMyListCtrl : public CListCtrl
{
public:
    explicit CMyListCtrl(SongInfoListControl* control);
    virtual ~CMyListCtrl();
    void NotifyListRebuilt();
    void SetPreviewTime(int songId, int previewTime);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg LRESULT OnUploadDone(WPARAM songId, LPARAM result);

private:
    friend class SongInfoListControl;

    static int __stdcall CompareFunction(LPARAM lParam1, LPARAM lParam2, 
                                         LPARAM lParamData);

    void Sort(int column, bool isAscending);
    void PlayMV(int row);
    void PreviewMV(int item);
    bool PrintMark(CBitmap* target, bool succeeded);

    SongInfoListControl* control_;
    bool isAscending_;
    int lastSortColumn_;
    CBitmap uploadDone_;
    CBitmap uploadDoneMask_;
    CBitmap uploadFailed_;
    CBitmap uploadFailedMask_;
};

namespace {
class MyUploadCallback : public UploadCallback
{
public:
    MyUploadCallback(CMyListCtrl* c) : c_(c) {}

    virtual void Done(int identifier, int result)
    {
        if (c_)
            c_->PostMessage(kUploadDone, identifier, result);
    }

private:
    CMyListCtrl* c_;
};
}

CMyListCtrl::CMyListCtrl(SongInfoListControl* control)
    : CListCtrl()
    , control_(control)
    , isAscending_(true)
    , lastSortColumn_(-1)
    , uploadDone_()
    , uploadDoneMask_()
    , uploadFailed_()
    , uploadFailedMask_()
{
}

CMyListCtrl::~CMyListCtrl()
{
}

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMyListCtrl::OnLvnItemchanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CMyListCtrl::OnColumnClick)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CMyListCtrl::OnNMCustomdraw)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CMyListCtrl::OnNMDblclk)
    ON_MESSAGE(kUploadDone, &CMyListCtrl::OnUploadDone)
END_MESSAGE_MAP()

void CMyListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    
    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED))
    {
        int selItem = pNMLV->iItem;
        TListItem item;

        for (int i = 0;i < GetHeaderCtrl()->GetItemCount(); i++)
        {
            HDITEM hdi; 
            wchar_t lpBuffer[256]; 

            hdi.mask = HDI_TEXT; 
            hdi.pszText = lpBuffer; 
            hdi.cchTextMax = 256; 
            GetHeaderCtrl()->GetItem(i, &hdi);
            CString colName = lpBuffer;

            if (colName == L"歌曲编号")
                item.id = GetItemText(selItem, i);
            else if (colName == L"旧哈希值")
                item.OldHash = GetItemText(selItem, i);
            else if (colName == L"编辑重命名")
                item.Name = GetItemText(selItem, i);          
            else if (colName == L"是否有交错")
                item.IsInterlace = GetItemText(selItem, i);  
            else if (colName == L"备注")
                item.Notes = GetItemText(selItem, i);   
            else if (colName == L"原唱音轨")
                item.TrackType = GetItemText(selItem, i); 
            else if (colName == L"歌曲类型")
                item.MVType = GetItemText(selItem, i);   
            else if (colName == L"新哈希值")
                item.NewHash = GetItemText(selItem, i);   
            else if (colName == L"画质级别")
                item.Quality = GetItemText(selItem, i);    
        } 
        ::AfxGetMainWnd()->SendMessage(UPDATESELITEM, (WPARAM)&item, selItem);
    }

    *pResult = 0;
}

void CMyListCtrl::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int column = pNMLV->iSubItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    Sort(column, column == lastSortColumn_ ? !isAscending_ : true);
    *pResult = 0;
}

void CMyListCtrl::Sort(int column, bool isAscending)
{
    lastSortColumn_ = column;
    isAscending_ = isAscending;
    SortItems(CompareFunction, reinterpret_cast<DWORD>(this));
    control_->UpdateMapping();
}

int CMyListCtrl::CompareFunction(LPARAM lParam1, LPARAM lParam2,
                                 LPARAM lParamData)
{
    CMyListCtrl* listCtrl = reinterpret_cast<CMyListCtrl*>(lParamData);
    LVFINDINFO findInfo;
    findInfo.flags = LVFI_PARAM;    // 指定查找方式
    findInfo.lParam = lParam1;
    int item1 = listCtrl->FindItem(&findInfo, -1); // 得到对应Item索引
    findInfo.lParam = lParam2;
    int item2 = listCtrl->FindItem(&findInfo, -1);

    CString text1 = listCtrl->GetItemText(item1, listCtrl->lastSortColumn_);
    CString text2 = listCtrl->GetItemText(item2, listCtrl->lastSortColumn_);

    bool r;
    if (FieldColumnMapping::get()->GetColumnIndex(
        FieldColumnMapping::kSongFullListSongId) == listCtrl->lastSortColumn_) {
        int id1 = lexical_cast<int>(text1.GetBuffer());
        int id2 = lexical_cast<int>(text2.GetBuffer());
        r = (id1 < id2);
    } else {
        r = text1 < text2;
    }

    return listCtrl->isAscending_ ? !r : r;
}

void CMyListCtrl::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        // This is the notification message for an item. We'll request
        // notifications before each subitem's prepaint stage.
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    }
    else if ( (CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage )
    {
        // This is the prepaint stage for a subitem. Here's where we set the
        // item's text and background colors. Our return value will tell 
        // Windows to draw the subitem itself, but it will use the new colors
        // we set here. 
        CString str = GetItemText(
            pLVCD->nmcd.dwItemSpec,
            FieldColumnMapping::get()->GetColumnIndex(
                FieldColumnMapping::kSongFullListNumOfTracks));
        int trackCount = _wtoi((LPTSTR)(LPCTSTR)str);
        if (trackCount >= 2)
        {
            pLVCD->clrTextBk = RGB(227,233,255);
            //pLVCD->clrText = 0xC0DCC0;
            //SetFont(m_Font, false);
        }
         else
         {
             pLVCD->clrTextBk = 16777215;
             pLVCD->clrText = 0;
         }
        //SetFont(m_Font, false);
        // Store the colors back in the NMLVCUSTOMDRAW struct.
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;
    }
}

void CMyListCtrl::OnNMDblclk(NMHDR* desc, LRESULT* result)
{
    LPNMITEMACTIVATE itemActivate = reinterpret_cast<LPNMITEMACTIVATE>(desc);
    if (::GetKeyState(VK_CONTROL) < 0) {
        const bool reportView = GetStyle() & LVS_REPORT;
        AfxGetMainWnd()->SendMessage(
            SongInfoListControl::GetDisplaySwitchMessage(), reportView, 0);

        if (reportView)
            ModifyStyle(LVS_REPORT, 0);
        else
            ModifyStyle(0, LVS_REPORT);

        *result = 0;
        return;
    }

    if (itemActivate->iItem >= 0) {
        if (GetStyle() & LVS_REPORT)
            PlayMV(itemActivate->iItem);
        else
            PreviewMV(itemActivate->iItem);
    }

    *result = 0;
}

LRESULT CMyListCtrl::OnUploadDone(WPARAM songId, LPARAM result)
{
    control_->ConfirmPreviewTime(songId);
    SongInfo info = {0};
    control_->GetSongInfoBySongId(songId, &info);

    const bool updateSucceeded =
        CSQLControl::get()->UpdatePreviewInfo(
            songId, static_cast<int>(info.PreviewTime));

    wstring md5 = GetItemText(
        info.ItemIndex,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMd5)).GetBuffer();

    CBitmap preview;
    LoadJPEG(&preview, GetMvPreviewPath() + md5 + L".jpg");

    do {
        if (!PrintMark(&preview, !result && updateSucceeded))
            break;

        CImageList* imageList = GetImageList(ILS_NORMAL);
        if (imageList) {
            if (imageList->Replace(info.ImageIndex, &preview, NULL)) {
                preview.Detach();
                RedrawItems(info.ItemIndex, info.ItemIndex);
            }
        }

        AfxGetMainWnd()->SendMessage(
            SongInfoListControl::GetPictureUploadedMessage(), 0, 0);

        return 0;
    } while (0);

    const bool succeeded = !result;
    wstringstream s;
    s << L"预览图" << songId << L"上传" << (succeeded ? L"成功" : L"失败");
    MessageBox(s.str().c_str(), succeeded ? L"信息" : L"错误",
               succeeded ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
    return 0;
}

void CMyListCtrl::PlayMV(int row)
{
    CString cmd = CIniControl::get()->GetKmplayer().c_str();
    cmd += L"KMPlayer.exe";
    CString str;
    str += L"\"";
    str += GetItemText(
        row,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListFilePath));
    str += L"\"";

    SHELLEXECUTEINFO  ShExecInfo;
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask    = NULL;
    ShExecInfo.hwnd      = NULL;
    ShExecInfo.lpVerb    = NULL;
    ShExecInfo.lpFile      = cmd;       
    ShExecInfo.lpParameters = str;
    ShExecInfo.lpDirectory    = NULL;
    //ShExecInfo.nShow          = SW_MAXIMIZE;                // 全屏显示这个程序
    ShExecInfo.hInstApp = NULL;

    BOOL r = ShellExecuteEx(&ShExecInfo);
}

void CMyListCtrl::PreviewMV(int item)
{
    path mvPath(GetItemText(
        item,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListFilePath)).GetBuffer());
    wstring md5 = GetItemText(
        item,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMd5)).GetBuffer();
    path previewPath(GetMvPreviewPath() + md5 + L".jpg");
    const int songId = GetItemData(item);
    const int previewTime = control_->GetPreviewTimeBySongId(songId);
    wstring songName = GetItemText(
        item,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListEditorRename)).GetBuffer();

    PreviewDialog dialog(this, mvPath, previewPath, previewTime, songName);
    if (IDOK == dialog.DoModal()) {
        std::shared_ptr<MyUploadCallback> callback(
            std::make_shared<MyUploadCallback>(this));

        const int64 previewTime = dialog.GetPreivewTime();
        control_->SetPreviewTimeToBeBySongId(songId,
                                             static_cast<int>(previewTime));
        PreviewUpload::Upload(previewPath, callback, songId);
    }
}

bool CMyListCtrl::PrintMark(CBitmap* target, bool succeeded)
{
    assert(target);
    CBitmap* mark;
    CBitmap* mask;
    if (succeeded) {
        if (!uploadDone_.GetSafeHandle())
            LoadNotificationImage(IDB_PNG_UPLOAD_DONE, &uploadDone_,
                                  &uploadDoneMask_);

        if (!uploadDone_.GetSafeHandle())
            return false;

        mark = &uploadDone_;
        mask = &uploadDoneMask_;
    } else {
        if (!uploadFailed_.GetSafeHandle())
            LoadNotificationImage(IDB_PNG_UPLOAD_FAILED, &uploadFailed_,
                                  &uploadFailedMask_);

        if (!uploadFailed_.GetSafeHandle())
            return false;

        mark = &uploadFailed_;
        mask = &uploadFailedMask_;
    }

    BITMAP previewInfo;
    target->GetBitmap(&previewInfo);
    const int previewWidth = previewInfo.bmWidth;
    const int previewHeight = previewInfo.bmHeight;

    BITMAP markInfo;
    if (sizeof(markInfo) != mark->GetBitmap(&markInfo))
        return false;

    const int markWidth = markInfo.bmWidth;
    const int markHeight = markInfo.bmHeight;

    CDC* dc = GetDC();
    CDC previewDc;
    previewDc.CreateCompatibleDC(dc);

    CDC markDc;
    markDc.CreateCompatibleDC(dc);

    CGdiObject* o1 = previewDc.SelectObject(target);
    CGdiObject* o2 = markDc.SelectObject(mark);

    previewDc.MaskBlt(previewWidth - markWidth - 5,
                      previewHeight - markHeight - 5, markWidth,
                      markHeight, &markDc, 0, 0, *mask, 0, 0,
                      MAKEROP4(SRCPAINT, SRCCOPY));
    markDc.SelectObject(o2);
    previewDc.SelectObject(o1);
    ReleaseDC(dc);
    return true;
}

//------------------------------------------------------------------------------
int SongInfoListControl::GetDisplaySwitchMessage()
{
    return WM_USER + 178;
}

int SongInfoListControl::GetPictureUploadedMessage()
{
    return WM_USER + 179;
}

SongInfoListControl::SongInfoListControl()
    : impl_(new CMyListCtrl(this))
    , songIdToItem_()
{
}

SongInfoListControl::~SongInfoListControl()
{
}

bool SongInfoListControl::Create(CWnd* parent, const CRect& rect,
                                 int resourceId)
{
    BOOL r = impl_->Create(
        WS_VISIBLE | WS_CHILD | LVS_REPORT | WS_VSCROLL | WS_HSCROLL |
            LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        rect, parent, resourceId);
    if (r)
        impl_->SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

    return !!r;
}

int SongInfoListControl::InsertColumn(int index, const wchar_t* heading,
                                      int format, int width, int subItem)
{
    return impl_->InsertColumn(index, heading, format, width, subItem);
}

int SongInfoListControl::AddItem(const wchar_t* text, int songId,
                                 int previewTime, const wstring& md5)
{
    // Load image.
    CBitmap bitmap;
    LoadJPEG(&bitmap, GetMvPreviewPath() + md5 + L".jpg");
    if (previewTime >= 0)
        impl_->PrintMark(&bitmap, true);

    int imageIndex = 0;
    CImageList* imageList = impl_->GetImageList(LVSIL_NORMAL);
    if (!imageList) {
        CImageList l;
        if (!InitImageList(&l, &bitmap))
            return -1;

        impl_->SetImageList(&l, LVSIL_NORMAL);
        l.Detach();
        imageList = impl_->GetImageList(LVSIL_NORMAL);
    }

    imageIndex = imageList->Add(&bitmap, reinterpret_cast<CBitmap*>(NULL));

    const int itemIndex = impl_->GetItemCount();
    const int r = impl_->InsertItem(itemIndex, text, imageIndex);
    impl_->SetItemData(itemIndex, songId);

    // Update song ID-item mapping.
    SongInfo s = { itemIndex, imageIndex, previewTime, previewTime };
    songIdToItem_.insert(pair<int, SongInfo>(songId, s));
    return r;
}

wstring SongInfoListControl::GetItemText(int item, int subItem)
{
    LVITEM itemDesc = {0};
    itemDesc.iSubItem = subItem;
    int len = 128;
    int result;
    unique_ptr<wchar_t[]> buf;
    do
    {
        len *= 2;
        buf.reset(new wchar_t[len]);
        itemDesc.cchTextMax = len;
        itemDesc.pszText = buf.get();
        result = static_cast<int>(SendMessage(
            impl_->GetSafeHwnd(), LVM_GETITEMTEXT, item,
            reinterpret_cast<LPARAM>(&itemDesc)));
    } while (result >= len - 1);
    return wstring(buf.get());
}

bool SongInfoListControl::SetItemText(int item, int subItem,
                                      const wchar_t* text)
{
    return !!impl_->SetItemText(item, subItem, text);
}

bool SongInfoListControl::DeleteAllItems()
{
    CImageList* imageList = impl_->SetImageList(NULL, ILS_NORMAL);
    if (imageList)
        imageList->DeleteImageList();

    songIdToItem_.clear();
    return !!impl_->DeleteAllItems();
}

void SongInfoListControl::SetRedraw(bool redraw)
{
    impl_->SetRedraw(redraw);
}

void SongInfoListControl::ShowWindow(int showCommand)
{
    impl_->ShowWindow(showCommand);
}

void SongInfoListControl::MoveWindow(const CRect& rect)
{
    impl_->MoveWindow(&rect);
}

bool SongInfoListControl::HasBeenCreated()
{
    return !!IsWindow(impl_->GetSafeHwnd());
}

void SongInfoListControl::SelectItem(int index)
{
    impl_->SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, 
                        LVIS_SELECTED | LVIS_FOCUSED);
}

bool SongInfoListControl::IsReportView()
{
    return (impl_->GetStyle() & LVS_REPORT);
}

void SongInfoListControl::UpdateMapping()
{
    const int itemCount = impl_->GetItemCount();
    for (int i = 0; i < itemCount; ++i) {
        int songId = impl_->GetItemData(i);
        auto iter = songIdToItem_.find(songId);
        assert(songIdToItem_.end() != iter);
        if (songIdToItem_.end() != iter)
            iter->second.ItemIndex = i;
    }
}

int SongInfoListControl::GetItemIndexBySongId(int songId)
{
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    return (songIdToItem_.end() != iter) ? iter->second.ItemIndex : 0;
}

int SongInfoListControl::GetPreviewTimeBySongId(int songId)
{
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    return (songIdToItem_.end() != iter) ?
        static_cast<int>(iter->second.PreviewTime) : 0;
}

void SongInfoListControl::GetSongInfoBySongId(int songId, SongInfo* info)
{
    assert(info);
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    if (songIdToItem_.end() != iter)
        *info = iter->second;
}

void SongInfoListControl::SetPreviewTimeToBeBySongId(int songId,
                                                     int previewTime)
{
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    if (songIdToItem_.end() != iter)
        iter->second.PreviewTimeToBe = previewTime;
}

void SongInfoListControl::ConfirmPreviewTime(int songId)
{
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    if (songIdToItem_.end() != iter)
        iter->second.PreviewTime = iter->second.PreviewTimeToBe;
}
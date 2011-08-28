#include "song_info_list_control.h"

#include <sstream>
#include <algorithm>
#include <list>
#include <functional>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/intrusive_ptr.hpp>

#include "mfc_predefine.h"
#include "list_item_define.h"
#include "preview_dialog.h"
#include "util.h"
#include "preview_upload.h"
#include "field_column_mapping.h"
#include "jpeg_tool.h"
#include "sql_control.h"
#include "ini_control.h"
#include "image_cache.h"
#include "intrusive_ptr_helper.h"
#include "task_canceler.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using std::pair;
using std::less;
using std::list;
using std::bind;
using boost::filesystem3::path;
using boost::lexical_cast;
using boost::intrusive_ptr;

namespace {
enum CustomMessage
{
    kUploadDone = WM_USER + 177,
    kJPEGLoaded = WM_USER + 178
};
enum { kDefaultPreviewTime = 6000 };

void CreateBitmapFromBuffer(CBitmap* bitmap, void* buffer)
{
    assert(bitmap);
    if (!buffer) {
        bitmap->LoadBitmap(IDB_BITMAP_NONE);
        return;
    }

    unique_ptr<int8> autoRelease(reinterpret_cast<int8*>(buffer));
    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(buffer);
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

void LoadJPEG(CBitmap* bitmap, const wstring& jpegPath)
{
    CreateBitmapFromBuffer(bitmap, Jpeg::LoadFromJPEGFile(jpegPath));
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
    int SongId;
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

    afx_msg void OnLvnItemchanged(NMHDR* desc, LRESULT* result);
    afx_msg void OnColumnClick(NMHDR* desc, LRESULT* result);
    afx_msg void OnNMCustomdraw(NMHDR* desc, LRESULT* result);
    afx_msg void OnNMDblclk(NMHDR* desc, LRESULT* result);
    afx_msg void OnRightClicked(NMHDR* desc, LRESULT* result);
    afx_msg LRESULT OnUploadDone(WPARAM songId, LPARAM result);
    afx_msg LRESULT OnMessageJPEGLoaded(WPARAM songId, LPARAM image);
    void Acknowledge(int songId, int result);

private:
    friend class SongInfoListControl;

    static int __stdcall CompareFunction(LPARAM lParam1, LPARAM lParam2, 
                                         LPARAM lParamData);

    void Sort(int column, bool isAscending);
    void PlayMV(int row);
    void PreviewMV(int item);
    bool PrintMark(CBitmap* target, bool succeeded);
    bool IsReportView() { return GetStyle() & LVS_REPORT; }
    void LoadJPEGIfNeeded(int item);
    void OnJPEGLoaded(int songId, void* image);

    SongInfoListControl* control_;
    bool isAscending_;
    int lastSortColumn_;
    CBitmap uploadDone_;
    CBitmap uploadDoneMask_;
    CBitmap uploadFailed_;
    CBitmap uploadFailedMask_;
    list<intrusive_ptr<CTaskCanceler>> pendingLoadTasks_;
    bool styleChanging_;
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
    , styleChanging_(false)
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
    ON_NOTIFY_REFLECT(NM_RCLICK, &CMyListCtrl::OnRightClicked)
    ON_MESSAGE(kUploadDone, &CMyListCtrl::OnUploadDone)
    ON_MESSAGE(kJPEGLoaded, &CMyListCtrl::OnMessageJPEGLoaded)
END_MESSAGE_MAP()

void CMyListCtrl::OnLvnItemchanged(NMHDR* desc, LRESULT* result)
{
    LPNMLISTVIEW specDesc = reinterpret_cast<LPNMLISTVIEW>(desc);
    if ((specDesc->uChanged & LVIF_STATE) &&
        (specDesc->uNewState & LVIS_SELECTED)) {
        int selItem = specDesc->iItem;
        TListItem item;
        for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); i++) {
            HDITEM itemDesc; 
            wchar_t lpBuffer[256]; 

            itemDesc.mask = HDI_TEXT; 
            itemDesc.pszText = lpBuffer; 
            itemDesc.cchTextMax = 256; 
            GetHeaderCtrl()->GetItem(i, &itemDesc);
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
        AfxGetMainWnd()->SendMessage(UPDATESELITEM,
                                     reinterpret_cast<WPARAM>(&item), selItem);
    }

    *result = 0;
}

void CMyListCtrl::OnColumnClick(NMHDR* desc, LRESULT* result)
{
    LPNMLISTVIEW specDesc = reinterpret_cast<LPNMLISTVIEW>(desc);
    const int column = specDesc->iSubItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    Sort(column, column == lastSortColumn_ ? !isAscending_ : true);
    *result = 0;
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

void CMyListCtrl::OnNMCustomdraw(NMHDR* desc, LRESULT* result)
{
    // Take the default processing unless we set this to something else below.
    *result = CDRF_DODEFAULT;

    if (styleChanging_)
        return;

    NMLVCUSTOMDRAW* drawInfo = reinterpret_cast<NMLVCUSTOMDRAW*>(desc);

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if (CDDS_PREPAINT == drawInfo->nmcd.dwDrawStage) {
        *result = CDRF_NOTIFYITEMDRAW;
    } else if (CDDS_ITEMPREPAINT == drawInfo->nmcd.dwDrawStage) {
        RECT itemRect;
        GetItemRect(drawInfo->nmcd.dwItemSpec, &itemRect, LVIR_BOUNDS);
        if (itemRect.bottom >= 0) {
            if (IsReportView()) {
                // This is the notification message for an item. We'll request
                // notifications before each subitem's prepaint stage.
                *result = CDRF_NOTIFYSUBITEMDRAW;
            } else { // Icon view.
//                 RECT itemRect;
//                 GetItemRect(drawInfo->nmcd.dwItemSpec, &itemRect, LVIR_BOUNDS);
// 
//                 if (itemRect.bottom >= 0)
                    LoadJPEGIfNeeded(drawInfo->nmcd.dwItemSpec);
            }
        }
    } else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) ==
        drawInfo->nmcd.dwDrawStage) {

        // This is the prepaint stage for a subitem. Here's where we set the
        // item's text and background colors. Our return value will tell 
        // Windows to draw the subitem itself, but it will use the new colors
        // we set here. 
        CString str = GetItemText(
            drawInfo->nmcd.dwItemSpec,
            FieldColumnMapping::get()->GetColumnIndex(
                FieldColumnMapping::kSongFullListNumOfTracks));
        int trackCount = lexical_cast<int>(str.GetBuffer());
        if (trackCount >= 2) {
            drawInfo->clrTextBk = RGB(227, 233, 255);
        } else {
             drawInfo->clrTextBk = 16777215;
             drawInfo->clrText = 0;
        }

        // Store the colors back in the NMLVCUSTOMDRAW struct.
        // Tell Windows to paint the control itself.
        *result = CDRF_DODEFAULT;
    }
}

void CMyListCtrl::OnNMDblclk(NMHDR* desc, LRESULT* result)
{
    LPNMITEMACTIVATE itemActivate = reinterpret_cast<LPNMITEMACTIVATE>(desc);
    if (::GetKeyState(VK_CONTROL) < 0) {
        const bool reportView = IsReportView();
        AfxGetMainWnd()->SendMessage(
            SongInfoListControl::GetDisplaySwitchMessage(), reportView, 0);

        styleChanging_ = true;
        if (reportView)
            ModifyStyle(LVS_REPORT, 0);
        else
            ModifyStyle(0, LVS_REPORT);

        styleChanging_ = false;
        *result = 0;
        return;
    }

    if (itemActivate->iItem >= 0) {
        if (IsReportView())
            PlayMV(itemActivate->iItem);
        else
            PreviewMV(itemActivate->iItem);
    }

    *result = 0;
}

void CMyListCtrl::OnRightClicked(NMHDR* desc, LRESULT* result)
{
    NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(desc);
    *result = 0;
    if (IsReportView() || (itemActivate->iItem < 0))
        return;

    const SongInfo* songInfo =
        reinterpret_cast<SongInfo*>(GetItemData(itemActivate->iItem));
    assert(songInfo);
    const int songId = songInfo->SongId;
    const int previewTime = control_->GetPreviewTimeBySongId(songId);
    if (previewTime >= 0) {
        MessageBox(L"已审核", L"提示", MB_ICONINFORMATION | MB_OK);
        return;
    }

    CMenu m;
    m.LoadMenu(IDR_MENU_PREVIEW);
    CMenu* s = m.GetSubMenu(0);
    if (!s)
        return;

    CPoint pos;
    GetCursorPos(&pos);
    const int r = s->TrackPopupMenu(
        TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pos.x, pos.y, this);
    if (r != ID_PREVIEW_ACKNOWLEDGE)
        return;

    control_->SetPreviewTimeToBeBySongId(songId, kDefaultPreviewTime);
    Acknowledge(songId, 0);
}

LRESULT CMyListCtrl::OnUploadDone(WPARAM songId, LPARAM result)
{
    Acknowledge(songId, result);
    return 0;
}

LRESULT CMyListCtrl::OnMessageJPEGLoaded(WPARAM songId, LPARAM image)
{
    SongInfo info;
    if (!control_->GetSongInfoBySongId(songId, &info)) {
        // this ID may no longer available(e.g. "search" button pressed).
        return 0;
    }

    // The loading request maybe applied more than once. We don't have to
    // process each of them.
    if (info.ImageIndex >= 0)
        return 0;

    CBitmap bitmap;
    CreateBitmapFromBuffer(&bitmap, reinterpret_cast<void*>(image));
    if (info.PreviewTime >= 0)
        PrintMark(&bitmap, true);

    CImageList* imageList = GetImageList(ILS_NORMAL);
    if (imageList) {
        const int imageIndex =
            imageList->Add(&bitmap, reinterpret_cast<CBitmap*>(NULL));

        SongInfo* songInfo =
            reinterpret_cast<SongInfo*>(GetItemData(info.ItemIndex));
        assert(songInfo);

        info.ImageIndex = imageIndex;
        songInfo->ImageIndex = imageIndex;

        LVITEM itemDesc = {0};
        itemDesc.mask = LVIF_IMAGE;
        itemDesc.iItem = info.ItemIndex;
        itemDesc.iImage = imageIndex;
        SetItem(&itemDesc);
    }

    return 0;
}

void CMyListCtrl::Acknowledge(int songId, int result)
{
    control_->ConfirmPreviewTime(songId);
    SongInfo info = {0};

    // TODO: check return value - this ID may no longer available.
    control_->GetSongInfoBySongId(songId, &info);

    const bool updateSucceeded =
        CSQLControl::get()->UpdatePreviewInfo(
            songId, static_cast<int>(info.PreviewTime));

    wstring md5 = GetItemText(
        info.ItemIndex,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMd5)).GetBuffer();

    // Note that it's not possible that reusing any of the bitmaps already in
    // the image list.
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

        return;
    } while (0);

    const bool succeeded = (!result && updateSucceeded);
    wstringstream s;
    s << L"预览图" << songId << L"审核" << (succeeded ? L"成功" : L"失败");
    MessageBox(s.str().c_str(), succeeded ? L"信息" : L"错误",
               succeeded ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
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
    const SongInfo* songInfo = reinterpret_cast<SongInfo*>(GetItemData(item));
    assert(songInfo);
    const int songId = songInfo->SongId;
    const int previewTime = control_->GetPreviewTimeBySongId(songId);
    wstring songName = GetItemText(
        item,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListEditorRename)).GetBuffer();

    PreviewDialog dialog(this, mvPath, previewPath,
                         previewTime < 0 ? kDefaultPreviewTime : previewTime,
                         songName);
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

void CMyListCtrl::LoadJPEGIfNeeded(int item)
{
    const SongInfo* songInfo = reinterpret_cast<SongInfo*>(GetItemData(item));
    assert(songInfo);
    if (!songInfo)
        return;
    
    if (songInfo->ImageIndex >= 0)
        return;

    wstring md5 = GetItemText(
        item,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMd5)).GetBuffer();

    const size_t maxNumPendingTasks = 50;
    if (pendingLoadTasks_.size() > maxNumPendingTasks) {
        pendingLoadTasks_.front()->Cancel();
        pendingLoadTasks_.pop_front();
    }

    auto callback = bind(&CMyListCtrl::OnJPEGLoaded, this, songInfo->SongId,
                         std::placeholders::_1);
    intrusive_ptr<CTaskCanceler> c(new CTaskCanceler);
    ImageCache::get()->LoadJPEG(
        GetMvPreviewPath() + md5 + L".jpg", c.get(), callback);
    pendingLoadTasks_.push_back(c);
}

void CMyListCtrl::OnJPEGLoaded(int songId, void* image)
{
    // TODO: memory leak occurs when the window is destroyed.
    ::PostMessage(GetSafeHwnd(), kJPEGLoaded, songId,
                  reinterpret_cast<LPARAM>(image));
}

//------------------------------------------------------------------------------
int SongInfoListControl::GetDisplaySwitchMessage()
{
    return WM_USER + 187;
}

int SongInfoListControl::GetPictureUploadedMessage()
{
    return WM_USER + 188;
}

SongInfoListControl::SongInfoListControl()
    : impl_(new CMyListCtrl(this))
    , songIdToItem_()
    , loadFailureUploaded_(new CBitmap)
    , loadFailure_(new CBitmap)
{
    loadFailureUploaded_->LoadBitmap(IDB_BITMAP_NONE);
    loadFailure_->LoadBitmap(IDB_BITMAP_NONE);
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

    if (loadFailureUploaded_->GetSafeHandle() && r)
        impl_->PrintMark(loadFailureUploaded_.get(), true);

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
    CBitmap* bitmap = (previewTime >= 0) ?
        loadFailureUploaded_.get() : loadFailure_.get();
    CImageList* imageList = impl_->GetImageList(LVSIL_NORMAL);
    if (!imageList) {
        CImageList l;
        if (!InitImageList(&l, bitmap))
            return -1;

        impl_->SetImageList(&l, LVSIL_NORMAL);
        l.Detach();
        imageList = impl_->GetImageList(LVSIL_NORMAL);
        imageList->Add(loadFailure_.get(), reinterpret_cast<CBitmap*>(NULL));
    }

    const int itemIndex = impl_->GetItemCount();
    const int r = impl_->InsertItem(itemIndex, text, 0);

    // Update song ID-item mapping.
    SongInfo d = { songId, itemIndex, -1, previewTime, previewTime };
    songIdToItem_.insert(pair<int, SongInfo>(songId, d));
    unique_ptr<SongInfo> info(new SongInfo(d));
    impl_->SetItemData(itemIndex,
                       reinterpret_cast<DWORD_PTR>(info.get()));
    songInfoManagement_.push_back(std::move(info));
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
    songInfoManagement_.clear();
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
    return impl_->IsReportView();
}

void SongInfoListControl::UpdateMapping()
{
    const int itemCount = impl_->GetItemCount();
    for (int i = 0; i < itemCount; ++i) {
        const SongInfo* songInfo =
            reinterpret_cast<SongInfo*>(impl_->GetItemData(i));
        assert(songInfo);
        const int songId = songInfo->SongId;
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

bool SongInfoListControl::GetSongInfoBySongId(int songId, SongInfo* info)
{
    assert(info);
    auto iter = songIdToItem_.find(songId);
    assert(songIdToItem_.end() != iter);
    if (songIdToItem_.end() == iter)
        return false;

    *info = iter->second;
    return true;
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
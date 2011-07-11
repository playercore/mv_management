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
#include "png_tool.h"
#include "field_column_mapping.h"
#include "jpeg_tool.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using std::pair;
using std::less;
using boost::filesystem3::path;
using boost::lexical_cast;

namespace {
enum CustomMessage { kUploadDone = WM_USER + 177 };

void LoadNotificationImage(int id, CBitmap* target, CBitmap* mask)
{
    assert(!target->GetSafeHandle());
    assert(!mask->GetSafeHandle());
    HRSRC res = FindResource(NULL, MAKEINTRESOURCE(id), L"PNG");
    if (!res)
        return;

    HGLOBAL handle = LoadResource(NULL, res);
    unique_ptr<void, void (__stdcall *)(void*)> resData(
        LockResource(handle),
        reinterpret_cast<void (__stdcall *)(void*)>(UnlockResource));
    void* decoded =
        Png::LoadFromMemory(resData.get(), SizeofResource(NULL, res));
    if (!decoded)
        return;

    unique_ptr<int8> autoRelease(reinterpret_cast<int8*>(decoded));
    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(decoded);
    void* bits = NULL;
    HBITMAP h = CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(header),
                                 DIB_RGB_COLORS, &bits, NULL, 0);
    if (!h || !bits)
        return;

    int width = header->biWidth;
    int height = abs(header->biHeight);
    memcpy(bits,
           header + 1, (width * header->biBitCount + 31) / 32 * 4 * height);
    target->Attach(h);

    // Prepare mask.
    BITMAP targetInfo;
    if (sizeof(targetInfo) != target->GetBitmap(&targetInfo))
        return;

    const int targetWidth = targetInfo.bmWidth;
    const int targetHeight = targetInfo.bmHeight;

    mask->CreateBitmap(targetWidth, targetHeight, 1, 1, NULL);

    CDC* dc = CDC::FromHandle(GetDC(NULL));
    if (!dc)
        return;

    CDC targetDc;
    targetDc.CreateCompatibleDC(dc);

    CDC maskDc;
    maskDc.CreateCompatibleDC(dc);

    CGdiObject* o1 = targetDc.SelectObject(target);
    CGdiObject* o2 = maskDc.SelectObject(mask);

    targetDc.SetBkColor(RGB(255, 0, 255));
    maskDc.BitBlt(0, 0, targetWidth, targetHeight, &targetDc, 0, 0, SRCCOPY);

    targetDc.SetTextColor(RGB(255, 255, 255));
    targetDc.SetBkColor(0);
    targetDc.BitBlt(0, 0, targetWidth, targetHeight, &maskDc, 0, 0, SRCAND);

    maskDc.SelectObject(o2);
    targetDc.SelectObject(o1);
}

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
    static int __stdcall CompareFunction(LPARAM lParam1, LPARAM lParam2, 
                                         LPARAM lParamData);

    void Sort(int column, bool isAscending);
    void PlayMV(int row);
    void PreviewMV(int item);

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
                item.MusicType = GetItemText(selItem, i);   
            else if (colName == L"新哈希值")
                item.NewHash = GetItemText(selItem, i);   
            else if (colName == L"画质级别")
                item.Quality = GetItemText(selItem, i);    
        } 
        ::AfxGetMainWnd()->SendMessage(UPDATESELITEM, (WPARAM)&item, 0);
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

    wstring text1 =
        listCtrl->control_->GetItemText(item1, listCtrl->lastSortColumn_);
    wstring text2 =
        listCtrl->control_->GetItemText(item2, listCtrl->lastSortColumn_);

    bool r;
    if (FieldColumnMapping::get()->GetColumnIndex(
        FieldColumnMapping::kSongFullListSongId) == listCtrl->lastSortColumn_) {
        int id1 = lexical_cast<int>(text1);
        int id2 = lexical_cast<int>(text2);
        r = (id1 < id2);
    } else {
        r = less<wstring>()(text1, text2);
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
        if (GetStyle() & LVS_REPORT)
            ModifyStyle(LVS_REPORT, 0);
        else
            ModifyStyle(0, LVS_REPORT);

        *result = 0;
        return;
    }

    if (GetStyle() & LVS_REPORT)
        PlayMV(itemActivate->iItem);
    else
        PreviewMV(itemActivate->iItem);

    *result = 0;
}

LRESULT CMyListCtrl::OnUploadDone(WPARAM songId, LPARAM result)
{
    control_->ConfirmPreviewTime(songId);
    const int itemIndex = control_->GetItemIndexBySongId(songId);
    if (itemIndex < 0)
        return 0;

    wstring md5 = control_->GetItemText(
        itemIndex,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMd5));

    CBitmap preview;
    LoadJPEG(&preview, GetMvPreviewPath() + md5 + L".jpg");

    do {
        CBitmap* mark;
        CBitmap* mask;
        if (!result) {
            if (!uploadDone_.GetSafeHandle())
                LoadNotificationImage(IDB_PNG_UPLOAD_DONE, &uploadDone_,
                                      &uploadDoneMask_);

            if (!uploadDone_.GetSafeHandle())
                break;

            mark = &uploadDone_;
            mask = &uploadDoneMask_;
        } else {
            if (!uploadFailed_.GetSafeHandle())
                LoadNotificationImage(IDB_PNG_UPLOAD_FAILED, &uploadFailed_,
                                      &uploadFailedMask_);

            if (!uploadFailed_.GetSafeHandle())
                break;

            mark = &uploadFailed_;
            mask = &uploadFailedMask_;
        }

        BITMAP previewInfo;
        preview.GetBitmap(&previewInfo);
        const int previewWidth = previewInfo.bmWidth;
        const int previewHeight = previewInfo.bmHeight;

        BITMAP markInfo;
        if (sizeof(markInfo) != mark->GetBitmap(&markInfo))
            break;

        const int markWidth = markInfo.bmWidth;
        const int markHeight = markInfo.bmHeight;

        CDC* dc = GetDC();
        CDC previewDc;
        previewDc.CreateCompatibleDC(dc);

        CDC markDc;
        markDc.CreateCompatibleDC(dc);

        CDC maskDc;
        maskDc.CreateCompatibleDC(dc);

        CGdiObject* o1 = previewDc.SelectObject(&preview);
        CGdiObject* o2 = markDc.SelectObject(mark);
        CGdiObject* o3 = maskDc.SelectObject(mask);

        previewDc.MaskBlt(previewWidth - markWidth - 5,
                          previewHeight - markHeight - 5, markWidth,
                          markHeight, &markDc, 0, 0, *mask, 0, 0,
                          MAKEROP4(SRCPAINT, SRCCOPY));
        maskDc.SelectObject(o3);
        markDc.SelectObject(o2);
        previewDc.SelectObject(o1);
        ReleaseDC(dc);

        CImageList* imageList = GetImageList(ILS_NORMAL);
        if (imageList) {
                if (imageList->Replace(itemIndex, &preview, NULL)) {
                preview.Detach();
                RedrawItems(itemIndex, itemIndex);
            }
        }

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
    wstring path;
    wchar_t curPath[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH,curPath);
    path = curPath;
    path += L"\\config.ini";
    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"setup", L"kmpPath", L"", buf, 32767, 
        path.c_str());
    CString cmd = buf;
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
    path mvPath(control_->GetItemText(item, 1));
    wstring md5 = control_->GetItemText(item, 2);
    path previewPath(GetMvPreviewPath() + md5 + L".jpg");
    const int songId = GetItemData(item);
    const int previewTime = control_->GetPreviewTimeBySongId(songId);

    PreviewDialog dialog(this, mvPath, previewPath, previewTime);
    if (IDOK == dialog.DoModal()) {
        std::shared_ptr<MyUploadCallback> callback(
            std::make_shared<MyUploadCallback>(this));

        const int64 previewTime = dialog.GetPreivewTime();
        control_->SetPreviewTimeToBeBySongId(songId,
                                             static_cast<int>(previewTime));
        PreviewUpload::Upload(previewPath, callback, songId);
    }
}

//------------------------------------------------------------------------------
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
        WS_VISIBLE | WS_BORDER |WS_CHILD | LVS_REPORT | WS_VSCROLL |
            WS_HSCROLL | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
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
    SongInfo s = { itemIndex, previewTime, previewTime };
    songIdToItem_.insert(pair<int, SongInfo>(songId, s));
    return r;
}

wstring SongInfoListControl::GetItemText(int item, int subItem)
{
    LVITEM itemDesc = {0};
    itemDesc.iSubItem = subItem;
    wstring str;
    int len = 128;
    int result;
    do
    {
        len *= 2;
        str.resize(len);
        itemDesc.cchTextMax = len;
        itemDesc.pszText = &str[0];
        result = static_cast<int>(SendMessage(
            impl_->GetSafeHwnd(), LVM_GETITEMTEXT, item,
            reinterpret_cast<LPARAM>(&itemDesc)));
    } while (result >= len - 1);
    return str;
}

bool SongInfoListControl::SetItemText(int item, int subItem,
                                      const wchar_t* text)
{
    return !!impl_->SetItemText(item, subItem, text);
}

bool SongInfoListControl::DeleteAllItems()
{
    CImageList* imageList = impl_->GetImageList(ILS_NORMAL);
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
    return !!impl_->GetSafeHwnd();
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
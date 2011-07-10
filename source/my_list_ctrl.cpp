// my_list_ctrl.cpp : ʵ���ļ�
//
#include <string>
#include <sstream>
#include <memory>

#include <boost/filesystem.hpp>

#include "mfc_predefine.h"
#include "my_list_ctrl.h"
#include "list_item_define.h"
#include "common.h"
#include "afxdb.h"
#include "preview_dialog.h"
#include "util.h"
#include "preview_upload.h"
#include "png_tool.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using boost::filesystem3::path;

namespace {
enum CustomMessage { kUploadDone = WM_USER + 177 };

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

void loadNotificationImage(int id, CBitmap* target)
{
    assert(!target->GetSafeHandle());
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
}
}

IMPLEMENT_DYNAMIC(CMyListCtrl, CListCtrl)
CMyListCtrl::CMyListCtrl()
    : m_isAscending(true)
    , m_sortColumn(-1)
    , m_trackCountCol(-1)
    , m_pathCol(-1)
    , m_uploadDone()
    , m_uploadFailed()
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

// CMyListCtrl ��Ϣ�������

void CMyListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    
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

            if (colName == L"�������")
                item.id = GetItemText(selItem, i);
            else if (colName == L"�ɹ�ϣֵ")
                item.OldHash = GetItemText(selItem, i);
            else if (colName == L"�༭������")
                item.Name = GetItemText(selItem, i);          
            else if (colName == L"�Ƿ��н���")
                item.IsInterlace = GetItemText(selItem, i);  
            else if (colName == L"��ע")
                item.Notes = GetItemText(selItem, i);   
            else if (colName == L"ԭ������")
                item.TrackType = GetItemText(selItem, i); 
            else if (colName == L"��������")
                item.MusicType = GetItemText(selItem, i);   
            else if (colName == L"�¹�ϣֵ")
                item.NewHash = GetItemText(selItem, i);   
            else if (colName == L"���ʼ���")
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
    sort(column, column == m_sortColumn ? !m_isAscending : TRUE );

    *pResult = 0;
}

void CMyListCtrl::sort(int column, bool isAscending)
{
    m_sortColumn = column;
    m_isAscending = isAscending;
    SortItems(compareFunction, reinterpret_cast<DWORD>(this));
}

int CALLBACK CMyListCtrl::compareFunction(LPARAM lParam1, LPARAM lParam2, 
                                          LPARAM lParamData)
{
    CMyListCtrl* listCtrl = reinterpret_cast<CMyListCtrl*>(lParamData);
    int item1;
    int item2;  
    LVFINDINFO findInfo;
    findInfo.flags = LVFI_PARAM;    // ָ�����ҷ�ʽ
    findInfo.lParam = lParam1;
    item1 = listCtrl->FindItem(&findInfo, -1); // �õ���ӦItem����
    findInfo.lParam = lParam2;
    item2 = listCtrl->FindItem(&findInfo, -1);

    CString strItem1 = listCtrl->GetItemText(item1, listCtrl->m_sortColumn);
    CString strItem2 = listCtrl->GetItemText(item2, listCtrl->m_sortColumn);

    if(IsNumber(strItem1) && IsNumber(strItem2))
        return listCtrl->m_isAscending ? 
            NumberCompare(strItem1, strItem2) : NumberCompare(strItem2, strItem1);
    else
        return listCtrl->m_isAscending ? 
            strItem1 < strItem2 : strItem1 > strItem2;
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
        CString str = GetItemText(pLVCD->nmcd.dwItemSpec, m_trackCountCol);
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

void CMyListCtrl::SetTrackCountCol(int col)
{
    m_trackCountCol = col;
}

void CMyListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    if(::GetKeyState(VK_CONTROL) < 0)
    {
   
        if (GetStyle() & LVS_REPORT)
            ModifyStyle(LVS_REPORT, 0);
        else
            ModifyStyle(0, LVS_REPORT);

        *pResult = 0;
        return;
    }

    if (GetStyle() & LVS_REPORT)
        playMV(pNMItemActivate->iItem);
    else
        previewMV(pNMItemActivate->iItem);

    *pResult = 0;
}

void CMyListCtrl::SetPathCol(int col)
{
    m_pathCol = col;
}

#include <fstream>
using std::ofstream;
void SaveToBMPFile(char* data, int width, int height)
{
    const int dataSize = ((width * 3 + 3) / 4) * 4 * height;

    BITMAPFILEHEADER fileHeader = {0};
    BITMAPINFOHEADER infoHeader = {0};

    fileHeader.bfType = 'MB';
    fileHeader.bfSize = dataSize + sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfOffBits = sizeof(fileHeader);

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = dataSize;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    ofstream bmpFile(L"d:/shit.bmp", std::ios_base::binary);
    bmpFile.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    bmpFile.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    bmpFile.write(data, dataSize);
//     for (int y = 0; y < height; ++y) {
//         char* data = reinterpret_cast<char*>(
//             frame->data[0] + y * frame->linesize[0]);
// 
//         for (int x = 0; x < frame->linesize[0] - 3; x += 3)
//             std::swap(data[x], data[x + 1]);
// 
//         bmpFile.write(data, frame->width * 3);
//     }
}

void MyCreateBitmap(CBitmap* b, char* data, int width, int height)
{
    const int dataSize = (width * 3 + 3) / 4 * 4 * height;
    BITMAPINFOHEADER header = {0};
    header.biSize = sizeof(header);
    header.biWidth = width;
    header.biHeight = -height;
    header.biPlanes = 1;
    header.biBitCount = 24;
    header.biCompression = BI_RGB;
    header.biSizeImage = dataSize;

    void* bits = NULL;
    HBITMAP h = CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&header),
                                 DIB_RGB_COLORS, &bits, NULL, 0);
    if (!h || !bits)
        return;

    memcpy(bits, data, dataSize);
    b->Attach(h);
}

LRESULT CMyListCtrl::OnUploadDone(WPARAM w, LPARAM l)
{
    do {
        CImageList* imageList = GetImageList(LVSIL_NORMAL);
        if (!imageList)
            break;

        IMAGEINFO info;
        if (!imageList->GetImageInfo(w, &info))
            break;

        CBitmap* mark;
        if (!l) {
            if (!m_uploadDone.GetSafeHandle())
                loadNotificationImage(IDB_PNG_UPLOAD_DONE, &m_uploadDone);

            if (!m_uploadDone.GetSafeHandle())
                break;

            mark = &m_uploadDone;
        } else {
            if (!m_uploadFailed.GetSafeHandle())
                loadNotificationImage(IDB_PNG_UPLOAD_FAILED, &m_uploadFailed);

            if (!m_uploadFailed.GetSafeHandle())
                break;

            mark = &m_uploadFailed;
        }

        CBitmap* preview = CBitmap::FromHandle(info.hbmImage);
        const int previewWidth = info.rcImage.right - info.rcImage.left;
        const int previewHeight = info.rcImage.bottom - info.rcImage.top;

        BITMAP markInfo;
        if (sizeof(markInfo) != mark->GetBitmap(&markInfo))
            break;

        const int markWidth = markInfo.bmWidth;
        const int markHeight = markInfo.bmHeight;

        CBitmap mask;
        mask.CreateBitmap(markWidth, markHeight, 1, 1, NULL);

        BITMAP previewInfo;
        preview->GetBitmap(&previewInfo);
        const int bufSize = previewInfo.bmWidthBytes * previewHeight;
        unique_ptr<char[]> buf(new char[bufSize]);
        preview->GetBitmapBits(bufSize, buf.get());
        CBitmap copy;
        previewInfo.bmBits = buf.get();
        previewInfo.bmHeight = previewHeight;
        MyCreateBitmap(&copy, buf.get(), previewWidth, previewHeight);

        CDC* dc = GetDC();
        CDC previewDc;
        previewDc.CreateCompatibleDC(dc);

        CDC markDc;
        markDc.CreateCompatibleDC(dc);

        CDC maskDc;
        maskDc.CreateCompatibleDC(dc);

        CGdiObject* o1 = previewDc.SelectObject(&copy);
        CGdiObject* o2 = markDc.SelectObject(mark);
        CGdiObject* o3 = maskDc.SelectObject(&mask);

        markDc.SetBkColor(RGB(255, 0, 255));
        maskDc.BitBlt(0, 0, markWidth, markHeight, &markDc, 0, 0, SRCCOPY);

        markDc.SetTextColor(RGB(255, 255, 255));
        markDc.SetBkColor(0);
        markDc.BitBlt(0, 0, markWidth, markHeight, &maskDc, 0, 0, SRCAND);

        previewDc.MaskBlt(previewWidth - markWidth - 5,
                          previewHeight - markHeight - 5, markWidth,
                          markHeight, &markDc, 0, 0, mask, 0, 0,
                          MAKEROP4(SRCPAINT, SRCCOPY));
        maskDc.SelectObject(o3);
        markDc.SelectObject(o2);
        previewDc.SelectObject(o1);
        ReleaseDC(dc);

        imageList->Replace(w, &copy, NULL);
        RedrawItems(w, w);
        return 0;
    } while (0);

    int id = w;
    bool succeeded = !l;
    wstringstream s;
    s << L"Ԥ��ͼ" << w << L"�ϴ�" << (succeeded ? L"�ɹ�" : L"ʧ��");
    MessageBox(s.str().c_str(), succeeded ? L"��Ϣ" : L"����",
               succeeded ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
    return 0;
}

void CMyListCtrl::playMV(int row)
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
    str += GetItemText(row, m_pathCol);
    str += L"\"";

    SHELLEXECUTEINFO  ShExecInfo;
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask    = NULL;
    ShExecInfo.hwnd      = NULL;
    ShExecInfo.lpVerb    = NULL;
    ShExecInfo.lpFile      = cmd;       
    ShExecInfo.lpParameters = str;
    ShExecInfo.lpDirectory    = NULL;
    //ShExecInfo.nShow          = SW_MAXIMIZE;                // ȫ����ʾ�������
    ShExecInfo.hInstApp = NULL;

    BOOL r = ShellExecuteEx(&ShExecInfo);
}

void CMyListCtrl::previewMV(int item)
{
    CString text1 = GetItemText(item, 1);
    CString text2 = GetItemText(item, 2);
    path mvPath(text1.GetBuffer());
    wstring md5(text2.GetBuffer());
    path previewPath(GetMvPreviewPath() + md5 + L".jpg");
    PreviewDialog dialog(this, mvPath, previewPath, 6000);
    if (IDOK == dialog.DoModal()) {
        std::shared_ptr<MyUploadCallback> c(
            std::make_shared<MyUploadCallback>(this));
        PreviewUpload::Upload(previewPath, c, item);
    }
}
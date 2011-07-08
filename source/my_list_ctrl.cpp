// my_list_ctrl.cpp : 实现文件
//
#include <string>

#include "mfc_predefine.h"
#include "my_list_ctrl.h"
#include "list_item_define.h"
#include "common.h"
#include "afxdb.h"

using std::wstring;
    
// CMyListCtrl

IMPLEMENT_DYNAMIC(CMyListCtrl, CListCtrl)

CMyListCtrl::CMyListCtrl()
    : m_isAscending(true)
    , m_sortColumn(-1)
    , m_trackCountCol(-1)
    , m_pathCol(-1)
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
END_MESSAGE_MAP()

// CMyListCtrl 消息处理程序

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
    findInfo.flags = LVFI_PARAM;    // 指定查找方式
    findInfo.lParam = lParam1;
    item1 = listCtrl->FindItem(&findInfo, -1); // 得到对应Item索引
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
    // TODO: 在此添加控件通知处理程序代码
    if(::GetKeyState(VK_CONTROL) < 0)
    {
   
        if (GetStyle() & LVS_REPORT)
            ModifyStyle(LVS_REPORT, 0);
        else
            ModifyStyle(0, LVS_REPORT);

        *pResult = 0;
        return;
    }


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
    str += GetItemText(pNMItemActivate->iItem, m_pathCol);
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

    *pResult = 0;
}

void CMyListCtrl::SetPathCol(int col)
{
    m_pathCol = col;
}

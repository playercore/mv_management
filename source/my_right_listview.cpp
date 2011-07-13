#include "my_right_listview.h"

#include <string>

#include "list_item_define.h"
#include "common.h"
#include "field_column_mapping.h"

using std::wstring;

extern bool IsNumber(CString str);
extern int NumberCompare(CString str1, CString str2);

IMPLEMENT_DYNCREATE(CMyRightListView, CListView)
    
CMyRightListView::CMyRightListView()
    : m_isAscending(true)
    , m_sortColumn(-1)
{

}

CMyRightListView::~CMyRightListView()
{

}

BEGIN_MESSAGE_MAP(CMyRightListView, CListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMyRightListView::OnLvnItemchanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CMyRightListView::OnColumnClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CMyRightListView::OnNMDblclk)
END_MESSAGE_MAP()

void CMyRightListView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // TODO: 在此添加控件通知处理程序代码
    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED))
    {
        int selItem = pNMLV->iItem;
        CListCtrl& listCtrl = GetListCtrl();
        CString hash;

        for (int i = 0;i < listCtrl.GetHeaderCtrl()->GetItemCount(); i++)
        {
            HDITEM hdi; 
            wchar_t lpBuffer[256]; 

            hdi.mask = HDI_TEXT; 
            hdi.pszText = lpBuffer; 
            hdi.cchTextMax = 256; 
            listCtrl.GetHeaderCtrl()->GetItem(i, &hdi);
            CString colName = lpBuffer;

            if (colName == L"旧哈希值")
                hash = listCtrl.GetItemText(selItem, i);
           
        } 

        CWnd* father = GetParent()->GetParent()->GetParent();
        ::AfxGetMainWnd()->SendMessage(RIGHTLISTITEM, (WPARAM)hash.GetBuffer(hash.GetLength()), 0);
        hash.ReleaseBuffer();
    }
    *pResult = 0;
}

void CMyRightListView::sort(int column, bool isAscending)
{
    m_sortColumn = column;
    m_isAscending = isAscending;

    CListCtrl& listCtrl = GetListCtrl();
    listCtrl.SortItems(compareFunction, reinterpret_cast<DWORD>(this));
}

int CALLBACK CMyRightListView::compareFunction(LPARAM lParam1, LPARAM lParam2, 
    LPARAM lParamData)
{
    CMyRightListView* listView = reinterpret_cast<CMyRightListView*>(lParamData);
    CListCtrl& listCtrl = listView->GetListCtrl();
    int item1;
    int item2;  
    LVFINDINFO findInfo;
    findInfo.flags = LVFI_PARAM;    // 指定查找方式
    findInfo.lParam = lParam1;
    item1 = listCtrl.FindItem(&findInfo, -1); // 得到对应Item索引
    findInfo.lParam = lParam2;
    item2 = listCtrl.FindItem(&findInfo, -1);

    CString strItem1 = listCtrl.GetItemText(item1, listView->m_sortColumn);
    CString strItem2 = listCtrl.GetItemText(item2, listView->m_sortColumn);

    if(IsNumber(strItem1) && IsNumber(strItem2))
        return listView->m_isAscending ? 
        NumberCompare(strItem1, strItem2) : NumberCompare(strItem2, strItem1);
    else
        return listView->m_isAscending ? 
        strItem1 < strItem2 : strItem1 > strItem2;

    return 0;
}

void CMyRightListView::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int column = pNMLV->iSubItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    sort(column, column == m_sortColumn ? !m_isAscending : TRUE );

    *pResult = 0;
}

void CMyRightListView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

    // TODO: 在此添加控件通知处理程序代码
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
    CListCtrl& list = GetListCtrl();
    str += list.GetItemText(pNMItemActivate->iItem, 
                            FieldColumnMapping::kSongFullListFilePath);
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



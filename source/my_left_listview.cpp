#include "my_left_listview.h"

#include <string>

#include "list_item_define.h"
#include "common.h"
using std::wstring;

IMPLEMENT_DYNCREATE(CMyLeftListView, CListView)
    
CMyLeftListView::CMyLeftListView()
    : m_isAscending(true)
    , m_sortColumn(-1)
    , m_pathCol(-1)
{

}

CMyLeftListView::~CMyLeftListView()
{

}

BEGIN_MESSAGE_MAP(CMyLeftListView, CListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMyLeftListView::OnLvnItemchanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CMyLeftListView::OnColumnClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CMyLeftListView::OnNMDblclk)
END_MESSAGE_MAP()

void CMyLeftListView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // TODO: 在此添加控件通知处理程序代码
    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED))
    {
        int selItem = pNMLV->iItem;
        TListItem item;
        CListCtrl& listCtrl = GetListCtrl();
        CString fullName;

        for (int i = 0;i < listCtrl.GetHeaderCtrl()->GetItemCount(); i++)
        {
            HDITEM hdi; 
            wchar_t lpBuffer[256]; 

            hdi.mask = HDI_TEXT; 
            hdi.pszText = lpBuffer; 
            hdi.cchTextMax = 256; 
            listCtrl.GetHeaderCtrl()->GetItem(i, &hdi);
            CString colName = lpBuffer;

            if (colName == L"歌曲编号")
                item.id = listCtrl.GetItemText(selItem, i);
            else if (colName == L"旧哈希值")
                item.OldHash = listCtrl.GetItemText(selItem, i);
            else if (colName == L"编辑重命名")
            {
                    item.Name = listCtrl.GetItemText(selItem, i);   
                    fullName = item.Name;
            }
            else if (colName == L"是否有交错")
                item.IsInterlace = listCtrl.GetItemText(selItem, i);  
            else if (colName == L"备注")
                item.Notes = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"原唱音轨")
                item.TrackType = listCtrl.GetItemText(selItem, i); 
            else if (colName == L"歌曲类型")
                item.MusicType = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"新哈希值")
                item.NewHash = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"画质级别")
                item.Quality = listCtrl.GetItemText(selItem, i);    
        } 

        ::AfxGetMainWnd()->SendMessage(UPDATESELITEM, (WPARAM)&item, 0);

        CString songName;
        int index = fullName.ReverseFind('-');
        songName = fullName.Right(fullName.GetLength() - index - 1);
        songName.TrimLeft();
        songName.TrimRight();
        CWnd* father = GetParent()->GetParent()->GetParent();
        TLeftListItem leftItem;
        leftItem.Name = songName;
        leftItem.OldHash = item.OldHash;
        father->SendMessage(LEFTLISTITEM, (WPARAM)&leftItem, 0);
    }
    *pResult = 0;
}

void CMyLeftListView::sort(int column, bool isAscending)
{
    m_sortColumn = column;
    m_isAscending = isAscending;

    CListCtrl& listCtrl = GetListCtrl();
    listCtrl.SortItems(compareFunction, reinterpret_cast<DWORD>(this));
}

int CALLBACK CMyLeftListView::compareFunction(LPARAM lParam1, LPARAM lParam2, 
                                              LPARAM lParamData)
{
    CMyLeftListView* listView = reinterpret_cast<CMyLeftListView*>(lParamData);
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

void CMyLeftListView::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int column = pNMLV->iSubItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    sort(column, column == m_sortColumn ? !m_isAscending : TRUE );

    *pResult = 0;
}

void CMyLeftListView::SetPathCol(int col)
{
    m_pathCol = col;
}

void CMyLeftListView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
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
    str += list.GetItemText(pNMItemActivate->iItem, m_pathCol);
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

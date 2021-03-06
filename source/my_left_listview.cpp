#include "my_left_listview.h"

#include <string>

#include "list_item_define.h"
#include "common.h"
#include "field_column_mapping.h"
#include "ini_control.h"

using std::wstring;

extern bool IsNumber(CString str);
extern int NumberCompare(CString str1, CString str2);

IMPLEMENT_DYNCREATE(CMyLeftListView, CListView)
    
CMyLeftListView::CMyLeftListView()
    : m_isAscending(true)
    , m_sortColumn(-1)
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
                item.MVType = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"新哈希值")
                item.NewHash = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"画质级别")
                item.Quality = listCtrl.GetItemText(selItem, i);    
        } 

        ::AfxGetMainWnd()->SendMessage(UPDATESELITEM, (WPARAM)&item, selItem);

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

void CMyLeftListView::OnNMDblclk(NMHDR* desc, LRESULT* r)
{
    NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(desc);

    CString str;
    str += L"\"";
    CListCtrl& list = GetListCtrl();
    str += list.GetItemText(itemActivate->iItem, 
                            FieldColumnMapping::kSongFullListFilePath);
    str += L"\"";

    wstring exePathName = CIniControl::get()->GetPlayerPathName();

    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(shExecInfo);
    shExecInfo.fMask = NULL;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = NULL;
    shExecInfo.lpFile = exePathName.c_str();
    shExecInfo.lpParameters = str;
    shExecInfo.lpDirectory = NULL;
    shExecInfo.hInstApp = NULL;

    ShellExecuteEx(&shExecInfo);
    *r = 0;
}
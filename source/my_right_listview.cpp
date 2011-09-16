#include "my_right_listview.h"

#include <string>

#include "list_item_define.h"
#include "common.h"
#include "field_column_mapping.h"
#include "ini_control.h"

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

    // TODO: �ڴ���ӿؼ�֪ͨ����������
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

            if (colName == L"�ɹ�ϣֵ")
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
    findInfo.flags = LVFI_PARAM;    // ָ�����ҷ�ʽ
    findInfo.lParam = lParam1;
    item1 = listCtrl.FindItem(&findInfo, -1); // �õ���ӦItem����
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

void CMyRightListView::OnNMDblclk(NMHDR* desc, LRESULT* r)
{
    NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(desc);

    CString str;
    str += L"\"";
    CListCtrl& list = GetListCtrl();
    str += list.GetItemText(itemActivate->iItem, 
                            FieldColumnMapping::kSongFullListFilePath);
    str += L"\"";

    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(shExecInfo);
    shExecInfo.fMask = NULL;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = NULL;
    shExecInfo.lpFile = CIniControl::get()->GetPlayerPathName().c_str();
    shExecInfo.lpParameters = str;
    shExecInfo.lpDirectory = NULL;
    shExecInfo.hInstApp = NULL;

    ShellExecuteEx(&shExecInfo);
    *r = 0;
}
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

    // TODO: �ڴ���ӿؼ�֪ͨ����������
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

            if (colName == L"�������")
                item.id = listCtrl.GetItemText(selItem, i);
            else if (colName == L"�ɹ�ϣֵ")
                item.OldHash = listCtrl.GetItemText(selItem, i);
            else if (colName == L"�༭������")
            {
                    item.Name = listCtrl.GetItemText(selItem, i);   
                    fullName = item.Name;
            }
            else if (colName == L"�Ƿ��н���")
                item.IsInterlace = listCtrl.GetItemText(selItem, i);  
            else if (colName == L"��ע")
                item.Notes = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"ԭ������")
                item.TrackType = listCtrl.GetItemText(selItem, i); 
            else if (colName == L"��������")
                item.MusicType = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"�¹�ϣֵ")
                item.NewHash = listCtrl.GetItemText(selItem, i);   
            else if (colName == L"���ʼ���")
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
    // TODO: �ڴ���ӿؼ�֪ͨ����������
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
    //ShExecInfo.nShow          = SW_MAXIMIZE;                // ȫ����ʾ�������
    ShExecInfo.hInstApp = NULL;

    BOOL r = ShellExecuteEx(&ShExecInfo);

    *pResult = 0;
}

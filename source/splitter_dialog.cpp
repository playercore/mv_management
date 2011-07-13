// splitter_dialog.cpp : 实现文件
//

#include "splitter_dialog.h"
#include "afxdialogex.h"
#include "my_left_listview.h"
#include "list_item_define.h"
#include "my_right_listview.h"
#include "common.h"
#include "sql_control.h"

// CSplitterDialog 对话框

IMPLEMENT_DYNAMIC(CSplitterDialog, CDialog)

CSplitterDialog::CSplitterDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSplitterDialog::IDD, pParent)
{

}

CSplitterDialog::~CSplitterDialog()
{
}

void CSplitterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSplitterDialog, CDialog)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_MESSAGE(LEFTLISTITEM, &CSplitterDialog::updateLeftListSel)
END_MESSAGE_MAP()


// CSplitterDialog 消息处理程序


BOOL CSplitterDialog::OnInitDialog()
{
    CDialog::OnInitDialog();


    CListView* leftListView = (CListView*)m_wndSplitter.GetPane(0, 0);
    CListCtrl& leftListCtrl = leftListView->GetListCtrl();

    leftListCtrl.ModifyStyle(0, LVS_REPORT|WS_VSCROLL|WS_HSCROLL|LVS_SHOWSELALWAYS|LVS_SINGLESEL);
    leftListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT); 

    CListView* rightListView = (CListView*)m_wndSplitter.GetPane(0, 1);
    CListCtrl& rightListCtrl = rightListView->GetListCtrl();
    rightListCtrl.ModifyStyle(0, LVS_REPORT|WS_VSCROLL|WS_HSCROLL|LVS_SHOWSELALWAYS|LVS_SINGLESEL);
    rightListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT); 

    _RecordsetPtr recordset = CSQLControl::get()->BaseSelect();
    Fields* fields;
    recordset->get_Fields(&fields);
    int num = fields->GetCount() - 8; //不显示最后8列

    for (long i = 0;i < num; ++i)
    {
        _variant_t index(i);    
        Field* field = NULL;
        FieldPtr p = fields->GetItem(index);
        _bstr_t name;
        name = p->GetName();
        CString strName =  (LPCTSTR)name;

        leftListCtrl.InsertColumn(i, strName, LVCFMT_LEFT , 80, i);
        rightListCtrl.InsertColumn(i, strName, LVCFMT_LEFT , 80, i);
    }

    // TODO:  在此添加额外的初始化
    return TRUE;  // return TRUE unless you set the focus to a control
}

int CSplitterDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1) 
        return -1; 
    // Create the frame window with "this" as the parent 
    m_pMyFrame = new CFrameWnd;
    m_pMyFrame->Create(NULL,L"", WS_CHILD, 
        CRect(0,0,1,1), this); 
    m_pMyFrame->ShowWindow(SW_SHOW); 
    CRect rect;
    GetParent()->GetClientRect(rect);
    m_pMyFrame->MoveWindow(rect); 

    // and finally, create the splitter with the frame as 
    // the parent 
    m_wndSplitter.CreateStatic(m_pMyFrame,1, 2); 
    m_wndSplitter.CreateView(0,0, RUNTIME_CLASS(CMyLeftListView), 
        CSize(rect.Width() / 2,rect.Height()), NULL); 
    m_wndSplitter.CreateView(0,1, RUNTIME_CLASS(CMyRightListView), 
        CSize(rect.Width() / 2,rect.Height()), NULL); 
    
    return 0;
}


void CSplitterDialog::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    CRect rect; 
    GetWindowRect(&rect); 

    ScreenToClient(&rect); 
    m_pMyFrame->MoveWindow(&rect); 
    m_pMyFrame->ShowWindow(SW_SHOW); 
    m_wndSplitter.MoveWindow(0,0, rect.Width(), rect.Height()); 
    m_wndSplitter.ShowWindow(SW_SHOW); 


    // TODO: 在此处添加消息处理程序代码
}

void CSplitterDialog::Search(_RecordsetPtr recordset)
{
    Fields* fields;
    recordset->get_Fields(&fields);
    int num = fields->GetCount() - 8; //不显示最后8列

    CListView* listView = (CListView*)m_wndSplitter.GetPane(0,0);
    CListCtrl& listCtrl = listView->GetListCtrl();
    listCtrl.DeleteAllItems();

    int row = 0;
    listCtrl.SetRedraw(FALSE); 
    DWORD begin = GetTickCount();
    while (!recordset->adoEOF)
    {
        _variant_t t = recordset->GetCollect((long)0);
        CString str = (LPWSTR)(_bstr_t)t;
        int index = listCtrl.InsertItem(row, str);
        listCtrl.SetItemData(index, row);

        for (long i = 1;i < num; ++i)
        {
            t =  recordset->GetCollect(i);

            if (t.vt != VT_NULL)
                str = (LPWSTR)(_bstr_t)t;
            else
                str = L"";
            listCtrl.SetItemText(row, i, str);
        }
        recordset->MoveNext();   
        row++;
    }

    listCtrl.SetRedraw(TRUE);
    listCtrl.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, 
        LVIS_SELECTED|LVIS_FOCUSED);
}

LRESULT CSplitterDialog::updateLeftListSel(WPARAM waram, LPARAM param)
{
    TLeftListItem* leftItem = (TLeftListItem*)waram;
    _RecordsetPtr recordset;
    recordset = CSQLControl::get()->SelectByLeftListView(
        (LPTSTR)(LPCTSTR)leftItem->Name, (LPTSTR)(LPCTSTR)leftItem->OldHash);
    if (!recordset)
        return 0;

    Fields* fields;
    recordset->get_Fields(&fields);
    int num = fields->GetCount() - 8; //不显示最后8列

    CListView* listView = (CListView*)m_wndSplitter.GetPane(0,1);
    CListCtrl& listCtrl = listView->GetListCtrl();
    listCtrl.DeleteAllItems();

    int row = 0;
    listCtrl.SetRedraw(FALSE); 
    DWORD begin = GetTickCount();
    while (!recordset->adoEOF)
    {
        _variant_t t = recordset->GetCollect((long)0);
        CString str = (LPWSTR)(_bstr_t)t;
        int index = listCtrl.InsertItem(row, str);
        listCtrl.SetItemData(index, row);
        for (long i = 1;i < num; ++i)
        {
            t =  recordset->GetCollect(i);

            if (t.vt != VT_NULL)
                str = (LPWSTR)(_bstr_t)t;
            else
                str = L"";
            listCtrl.SetItemText(row, i, str);
        }
        recordset->MoveNext();   
        row++;
    }

    listCtrl.SetRedraw(TRUE);
    listCtrl.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, 
        LVIS_SELECTED|LVIS_FOCUSED);
    return 0;
}

void CSplitterDialog::SelectItem()
{
    CListView* listView = (CListView*)m_wndSplitter.GetPane(0,1);
    CListCtrl& listCtrl = listView->GetListCtrl();
    int index = listCtrl.GetSelectionMark();
    listCtrl.SetItemState(index, LVIS_SELECTED|LVIS_FOCUSED, 
        LVIS_SELECTED|LVIS_FOCUSED);
}


#include "mv_management_dialog.h"

#include <sstream>
#include <cmath>
#include <string>
#include <memory>
#include <vld.h>

#include <boost/filesystem.hpp>
#include "afxdialogex.h"
#include "afxdb.h"

#include "field_column_mapping.h"
#include "mv_management_app.h"
#include "search_dialog.h"
#include "sql_control.h"
#include "common.h"
#include "util.h"

using std::wstring;
using std::unique_ptr;
using std::wstringstream;
using boost::filesystem3::path;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {
struct EnumChildrenParam
{
    HWND Excluded1;
    HWND Excluded2;
    HWND Excluded3;
    HWND Excluded4;
    HWND SpecifiedParent;
    bool Hide;
};

BOOL __stdcall EnumAndShow(HWND h, LPARAM p)
{
    EnumChildrenParam* param = reinterpret_cast<EnumChildrenParam*>(p);
    if ((h == param->Excluded1) || (h == param->Excluded2) ||
        (h == param->Excluded3) || (h == param->Excluded4))
        return TRUE;

    if (GetParent(h) != param->SpecifiedParent)
        return TRUE;

    ShowWindow(h, param->Hide ? SW_HIDE : SW_SHOW);
    return TRUE;
}
}

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* dataExch);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* dataExch)
{
	CDialogEx::DoDataExchange(dataExch);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CMVManagementDialog::CMVManagementDialog(CWnd* parent /*=NULL*/)
	: CDialogEx(CMVManagementDialog::IDD, parent)
    , m_id_from(L"")
    , m_id_to(L"")
    , m_page(0)
    , m_curListSelItem(-1)
    , collapse_()
    , tabTop_(-1)
    , guideText1_()
    , guideText2_()
{
	m_icon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMVManagementDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_TAB1, m_tab);
    DDX_Text(dataExch, IDC_EDIT_ID_FROM, m_id_from);
    DDX_Text(dataExch, IDC_EDIT_ID_TO, m_id_to);
    DDX_Control(dataExch, IDC_STATIC_GUIDE, guide_);
    DDX_Control(dataExch, IDC_BUTTON_LIST_ONLY, collapse_);
    DDX_Control(dataExch, IDC_STATIC_GUIDE_TEXT_1, guideText1_);
    DDX_Control(dataExch, IDC_STATIC_GUIDE_TEXT_2, guideText2_);
}

BEGIN_MESSAGE_MAP(CMVManagementDialog, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CMVManagementDialog::OnTcnSelchange)
    ON_BN_CLICKED(IDC_BUTTON_SEARCH,
                  &CMVManagementDialog::OnBnClickedButtonSearch)
    ON_MESSAGE(UPDATESELITEM, &CMVManagementDialog::updateListSel)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_BUTTON_SUBMIT,
                  &CMVManagementDialog::OnBnClickedButtonSubmit)
    ON_CBN_SELCHANGE(IDC_COMBO_FILTER_CONDITION,
                     &CMVManagementDialog::OnCbnSelchangeComboFilterCondition)
    ON_MESSAGE(RIGHTLISTITEM, &CMVManagementDialog::updateRightListSel)
    ON_BN_CLICKED(IDC_BUTTON_ONLY_LIST,
                  &CMVManagementDialog::OnBnClickedButtonOnlyList)
    ON_WM_SIZE()
    ON_MESSAGE(SongInfoListControl::GetDisplaySwitchMessage(),
               &CMVManagementDialog::OnSongFullListDisplaySwitch)
    ON_MESSAGE(SongInfoListControl::GetPictureUploadDone(),
               &CMVManagementDialog::OnPictureUnLoaded)

END_MESSAGE_MAP()


// Cmv_managementDlg 消息处理程序

BOOL CMVManagementDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_icon, TRUE);			// 设置大图标
	SetIcon(m_icon, FALSE);		// 设置小图标

    CreateStatusBar();

	CButton* trackButton = reinterpret_cast<CButton*>(GetDlgItem(IDC_RADIO_0));
	trackButton->SetCheck(1);

	CButton* typeButton = reinterpret_cast<CButton*>(GetDlgItem(IDC_RADIO_6));
	typeButton->SetCheck(1);

	CButton* interlaceButton = 
		reinterpret_cast<CButton*>(GetDlgItem(IDC_RADIO_NO));
	interlaceButton->SetCheck(1);

	CComboBox* qualityCombox = 
		reinterpret_cast<CComboBox*>(GetDlgItem(IDC_COMBO_MV_QUALITY));

	for (int i = 1; i <= 9; ++i)
	{
		CString str;
		str.Format(L"%d",i);
		qualityCombox->AddString(str);
	}
	qualityCombox->SetCurSel(0);

	CComboBox* filterCombox = 
		reinterpret_cast<CComboBox*>(GetDlgItem(IDC_COMBO_FILTER_CONDITION));
	filterCombox->AddString(L"当前审核歌曲");
	filterCombox->AddString(L"第一批歌曲");
	filterCombox->AddString(L"第二批歌曲");
	filterCombox->AddString(L"全部歌曲");
    filterCombox->AddString(L"零散MV");
	filterCombox->SetCurSel(3);

    m_filterType = filterCombox->GetCurSel();

	wstring path;
	wchar_t curPath[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH,curPath);
	path = curPath;
	path += L"\\config.ini";

    wchar_t buf[32767];
    int len = GetPrivateProfileString(L"ID", L"START", L"0", buf, 32767, 
                                      path.c_str());
    m_id_from = buf;

    len = GetPrivateProfileString(L"ID", L"END", L"0", buf, 32767, 
                                  path.c_str());
    m_id_to = buf;
    UpdateData(FALSE);

	buf[32767];
	len = GetPrivateProfileString(L"type", NULL, L"", buf, 32767, 
									  path.c_str());

	CComboBox* typeCombox = 
		reinterpret_cast<CComboBox*>(GetDlgItem(IDC_COMBO_MV_TYPE));

	typeCombox->AddString(L"多首歌曲组合");

	CString sectionData;
	for(int i = 0; i < len; i++)
	{
		if(buf[i] != '\0') 
		{
			sectionData = sectionData + buf[i];
		} 
		else
		{
			if(sectionData != L"") 
				typeCombox->AddString(sectionData);
			
			sectionData = L"";
		}
	}
    
    guideText1_.SetParent(&guide_);
    guideText2_.SetParent(&guide_);
    guide_.Init();
    guide_.ShowWindow(SW_HIDE);

    CRect rect;
    m_tab.GetWindowRect(&rect);
    ScreenToClient(&rect);
    tabTop_ = rect.top;

	m_tab.InsertItem(0,L"所有歌曲");
	m_tab.InsertItem(1,L"去除重复歌曲");

    m_songFullList.Create(&m_tab, rect, IDC_LIST1);
    m_splitterDialog.Create(IDD_DIALOG_SPLITTER, &m_tab);
    m_splitterDialog.MoveWindow(&rect);
  
    _RecordsetPtr recordset;
    simpleUpdate(recordset);
        
    if (recordset != NULL)
    {
        Fields* fields;
        recordset->get_Fields(&fields);
        int num = fields->GetCount() - 8; //不显示最后8列

        for (long i = 0; i < num; ++i)
        {
            _variant_t index(i);    
            Field* field = NULL;
            FieldPtr p = fields->GetItem(index);
            _bstr_t name;
            name = p->GetName();
            CString strName =  (LPCTSTR)name;
            m_songFullList.InsertColumn(i, strName, LVCFMT_LEFT , 80, i);
        }

        updateSongFullListByRecordset(recordset);
    }

    GetClientRect(&rect);
    Layout(rect.Width(), rect.Height());
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMVManagementDialog::OnSysCommand(UINT id, LPARAM param)
{
	if ((id & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(id, param);
	}
}

void CMVManagementDialog::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_icon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMVManagementDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_icon);
}

void CMVManagementDialog::OnBnClickedOtherMVType()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_COMBO_MV_TYPE)->EnableWindow(TRUE);
}

void CMVManagementDialog::OnBnClickedDefinedMVType(UINT id)
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_COMBO_MV_TYPE)->EnableWindow(FALSE);
}

void CMVManagementDialog::OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult)
{
	//当点击了Tab的按钮
	
	int num = m_tab.GetCurSel();
	m_page = num;
    _RecordsetPtr recordSet;
    simpleUpdate(recordSet);
	switch(num)	
	{
 		case 0:
		{
			m_songFullList.ShowWindow(SW_SHOW);
			m_splitterDialog.ShowWindow(SW_HIDE);
            m_songFullList.DeleteAllItems();
            updateSongFullListByRecordset(recordSet);
            m_songFullList.SelectItem(0);
            if (!m_songFullList.IsReportView())
                guide_.ShowWindow(SW_SHOW);

			break;
		}
		case 1:
		{
			m_songFullList.ShowWindow(SW_HIDE);		
			m_splitterDialog.ShowWindow(SW_SHOW);	
            updateSplitterWnd(recordSet);
            m_splitterDialog.SelectItem();
            guide_.ShowWindow(SW_HIDE);
			break;
		}
	}
	*pResult = 0;
}

void CMVManagementDialog::updateSongFullListByRecordset(_RecordsetPtr recordset)
{
    Fields* fields;
    recordset->get_Fields(&fields);
    int num = fields->GetCount() - 8; //不显示最后8列

    int row = 0;
    m_songFullList.DeleteAllItems();
    m_songFullList.SetRedraw(FALSE);
    DWORD begin = GetTickCount();
    while (!recordset->adoEOF)
    {
        _variant_t t = recordset->GetCollect(0L);
        CString songIdInText = (LPWSTR)(_bstr_t)t;
        const int songId = static_cast<int>(t);
        t = recordset->GetCollect(2L);
        wstring md5 = static_cast<wchar_t*>(static_cast<_bstr_t>(t));
        t = recordset->GetCollect(16L);
        int index = m_songFullList.AddItem(
            songIdInText, songId,
            ((VT_NULL != t.vt) ? static_cast<int>(t) : -1), md5);
        for (long i = 1;i < num; ++i)
        {
            t = recordset->GetCollect(i);

            if (t.vt != VT_NULL)
                songIdInText = (LPWSTR)(_bstr_t)t;
            else
                songIdInText = L"";

            m_songFullList.SetItemText(row, i, songIdInText);
        }

        recordset->MoveNext();
        row++;
    }

    m_songFullList.SetRedraw(TRUE);
}

void CMVManagementDialog::OnClose()
{
    CDialogEx::OnClose();
}


void CMVManagementDialog::OnBnClickedButtonSearch()
{
    // TODO: 在此添加控件通知处理程序代码
    _RecordsetPtr recordSet;
    simpleUpdate(recordSet);

    if (m_page == 1)
    {
        updateSplitterWnd(recordSet);
        return;
    }
    
    m_songFullList.DeleteAllItems();

    updateSongFullListByRecordset(recordSet);
}

LRESULT CMVManagementDialog::updateListSel(WPARAM waram, LPARAM param)
{
    TListItem* item = (TListItem*)waram;
    m_curListSelItem = (int)param;
    //m_curSelectedSongId = item->id;
    GetDlgItem(IDC_EDIT_SONG_NAME)->SetWindowText(item->Name);
    GetDlgItem(IDC_EDIT_NOTES)->SetWindowTextW(item->Notes);
    GetDlgItem(IDC_EDIT_NEW_HASH)->SetWindowText(item->NewHash);
    GetDlgItem(IDC_EDIT_OLD_HASH)->SetWindowTextW(item->OldHash);
    int trackType = _wtoi((LPTSTR)(LPCTSTR)item->TrackType);

    CheckRadioButton(IDC_RADIO_0, IDC_RADIO_5, IDC_RADIO_0 + trackType);
    
    if (item->IsInterlace == L"有")
    {
        //CheckRadioButton(IDC_RADIO_YES, IDC_RADIO_NO, IDC_RADIO_0 + trackType);
        ((CButton*)GetDlgItem(IDC_RADIO_YES))->SetCheck(TRUE);
        ((CButton*)GetDlgItem(IDC_RADIO_NO))->SetCheck(FALSE);
    }
    else
    {
        ((CButton*)GetDlgItem(IDC_RADIO_YES))->SetCheck(FALSE);
        ((CButton*)GetDlgItem(IDC_RADIO_NO))->SetCheck(TRUE);
    }
   
    int quality = _wtoi((LPTSTR)(LPCTSTR)item->Quality);
    ((CComboBox*)GetDlgItem(IDC_COMBO_MV_QUALITY))->SetCurSel(quality - 1);

    CString musicType = (LPTSTR)(LPCTSTR)item->MVType;
    if (musicType == L"MTV")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_6);
    else if (musicType == L"演唱会")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_7);
    else if (musicType == L"戏曲")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_8);
    else if (musicType == L"风景")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_9);
    else if (musicType == L"串烧")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_10);
    else
    {
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_11);
        GetDlgItem(IDC_COMBO_MV_TYPE)->SetWindowText(
            (LPTSTR)(LPCTSTR)item->MVType);  
    }
    return 0;
}


void CMVManagementDialog::OnBnClickedButtonSubmit()
{
    // TODO: 在此添加控件通知处理程序代码
    int r = GetCheckedRadioButton(IDC_RADIO_6, IDC_RADIO_11);
    CString mvType;

    switch (r)
    {
        case IDC_RADIO_6:
        {
            mvType = L"MTV";
            break;
        }
        case IDC_RADIO_7:
        {
            mvType = L"演唱会";
            break;
        }
        case IDC_RADIO_8:
        {
            mvType = L"戏曲";
            break;
        }
        case IDC_RADIO_9:
        {
            mvType = L"风景";
            break;
        }
        case IDC_RADIO_10:
        {
            mvType = L"串烧";
            break;
        }
        case IDC_RADIO_11:
        {
            GetDlgItem(IDC_COMBO_MV_TYPE)->GetWindowText(mvType);
            break;
        }
    }
    if (mvType == L"")
    {
        MessageBox(L"请输入歌曲类型");
        return;
    }

    r = GetCheckedRadioButton(IDC_RADIO_0, IDC_RADIO_5);
    CString trackType;
    trackType.Format(L"%d",r - IDC_RADIO_0);

    CString name;
    GetDlgItem(IDC_EDIT_SONG_NAME)->GetWindowText(name);

    CString notes;
    GetDlgItem(IDC_EDIT_NOTES)->GetWindowText(notes);

    r = GetCheckedRadioButton(IDC_RADIO_YES, IDC_RADIO_NO);
    CString isInterlace;
    if (r == IDC_RADIO_YES)
        isInterlace = L"有";
    else if (r == IDC_RADIO_NO)
        isInterlace = L"没有";

    CString quality;
    int index = ((CComboBox*)GetDlgItem(IDC_COMBO_MV_QUALITY))->GetCurSel();
    quality.Format(L"%d", index + 1);

    CString newHash;
    r = ((CButton*)GetDlgItem(IDC_CHECK_REPLACE))->GetCheck();
    if (r == 1)
        GetDlgItem(IDC_EDIT_NEW_HASH)->GetWindowText(newHash);

    CString curTime;
    getCurTime(curTime);

    CString query;
    query = L"UPDATE CHECKED_ENCODE_FILE_INFO SET 原唱音轨 = ";
    query += trackType;
    query += L",编辑重命名 = '";
    query += name;
    query += L"',备注 = '";
    query += notes;
    query += L"',是否有交错 = '";
    query += isInterlace;
    query += L"',歌曲类型 = '";
    query += mvType;
    query += L"',画质级别 = ";
    query += quality;

    bool isReplace = !!((CButton*)GetDlgItem(IDC_CHECK_REPLACE))->GetCheck();
 
    if (isReplace)
    {
        query += L",新哈希值 = '";
        query += newHash;
        query += L"'";
    }
    query += L",客户端ip地址 = '";
    query += GetLocalIP().c_str();
    query += L"',提交时间 = '";
    query += curTime;
    query += L"',歌曲状态 = '已审阅' WHERE 歌曲编号 = ";
    wstring songID = m_songFullList.GetItemText(m_curListSelItem, 
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListSongId));
    query += songID.c_str();
    
    if (!CSQLControl::get()->UpdateByString((LPTSTR)(LPCTSTR)query))
    { 
        MessageBox(L"插入失败", L"警告", MB_OK);
        return;
    }

    TListItem item;
    item.id = songID.c_str();
    item.IsInterlace = isInterlace;
    item.MVType = mvType;
    item.Name = name;
    item.NewHash = newHash;
    item.Notes = notes;
    item.Quality = quality;
    item.TrackType = trackType;
    if (m_page == 0)
    {
        updateSongFullListByString(&item, isReplace);                  
    }

    updateStatusForSong();
   
    _RecordsetPtr recordSet;
    simpleUpdate(recordSet);
    updateSplitterWnd(recordSet);
}


void CMVManagementDialog::OnCbnSelchangeComboFilterCondition()
{
    // TODO: 在此添加控件通知处理程序代码
    int index = ((CComboBox*)GetDlgItem(IDC_COMBO_FILTER_CONDITION))->GetCurSel();
    m_filterType = index;
    SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_SEARCH,BN_CLICKED),0 );
    
    return;
}

void CMVManagementDialog::updateSplitterWnd(_RecordsetPtr recordset)
{
    m_splitterDialog.Search(recordset);
}

LRESULT CMVManagementDialog::updateRightListSel(WPARAM waram, LPARAM param)
{
    GetDlgItem(IDC_EDIT_NEW_HASH)->SetWindowText((wchar_t*)waram);
    return 0;
}

void CMVManagementDialog::getCurTime(CString& time)
{
    CTime now = CTime::GetCurrentTime();
    time = now.Format(L"%Y-%m-%d %H:%M:%S");
}

void CMVManagementDialog::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    if (!IsWindow(m_tab.GetSafeHwnd()) || !m_songFullList.HasBeenCreated() ||
        !IsWindow(m_splitterDialog.GetSafeHwnd()))
        return;

    Layout(cx, cy);
}


BOOL CMVManagementDialog::PreTranslateMessage(MSG* pMsg)
{
    // TODO: 在此添加专用代码和/或调用基类
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case 'F':
            {
                if(::GetKeyState(VK_CONTROL) < 0)
                {
                    CSearchDialog dlg;
                    if (dlg.DoModal() == IDOK)
                    {
                        CString query = dlg.GetQuery();

                        _RecordsetPtr recordSet = 
                            CSQLControl::get()->SelectByString(
                                (LPTSTR)(LPCTSTR)query);

                        if (!recordSet)
                            return FALSE;

                        if (m_page == 0)
                            updateSongFullListByRecordset(recordSet);
                        else if (m_page == 1)
                            updateSplitterWnd(recordSet);                      
                    }
                    return TRUE;
                }           
            }
        }
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}

void CMVManagementDialog::simpleUpdate(_RecordsetPtr& recordset)
{
    UpdateData(TRUE);
    CString query = GetBaseQuery();
    int idFrom = _wtoi((LPTSTR)(LPCTSTR)m_id_from);
    int idTo = _wtoi((LPTSTR)(LPCTSTR)m_id_to);
    m_filterType = 
        ((CComboBox*)GetDlgItem(IDC_COMBO_FILTER_CONDITION))->GetCurSel();

    recordset = CSQLControl::get()->BaseSelect(idFrom, idTo, m_filterType);
}

void CMVManagementDialog::updateSongFullListByString(TListItem* item, 
                                                     bool isReplace)
{
    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMVType), 
        item->MVType);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListTrackType), 
        item->TrackType);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListEditorRename), 
        item->Name);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListNotes), 
        item->Notes);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListInterlace), 
        item->IsInterlace);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListQuality), 
        item->Quality);

    m_songFullList.SetItemText(
        m_curListSelItem,
        FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListMVStatus), 
        L"已审阅");

    if (isReplace)
    {
        m_songFullList.SetItemText(
            m_curListSelItem,
            FieldColumnMapping::get()->GetColumnIndex(
            FieldColumnMapping::kSongFullListNewHash), 
            item->NewHash);
    }
}

void CMVManagementDialog::OnBnClickedButtonOnlyList()
{
    CWnd* listOnly = GetDlgItem(IDC_BUTTON_LIST_ONLY);
    CWnd* guide = GetDlgItem(IDC_STATIC_GUIDE);
    CWnd* statusBar = GetDlgItem(IDS_STATUS);
    if (!listOnly || !guide || !statusBar)
        return;

    EnumChildrenParam p = {
        listOnly->GetSafeHwnd(), m_tab.GetSafeHwnd(), guide->GetSafeHwnd(),
        statusBar->GetSafeHwnd(), GetSafeHwnd(), collapse_.IsCollapsed() };
    EnumChildWindows(GetSafeHwnd(), EnumAndShow, reinterpret_cast<LPARAM>(&p));

    CRect myRect;
    GetClientRect(&myRect);
    Layout(myRect.Width(), myRect.Height());
}

LRESULT CMVManagementDialog::OnSongFullListDisplaySwitch(WPARAM w, LPARAM l)
{
    guide_.ShowWindow(w ? SW_SHOW : SW_HIDE);

    if (w)
        updateStatusForPicture();
    else
        updateStatusForSong();

    return 0;
}

void CMVManagementDialog::Layout(int cx, int cy)
{
    // 'list-only' button.
    CWnd* listOnly = GetDlgItem(IDC_BUTTON_LIST_ONLY);
    CRect buttonRect;
    listOnly->GetWindowRect(&buttonRect);
    ScreenToClient(&buttonRect);

    listOnly->SetWindowPos(NULL, buttonRect.left,
                           collapse_.IsCollapsed() ? 10 : 263, 0, 0,
                           SWP_NOSIZE);

    // Tab control.
    CRect tabRect;
    m_tab.GetWindowRect(&tabRect);
    ScreenToClient(&tabRect);

    const int tabTop = collapse_.IsCollapsed() ? 25 : tabTop_;
    const int tabHeight = cy - tabTop - 22;
    m_tab.SetWindowPos(NULL, tabRect.left, tabTop, cx - tabRect.left * 2,
                       tabHeight, SWP_NOZORDER);

    // 'guide' control.
    CRect itemRect;
    m_tab.GetItemRect(0, &itemRect);
    CRect guideRect;
    guide_.GetWindowRect(&guideRect);
    const int guideWidth = 400;
    guide_.MoveWindow(cx - guideWidth - tabRect.left - 2, tabTop - 1,
                      guideWidth, itemRect.Height());

    CRect text1Rect;
    guideText1_.GetWindowRect(&text1Rect);
    const int guideTextTop = (itemRect.Height() - text1Rect.Height()) / 2;
    guideText1_.MoveWindow(20, guideTextTop, text1Rect.Width(),
                           text1Rect.Height());

    CRect text2Rect;
    guideText2_.GetWindowRect(&text2Rect);
    guideText2_.MoveWindow(20 + text1Rect.Width() + 30 + 20, guideTextTop,
                           text2Rect.Width(), text2Rect.Height());

    // Lists.
    m_tab.GetClientRect(&tabRect);
    CRect fullListRect(1, itemRect.Height() + 3, tabRect.Width() - 3,
                       tabRect.Height() - 3);
    m_songFullList.MoveWindow(&fullListRect);
    m_splitterDialog.MoveWindow(&fullListRect);

    // Status bar.
    CWnd* statusBar = GetDlgItem(IDS_STATUS);
    if (statusBar) {
        CRect statusBarRect;
        statusBar->GetWindowRect(&statusBarRect);
        statusBar->MoveWindow(CRect(0, cy - statusBarRect.Height(), cx, cy));
    }
}

void CMVManagementDialog::CreateStatusBar()
{
    CRect rect;
    GetClientRect(&rect);
    HWND statusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        L"", // 显示在状态栏上的信息
                                        GetSafeHwnd(), // 父窗口句柄
                                        IDS_STATUS); // 预定义的资源ID 

    int parts[] = { rect.Width() / 6, rect.Width() / 3, rect.Width() / 2,
        rect.Width() * 2 / 3, rect.Width() * 5 / 6, -1 };
    ::SendMessage(statusBar, SB_SETPARTS, arraysize(parts),
                  reinterpret_cast<LPARAM>(parts));
    ::SendMessage(statusBar, SB_SETTEXT, 1,
                  reinterpret_cast<LPARAM>(L"当前已审核的歌曲：0"));
    ::SendMessage(statusBar, SB_SETTEXT, 2,
                  reinterpret_cast<LPARAM>(L"当天审核的歌曲：0"));
    ::SendMessage(statusBar, SB_SETTEXT, 3,
                  reinterpret_cast<LPARAM>(L"还没有审核的歌曲：0"));
    ::SendMessage(statusBar, SB_SETTEXT, 4,
                  reinterpret_cast<LPARAM>(L"一共有歌曲数：0"));
}

LRESULT CMVManagementDialog::OnPictureUnLoaded(WPARAM w, LPARAM l)
{
    updateStatusForPicture();
    return 0;
}

void CMVManagementDialog::updateStatusForSong()
{
    UpdateData(TRUE);
    int from = _wtoi((LPTSTR)(LPCTSTR)m_id_from);
    int to = _wtoi((LPTSTR)(LPCTSTR)m_id_to);
    int curReviewed = 0;
    int todayReviewed = 0;
    int needReview = 0;
    int totalSong = 0;
    CSQLControl::get()->StatusStoreProcForSong(from, to, m_filterType, 
        &curReviewed, &todayReviewed, &needReview, &totalSong);

    wstringstream strCurReviewed;
    strCurReviewed << L"当前已审核的歌曲：" << curReviewed;

    wstringstream strTodayReviewed;
    strTodayReviewed << L"当天审核的歌曲：" << todayReviewed;

    wstringstream strNeedReview;
    strNeedReview << L"还没有审核的歌曲：" << needReview;

    wstringstream strTotalSong;
    strTotalSong << L"一共有歌曲数：" << totalSong;

    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 1, (LPARAM)strCurReviewed.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 2, (LPARAM)strTodayReviewed.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 3, (LPARAM)strNeedReview.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 4, (LPARAM)strTotalSong.str().c_str());
}

void CMVManagementDialog::updateStatusForPicture()
{
    UpdateData(TRUE);
    int from = _wtoi((LPTSTR)(LPCTSTR)m_id_from);
    int to = _wtoi((LPTSTR)(LPCTSTR)m_id_to);
    int curReviewed = 0;
    int todayReviewed = 0;
    int needReview = 0;
    int totalSong = 0;
    CSQLControl::get()->StatusStoreProcForPic(from, to, &curReviewed,
        &todayReviewed, &needReview, &totalSong);

    wstringstream strCurReviewed;
    strCurReviewed << L"当前已审核的缩略图：" << curReviewed;

    wstringstream strTodayReviewed;
    strTodayReviewed << L"当天审核的缩略图：" << todayReviewed;

    wstringstream strNeedReview;
    strNeedReview << L"还没有审核的缩略图：" << needReview;

    wstringstream strTotalSong;
    strTotalSong << L"一共有缩略图数：" << totalSong;

    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 1, (LPARAM)strCurReviewed.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 2, (LPARAM)strTodayReviewed.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 3, (LPARAM)strNeedReview.str().c_str());
    GetDlgItem(IDS_STATUS)->SendMessage(
        SB_SETTEXT, 4, (LPARAM)strTotalSong.str().c_str());
}

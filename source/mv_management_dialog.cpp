#include "mv_management_dialog.h"

#include <cmath>
#include <string>
#include <memory>
#include <vld.h>

#include <boost/filesystem.hpp>
#include "afxdialogex.h"
#include "afxdb.h"
#include <winsock2.h>

#include "mv_management_app.h"
#include "search_dialog.h"
#include "common.h"
#include "jpeg_tool.h"
#include "util.h"

using std::wstring;
using std::unique_ptr;
using boost::filesystem3::path;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {
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
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

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


// Cmv_managementDlg 对话框




CMVManagementDialog::CMVManagementDialog(CWnd* parent /*=NULL*/)
	: CDialogEx(CMVManagementDialog::IDD, parent)
    , m_id_from(L"")
    , m_id_to(L"")
    , m_page(0)
{
	m_icon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMVManagementDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_TAB1, m_tab);
    DDX_Text(dataExch, IDC_EDIT_ID_FROM, m_id_from);
    DDX_Text(dataExch, IDC_EDIT_ID_TO, m_id_to);

}

BEGIN_MESSAGE_MAP(CMVManagementDialog, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()

	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CMVManagementDialog::OnTcnSelchange)
    ON_BN_CLICKED(IDC_BUTTON_SEARCH, &CMVManagementDialog::OnBnClickedButtonSearch)
    ON_MESSAGE(UPDATESELITEM, &CMVManagementDialog::updateListSel)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_BUTTON_SUBMIT, &CMVManagementDialog::OnBnClickedButtonSubmit)
    ON_CBN_SELCHANGE(IDC_COMBO_FILTER_CONDITION, &CMVManagementDialog::OnCbnSelchangeComboFilterCondition)
    ON_MESSAGE(RIGHTLISTITEM, &CMVManagementDialog::updateRightListSel)
    ON_WM_SIZE()
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
    	
	// TODO: 在此添加额外的初始化代码
    // 获取本机IP
    getLocalIP();

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

	CRect tabRect;
	m_tab.GetClientRect(&tabRect);  
	tabRect.top += 22;
	tabRect.bottom -= 0;
	tabRect.left += 0;  
	tabRect.right -= 2;  

	m_tab.InsertItem(0,L"所有歌曲");
	m_tab.InsertItem(1,L"去除重复歌曲");


    m_allSongList.Create(
        WS_VISIBLE|WS_BORDER|WS_CHILD|LVS_REPORT|WS_VSCROLL|WS_HSCROLL|LVS_SHOWSELALWAYS|LVS_SINGLESEL,
        tabRect, &m_tab, IDC_LIST1);

    m_allSongList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);      //生成网格线|选择整行

    m_allSongList.MoveWindow(&tabRect);

    m_splitterDialog.Create(IDD_DIALOG2, &m_tab);
    m_splitterDialog.MoveWindow(&tabRect);

    if (!AfxOleInit())
        MessageBox(L"OLE初始化失败");
    try
    {   
        m_connection.CreateInstance(__uuidof(Connection));

        wchar_t buf1[32767];
        int len1 = GetPrivateProfileString(L"databaseSetup", L"ServerIP", L"0", 
                                           buf1, 32767, path.c_str());
        CString server = buf1;
        len1 = GetPrivateProfileString(L"databaseSetup", L"UserName", L"0", 
                                           buf1, 32767, path.c_str());
        CString userName = buf1;

        len1 = GetPrivateProfileString(L"databaseSetup", L"Password", L"0", 
            buf1, 32767, path.c_str());
        CString password = buf1;

        len1 = GetPrivateProfileString(L"databaseSetup", L"DatabaseName", L"0", 
            buf1, 32767, path.c_str());
        CString databaseName = buf1;

        CString strSQL = L"Driver={SQL Server};Server=";
        strSQL += server;
        strSQL += L";Database=";
        strSQL += databaseName;
        strSQL += L";UID=";
        strSQL += userName;
        strSQL += L";PWD=";
        strSQL += password;
        
        m_connection->Open((_bstr_t)strSQL, "","",adConnectUnspecified);
    }
    catch (_com_error e)
    {
        MessageBox(e.Description());
    }

    try
    {  
        CString query = GetBaseQuery();
        query += L" and 歌曲编号 between " + m_id_from + " and " + m_id_to;
        if (m_filterType != 3)
        {   
            CString flag;
            flag.Format(L"%d", m_filterType);
            query += L" and 标识=";
            query += flag;
        }

        m_recordset = 
            m_connection->Execute((_bstr_t)query, NULL,adConnectUnspecified);
        Fields* fields;
        m_recordset->get_Fields(&fields);
        int num = fields->GetCount() - 8; //不显示最后8列

        for (long i = 0;i < num; ++i)
        {
            _variant_t index(i);    
            Field* field = NULL;
            FieldPtr p = fields->GetItem(index);
            _bstr_t name;
            name = p->GetName();
            CString strName =  (LPCTSTR)name;

            if (strName == L"总音轨数")
                m_allSongList.SetTrackCountCol(i);
            else if (strName == L"文件路径")
                m_allSongList.SetPathCol(i);                    
                                        
            m_allSongList.InsertColumn(i, strName, LVCFMT_LEFT , 80, i);
        }

        updateAllSongList(m_recordset);

        m_allSongList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, 
            LVIS_SELECTED|LVIS_FOCUSED);
    }
    catch (_com_error e)
    {
        MessageBox(e.Description());
    }
    
    m_splitterDialog.SetConnectionPtr(m_connection);
    UpdateData(FALSE);
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

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
	switch(num)	
	{
 		case 0:
		{
			m_allSongList.ShowWindow(SW_SHOW);
			m_splitterDialog.ShowWindow(SW_HIDE);
            int index = m_allSongList.GetSelectionMark();
            m_allSongList.SetItemState(index, LVIS_SELECTED|LVIS_FOCUSED, 
                LVIS_SELECTED|LVIS_FOCUSED);
			break;
		}
		case 1:
		{
			m_allSongList.ShowWindow(SW_HIDE);		
			m_splitterDialog.ShowWindow(SW_SHOW);	
            m_splitterDialog.SelectItem();
			break;
		}
	}
	*pResult = 0;
}

void CMVManagementDialog::updateAllSongList(_RecordsetPtr recordset)
{
    Fields* fields;
    recordset->get_Fields(&fields);
    int num = fields->GetCount() - 8; //不显示最后8列

    int row = 0;
    m_allSongList.DeleteAllItems();
    m_allSongList.SetRedraw(FALSE); 
    m_imageList.DeleteImageList();
    DWORD begin = GetTickCount();
    while (!recordset->adoEOF)
    {
        _variant_t t = recordset->GetCollect(0L);
        CString str = (LPWSTR)(_bstr_t)t;
        t = recordset->GetCollect(2L);
        wstring md5 = static_cast<wchar_t*>(static_cast<_bstr_t>(t));
        CBitmap bitmap;
        LoadJPEG(&bitmap, GetMvPreviewPath() + md5 + L".jpg");
        if (!m_imageList.m_hImageList)
        {
            if (!initImageList(&bitmap))
                return;
        }

        int imageIndex = m_imageList.Add(&bitmap, RGB(0,0,0));
        int index = m_allSongList.InsertItem(row, str, imageIndex);
        
        m_allSongList.SetItemData(index, row);
        for (long i = 1;i < num; ++i)
        {
            t =  recordset->GetCollect(i);

            if (t.vt != VT_NULL)
                str = (LPWSTR)(_bstr_t)t;
            else
                str = L"";
            m_allSongList.SetItemText(row, i, str);
        }

        recordset->MoveNext();   
        row++;
    }

    m_allSongList.SetRedraw(TRUE);
}

void CMVManagementDialog::OnClose()
{
    m_connection->Close(); 
    CDialogEx::OnClose();
}


void CMVManagementDialog::OnBnClickedButtonSearch()
{
    // TODO: 在此添加控件通知处理程序代码
    UpdateData(TRUE);
    simpleUpdate(m_recordset);

    if (m_page == 1)
    {
        updateSplitterWnd(m_recordset);
        return;
    }
    
    m_allSongList.DeleteAllItems();

    updateAllSongList(m_recordset);

    m_allSongList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, 
        LVIS_SELECTED|LVIS_FOCUSED);

}

LRESULT CMVManagementDialog::updateListSel(WPARAM waram, LPARAM param)
{
    TListItem* item = (TListItem*)waram;
    m_curSelectedId = item->id;
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

    CString musicType = (LPTSTR)(LPCTSTR)item->MusicType;
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
            (LPTSTR)(LPCTSTR)item->MusicType);  
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

    r = ((CButton*)GetDlgItem(IDC_CHECK_REPLACE))->GetCheck();
    
    if (r)
    {
        query += L",新哈希值 = '";
        query += newHash;
        query += L"'";
    }
    query += L",客户端ip地址 = '";
    query += m_ip;
    query += L"',提交时间 = '";
    query += curTime;
    query += L"',歌曲状态 = '已审阅' WHERE 歌曲编号 = ";
    query += m_curSelectedId;
    
    try
    {
        m_connection->Execute((_bstr_t)query, NULL, adConnectUnspecified);
    }
    catch (_com_error e)
    {
        MessageBox(e.Description());
    }
    
    UpdateData(TRUE);

    m_allSongList.DeleteAllItems();

    simpleUpdate(m_recordset);
    updateAllSongList(m_recordset);

    m_allSongList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, 
        LVIS_SELECTED|LVIS_FOCUSED);
    updateSplitterWnd(m_recordset);
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

void CMVManagementDialog::getLocalIP()
{
    WORD wVersionRequested;  

    wVersionRequested = MAKEWORD(1, 1);//版本号1.1

    WSADATA  wsaData;
    //1.加载套接字库 
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
        return;

    //判断是否我们请求的winsocket版本，如果不是
    //则调用WSACleanup终止winsocket的使用并返回            
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) 
    {
            WSACleanup();
            return; 
    }

    char name[255];  
    PHOSTENT hostinfo;  
    if(gethostname(name, sizeof(name)) == 0)  
    {  
        if((hostinfo = gethostbyname(name)) != NULL)  
        {  
            m_ip = inet_ntoa(*(struct in_addr*)*hostinfo->h_addr_list);  
        }  
    }  

    WSACleanup();  
}

void CMVManagementDialog::getCurTime(CString& time)
{
    CTime now = CTime::GetCurrentTime();
    time = now.Format(L"%Y-%m-%d %H:%M:%S");
}


void CMVManagementDialog::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    CRect clientRect;  
    GetClientRect(&clientRect);  

    // TODO: 在此处添加消息处理程序代码
     
    if (m_tab.m_hWnd!= NULL)
    {
        CRect tabRect;
        GetClientRect(&tabRect);  
        tabRect.top = clientRect.top + 275;  
        tabRect.bottom = clientRect.bottom;  
        tabRect.left = clientRect.left + 10;  
        tabRect.right = clientRect.right - 10;  

        m_tab.MoveWindow(tabRect);
    }

    if (m_allSongList.m_hWnd != NULL)
    {
        CRect tabRect;
        m_tab.GetClientRect(&tabRect);  
        tabRect.top += 22;
        tabRect.bottom -= 0;
        tabRect.left += 0;  
        tabRect.right -= 2;    
        m_allSongList.MoveWindow(tabRect);
    }

    if (m_splitterDialog.m_hWnd != NULL)
    {
        CRect tabRect;
        m_tab.GetClientRect(&tabRect);  
        tabRect.top += 22;
        tabRect.bottom -= 0;
        tabRect.left += 0;  
        tabRect.right -= 2;    
        m_splitterDialog.MoveWindow(tabRect);
    }
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
                        try
                        {              
                            m_recordset = m_connection->Execute(
                                (_bstr_t)query, NULL,adConnectUnspecified);
                        }
                        catch (_com_error e)
                        {
                            MessageBox(e.Description());
                            return FALSE;
                        }


                        if (m_page == 0)
                            updateAllSongList(m_recordset);
                        else if (m_page == 1)
                            updateSplitterWnd(m_recordset);                      
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
    CString query = GetBaseQuery();
    query += L" and 歌曲编号 between " + m_id_from + " and " + m_id_to;
    if (m_filterType != 3)
    {   
        CString flag;
        flag.Format(L"%d", m_filterType);
        query += L" and 标识=";
        query += flag;
    }

    try
    {   
        recordset = m_connection->Execute((_bstr_t)query, NULL, 
                                          adConnectUnspecified);
    }
    catch (_com_error e)
    {
        MessageBox(e.Description());
    }

}

bool CMVManagementDialog::initImageList(CBitmap* bitmap)
{
    if (!bitmap->GetSafeHandle())
        return false;
    
    BITMAP i;
    bitmap->GetBitmap(&i);
    if (!m_imageList.Create(i.bmWidth, i.bmHeight, ILC_COLOR24, 0, 100))
        return false;

    m_allSongList.SetImageList(&m_imageList, LVSIL_NORMAL);
    return true;
}

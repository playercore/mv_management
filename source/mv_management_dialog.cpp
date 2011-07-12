#include "mv_management_dialog.h"

#include <cmath>
#include <string>
#include <memory>
#include <vld.h>

#include <boost/filesystem.hpp>
#include "afxdialogex.h"
#include "afxdb.h"

#include "mv_management_app.h"
#include "search_dialog.h"
#include "sql_control.h"
#include "common.h"
#include "util.h"

using std::wstring;
using std::unique_ptr;
using boost::filesystem3::path;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* dataExch);    // DDX/DDV ֧��

// ʵ��
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


// Cmv_managementDlg �Ի���




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


// Cmv_managementDlg ��Ϣ�������

BOOL CMVManagementDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_icon, TRUE);			// ���ô�ͼ��
	SetIcon(m_icon, FALSE);		// ����Сͼ��
    	
	// TODO: �ڴ���Ӷ���ĳ�ʼ������
    // ��ȡ����IP
    m_ip = GetLocalIP().c_str();

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
	filterCombox->AddString(L"��ǰ��˸���");
	filterCombox->AddString(L"��һ������");
	filterCombox->AddString(L"�ڶ�������");
	filterCombox->AddString(L"ȫ������");
    filterCombox->AddString(L"��ɢMV");
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

	typeCombox->AddString(L"���׸������");

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

	m_tab.InsertItem(0,L"���и���");
	m_tab.InsertItem(1,L"ȥ���ظ�����");


    m_songFullList.Create(&m_tab, tabRect, IDC_LIST1);
    //m_songFullList.MoveWindow(&tabRect);

    m_splitterDialog.Create(IDD_DIALOG_SPLITTER, &m_tab);
    m_splitterDialog.MoveWindow(&tabRect);
  
    _RecordsetPtr recordset;
    simpleUpdate(recordset);
        
    if (recordset != NULL)
    {
        Fields* fields;
        recordset->get_Fields(&fields);
        int num = fields->GetCount() - 8; //����ʾ���8��

        for (long i = 0;i < num; ++i)
        {
            _variant_t index(i);    
            Field* field = NULL;
            FieldPtr p = fields->GetItem(index);
            _bstr_t name;
            name = p->GetName();
            CString strName =  (LPCTSTR)name;
            m_songFullList.InsertColumn(i, strName, LVCFMT_LEFT , 80, i);
        }

        updateAllSongList(recordset);
    }
        
    //m_splitterDialog.SetConnectionPtr(m_connection);
  
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CMVManagementDialog::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_icon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CMVManagementDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_icon);
}

void CMVManagementDialog::OnBnClickedOtherMVType()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GetDlgItem(IDC_COMBO_MV_TYPE)->EnableWindow(TRUE);
}

void CMVManagementDialog::OnBnClickedDefinedMVType(UINT id)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GetDlgItem(IDC_COMBO_MV_TYPE)->EnableWindow(FALSE);
}

void CMVManagementDialog::OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult)
{
	//�������Tab�İ�ť
	
	int num = m_tab.GetCurSel();
	m_page = num;
	switch(num)	
	{
 		case 0:
		{
			m_songFullList.ShowWindow(SW_SHOW);
			m_splitterDialog.ShowWindow(SW_HIDE);
//             int index = m_songFullList.GetSelectionMark();
//             m_songFullList.SetItemState(index, LVIS_SELECTED|LVIS_FOCUSED, 
//                 LVIS_SELECTED|LVIS_FOCUSED);
			break;
		}
		case 1:
		{
			m_songFullList.ShowWindow(SW_HIDE);		
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
    int num = fields->GetCount() - 8; //����ʾ���8��

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
            ((VT_NULL != t.vt) ? static_cast<int>(t) : 6000), md5);
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
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    _RecordsetPtr recordSet;
    simpleUpdate(recordSet);

    if (m_page == 1)
    {
        updateSplitterWnd(recordSet);
        return;
    }
    
    m_songFullList.DeleteAllItems();

    updateAllSongList(recordSet);
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
    
    if (item->IsInterlace == L"��")
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
    else if (musicType == L"�ݳ���")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_7);
    else if (musicType == L"Ϸ��")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_8);
    else if (musicType == L"�羰")
        CheckRadioButton(IDC_RADIO_6, IDC_RADIO_11, IDC_RADIO_9);
    else if (musicType == L"����")
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
    // TODO: �ڴ���ӿؼ�֪ͨ����������
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
            mvType = L"�ݳ���";
            break;
        }
        case IDC_RADIO_8:
        {
            mvType = L"Ϸ��";
            break;
        }
        case IDC_RADIO_9:
        {
            mvType = L"�羰";
            break;
        }
        case IDC_RADIO_10:
        {
            mvType = L"����";
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
        MessageBox(L"�������������");
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
        isInterlace = L"��";
    else if (r == IDC_RADIO_NO)
        isInterlace = L"û��";

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
    query = L"UPDATE CHECKED_ENCODE_FILE_INFO SET ԭ������ = ";
    query += trackType;
    query += L",�༭������ = '";
    query += name;
    query += L"',��ע = '";
    query += notes;
    query += L"',�Ƿ��н��� = '";
    query += isInterlace;
    query += L"',�������� = '";
    query += mvType;
    query += L"',���ʼ��� = ";
    query += quality;

    r = ((CButton*)GetDlgItem(IDC_CHECK_REPLACE))->GetCheck();
    
    if (r)
    {
        query += L",�¹�ϣֵ = '";
        query += newHash;
        query += L"'";
    }
    query += L",�ͻ���ip��ַ = '";
    query += m_ip;
    query += L"',�ύʱ�� = '";
    query += curTime;
    query += L"',����״̬ = '������' WHERE ������� = ";
    query += m_curSelectedId;
    
    CSQLControl::get()->UpdateByString((LPTSTR)(LPCTSTR)query);
    
    m_songFullList.DeleteAllItems();

    _RecordsetPtr recordSet;
    simpleUpdate(recordSet);
    updateAllSongList(recordSet);
    updateSplitterWnd(recordSet);
}


void CMVManagementDialog::OnCbnSelchangeComboFilterCondition()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
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

    CRect clientRect;  
    GetClientRect(&clientRect);  

    // TODO: �ڴ˴������Ϣ����������
     
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

    if (m_songFullList.HasBeenCreated())
    {
        CRect tabRect;
        m_tab.GetClientRect(&tabRect);  
        tabRect.top += 22;
        tabRect.bottom -= 0;
        tabRect.left += 0;  
        tabRect.right -= 2;    
        m_songFullList.MoveWindow(tabRect);
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
    // TODO: �ڴ����ר�ô����/����û���
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
                            updateAllSongList(recordSet);
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
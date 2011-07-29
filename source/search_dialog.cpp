#include "search_dialog.h"

#include <boost/lexical_cast.hpp>

#include "afxdialogex.h"
#include "common.h"
#include "sql_control.h"

using std::wstring;
using boost::lexical_cast;

IMPLEMENT_DYNAMIC(CSearchDialog, CDialogEx)

CSearchDialog::CSearchDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSearchDialog::IDD, pParent)
{

}

CSearchDialog::~CSearchDialog()
{
}

void CSearchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSearchDialog, CDialogEx)
    ON_BN_CLICKED(IDOK, &CSearchDialog::OnBnClickedOk)
    ON_EN_CHANGE(IDC_EDIT_OLDHASH1, &CSearchDialog::OnEnChangeEditOldhash1)
    ON_EN_CHANGE(IDC_EDIT_NAME1, &CSearchDialog::OnEnChangeEditName1)
    ON_EN_CHANGE(IDC_EDIT_STATE1, &CSearchDialog::OnEnChangeEditState1)
    ON_EN_CHANGE(IDC_EDIT_NOTES1, &CSearchDialog::OnEnChangeEditNotes1)
    ON_EN_CHANGE(IDC_EDIT_NEWHASH1, &CSearchDialog::OnEnChangeEditNewhash1)
    ON_EN_CHANGE(IDC_EDIT_IDFROM1, &CSearchDialog::OnEnChangeEditIdfrom1)
    ON_EN_CHANGE(IDC_EDIT_IDTO1, &CSearchDialog::OnEnChangeEditIdto1)
    ON_CBN_SELCHANGE(IDC_COMBO_INTERLACE1, &CSearchDialog::OnCbnSelchangeComboInterlace1)
    ON_CBN_SELCHANGE(IDC_COMBO_TRACKTYPE1, &CSearchDialog::OnCbnSelchangeComboTracktype1)
    ON_CBN_EDITCHANGE(IDC_COMBO_MVTYPE1, &CSearchDialog::OnCbnEditchangeComboMvtype1)
    ON_CBN_SELCHANGE(IDC_COMBO_MVTYPE1, &CSearchDialog::OnCbnSelchangeComboMvtype1)
    ON_CBN_SELCHANGE(IDC_COMBO_QUALITY1, &CSearchDialog::OnCbnSelchangeComboQuality1)
END_MESSAGE_MAP()

BOOL CSearchDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CComboBox* combox = (CComboBox*)GetDlgItem(IDC_COMBO_INTERLACE1);
    combox->AddString(L"没有");
    combox->AddString(L"有");
    combox->SetCurSel(0);
    
    combox = (CComboBox*)GetDlgItem(IDC_COMBO_TRACKTYPE1);
    combox->AddString(L"左声道是原唱");
    combox->AddString(L"右声道是原唱");
    combox->AddString(L"音轨1是原唱");
    combox->AddString(L"音轨2是原唱");
    combox->AddString(L"无原唱");
    combox->AddString(L"无伴奏");
    combox->SetCurSel(0);

    combox = (CComboBox*)GetDlgItem(IDC_COMBO_MVTYPE1);
    combox->AddString(L"MTV");
    combox->AddString(L"演唱会");
    combox->AddString(L"戏曲");
    combox->AddString(L"风景");
    combox->AddString(L"串烧");
    combox->SetCurSel(0); 

    combox = (CComboBox*)GetDlgItem(IDC_COMBO_QUALITY1);
    for (int i = 1; i <= 9; ++i)
    {
        CString str;
        str.Format(L"%d",i);
        combox->AddString(str);
    }
    combox->SetCurSel(0); 
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


// CSearchDialog 消息处理程序


void CSearchDialog::OnBnClickedOk()
{
    // TODO: 在此添加控件通知处理程序代码
    BOOL useOldHash = ((CButton*)GetDlgItem(IDC_CHECK_USE1))->GetCheck();
    BOOL useName = ((CButton*)GetDlgItem(IDC_CHECK_USE2))->GetCheck();
    BOOL useInterlace = ((CButton*)GetDlgItem(IDC_CHECK_USE3))->GetCheck();
    BOOL useState = ((CButton*)GetDlgItem(IDC_CHECK_USE4))->GetCheck();
    BOOL useNotes = ((CButton*)GetDlgItem(IDC_CHECK_USE5))->GetCheck();
    BOOL useTrackType = ((CButton*)GetDlgItem(IDC_CHECK_USE6))->GetCheck();
    BOOL useMVType = ((CButton*)GetDlgItem(IDC_CHECK_USE7))->GetCheck();
    BOOL useNewHash = ((CButton*)GetDlgItem(IDC_CHECK_USE8))->GetCheck();
    BOOL useQuality = ((CButton*)GetDlgItem(IDC_CHECK_USE9))->GetCheck();
    BOOL useID = ((CButton*)GetDlgItem(IDC_CHECK_USE10))->GetCheck();

    if (!useOldHash && !useName && !useInterlace && !useState && !useNotes &&
        !useTrackType && !useMVType && !useNewHash && !useQuality && !useID)
        return CDialogEx::OnOK();

    m_query = CSQLControl::GetBaseQuery();
    if (useOldHash)
    {
          m_query += L" AND 旧哈希值 = '";
          CString str;
          GetDlgItem(IDC_EDIT_OLDHASH1)->GetWindowText(str);
          m_query += str.GetBuffer();
          m_query += L"'";
    }

    if (useName)
    {
        m_query += L" AND 编辑重命名 LIKE '%";
        CString str;
        GetDlgItem(IDC_EDIT_NAME1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"%'";
    }

    if (useInterlace)
    {
        m_query += L" AND 是否有交错 = '";
        CString str;
        GetDlgItem(IDC_COMBO_INTERLACE1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useState)
    {
        m_query += L" AND 歌曲状态 = '";
        CString str;
        GetDlgItem(IDC_EDIT_STATE1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useNotes)
    {
        m_query += L" AND 备注 = '";
        CString str;
        GetDlgItem(IDC_EDIT_NOTES1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useTrackType)
    {
        m_query += L" AND 原唱音轨 = '";
        CString str;
        GetDlgItem(IDC_COMBO_TRACKTYPE1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useMVType)
    {
        m_query += L" AND 歌曲类型 = '";
        CString str;
        GetDlgItem(IDC_COMBO_MVTYPE1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useNewHash)
    {
        m_query += L" AND 新哈希值 = '";
        CString str;
        GetDlgItem(IDC_EDIT_NEWHASH1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L"'";
    }

    if (useQuality)
    {
        m_query += L" AND 画质级别 = ";
        int index = ((CComboBox*)(GetDlgItem(IDC_COMBO_QUALITY1)))->GetCurSel();
        m_query += lexical_cast<wstring>(index + 1);
    }

    if (useID)
    {
        m_query += L" AND 序号 BETWEEN ";
        CString str;
        GetDlgItem(IDC_EDIT_IDFROM1)->GetWindowText(str);
        m_query += str.GetBuffer();
        m_query += L" AND ";
        GetDlgItem(IDC_EDIT_IDTO1)->GetWindowText(str);
        m_query += str.GetBuffer();
    }

    CDialogEx::OnOK();
}


void CSearchDialog::OnEnChangeEditOldhash1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str;
    GetDlgItem(IDC_EDIT_OLDHASH1)->GetWindowText(str);
    if (str != L"")
        ((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(FALSE);
}


void CSearchDialog::OnEnChangeEditName1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str;
    GetDlgItem(IDC_EDIT_NAME1)->GetWindowText(str);
    if (str != L"")
        ((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(FALSE);
}


void CSearchDialog::OnEnChangeEditState1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str;
    GetDlgItem(IDC_EDIT_STATE1)->GetWindowText(str);
    if (str != L"")
        ((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(FALSE);
}


void CSearchDialog::OnEnChangeEditNotes1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str;
    GetDlgItem(IDC_EDIT_NOTES1)->GetWindowText(str);
    if (str != L"")
        ((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(FALSE);
}


void CSearchDialog::OnEnChangeEditNewhash1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str;
    GetDlgItem(IDC_EDIT_NEWHASH1)->GetWindowText(str);
    if (str != L"")
        ((CButton*)GetDlgItem(IDC_CHECK8))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK8))->SetCheck(FALSE);
}


void CSearchDialog::OnEnChangeEditIdfrom1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str1;
    GetDlgItem(IDC_EDIT_IDFROM1)->GetWindowText(str1);
    CString str2;
    GetDlgItem(IDC_EDIT_IDTO1)->GetWindowText(str2);

    // 禁止输入非数字字符
    for (int index = 0; index < str1.GetLength(); index++)  
    {  
        if ((str1[index] > '9') || (str1[index] < '0'))  
        {  
            str1 = str1.Left(index) + str1.Right(str1.GetLength() - index - 1);  
            GetDlgItem(IDC_EDIT_IDFROM1)->SetWindowText(str1);    
        }  
    }  

    if (str1 != L"" && str2 != L"")
        ((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(FALSE);

}


void CSearchDialog::OnEnChangeEditIdto1()
{
    // TODO:  在此添加控件通知处理程序代码
    CString str1;
    GetDlgItem(IDC_EDIT_IDFROM1)->GetWindowText(str1);
    CString str2;
    GetDlgItem(IDC_EDIT_IDTO1)->GetWindowText(str2);

    for (int index = 0; index < str2.GetLength(); index++ )  
    {  
        if ((str2[index] > '9') || (str2[index] < '0'))  
        {  
            str2 = str2.Left(index) + str2.Right(str2.GetLength() - index - 1);  
            GetDlgItem(IDC_EDIT_IDTO1)->SetWindowText(str2);    
        }  
    }  
    if (str1 != L"" && str2 != L"")
        ((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(TRUE);
    else
        ((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(FALSE);
}


void CSearchDialog::OnCbnSelchangeComboInterlace1()
{
    // TODO: 在此添加控件通知处理程序代码
    ((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(TRUE);
}


void CSearchDialog::OnCbnSelchangeComboTracktype1()
{
    // TODO: 在此添加控件通知处理程序代码
    ((CButton*)GetDlgItem(IDC_CHECK6))->SetCheck(TRUE);
}


void CSearchDialog::OnCbnEditchangeComboMvtype1()
{
    // TODO: 在此添加控件通知处理程序代码
    ((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(TRUE);
}


void CSearchDialog::OnCbnSelchangeComboMvtype1()
{
    // TODO: 在此添加控件通知处理程序代码
    ((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(TRUE);
}


void CSearchDialog::OnCbnSelchangeComboQuality1()
{
    // TODO: 在此添加控件通知处理程序代码
    ((CButton*)GetDlgItem(IDC_CHECK9))->SetCheck(TRUE);
}

wstring CSearchDialog::GetQuery()
{
    return m_query;
}

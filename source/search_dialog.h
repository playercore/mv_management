#pragma once

#include "mfc_predefine.h"
#include "resource/resource.h" // main symbols

// CSearchDialog 对话框

class CSearchDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSearchDialog)

public:
	CSearchDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSearchDialog();
    CString GetQuery();

// 对话框数据
	enum { IDD = IDD_DIALOG_SEARCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedOk();
    afx_msg void OnEnChangeEditOldhash1();
    afx_msg void OnEnChangeEditName1();
    afx_msg void OnEnChangeEditState1();
    afx_msg void OnEnChangeEditNotes1();
    afx_msg void OnEnChangeEditNewhash1();
    afx_msg void OnEnChangeEditIdfrom1();
    afx_msg void OnEnChangeEditIdto1();
    afx_msg void OnCbnSelchangeComboInterlace1();
    afx_msg void OnCbnSelchangeComboTracktype1();
    afx_msg void OnCbnEditchangeComboMvtype1();
    afx_msg void OnCbnSelchangeComboMvtype1();
    afx_msg void OnCbnSelchangeComboQuality1();

    CString m_oldHash;
    CString m_name;
    CString m_isInterlace;
    CString m_state;
    CString m_notes;
    CString m_trackType;
    CString m_mvType;
    CString m_newHash;
    CString m_quality;
    CString m_idFrom;
    CString m_idTo;
    CString m_query;
};

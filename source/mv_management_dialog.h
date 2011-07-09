#pragma once

#include "mfc_predefine.h"
#include "afxwin.h"
#include "afxcmn.h"

#include "third_party/chromium/base/thread.h"
#include "resource/resource.h" // main symbols

#include "my_list_ctrl.h"
#include "all_song_list_dialog.h"
#include "splitter_dialog.h"
#include "list_item_define.h"
#import "msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

// CMVManagementDialog 对话框
class CMVManagementDialog : public CDialogEx
{
public:
	enum { IDD = IDD_MV_MANAGEMENT_DIALOG };
    enum KUploadState
    {
        uploaded = 0,
        normal = 1,
        fail = 2
    };

	CMVManagementDialog(CWnd* parent = NULL);	// 标准构造函数

protected:
	virtual void DoDataExchange(CDataExchange* dataExch);	// DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT id, LPARAM param);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
	
private:
	afx_msg void OnBnClickedOtherMVType(); //MV类型为其他
    afx_msg void OnBnClickedDefinedMVType(UINT id); //MV类型为已定义的
	afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedButtonSearch();
    void updateAllSongList(_RecordsetPtr recordset);
    afx_msg LRESULT updateListSel(WPARAM waram, LPARAM param);
    afx_msg void OnBnClickedButtonSubmit();
    afx_msg void OnCbnSelchangeComboFilterCondition();
    void updateSplitterWnd(_RecordsetPtr recordset);
    afx_msg LRESULT updateRightListSel(WPARAM waram, LPARAM param);
    void getLocalIP();
    void getCurTime(CString& time);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    void simpleUpdate(_RecordsetPtr& recordset);
    void clearImageList();
    bool initImageList(CBitmap* bitmap);

	HICON m_icon;
	CMyListCtrl m_allSongList;
	CTabCtrl m_tab;
	CAllSongListDialog m_dialog1;
    _ConnectionPtr m_connection;//connection   object's   pointer     
    _RecordsetPtr m_recordset;
      
    CString m_id_from;
    CString m_id_to;
    CSplitterDialog m_splitterDialog;
    int m_filterType;
    int m_page; //0：所有歌曲 ，1：去除重复歌曲
    CString m_ip;
    CString m_curSelectedId;  
    CImageList m_imageList;
};

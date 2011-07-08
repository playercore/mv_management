#pragma once

#include "mfc_predefine.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "afxcview.h"

#include "resource/resource.h"

#import "c:\program files\common files\system\ado\msado15.dll" \
    no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

// CSplitterDialog 对话框

class CSplitterDialog : public CDialog
{
public:
	enum { IDD = IDD_DIALOG2 };
	CSplitterDialog(CWnd* pParent = NULL);   // 标准构造函数
    CSplitterDialog(_ConnectionPtr connect, CWnd* pParent = NULL);
	virtual ~CSplitterDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
   virtual BOOL OnInitDialog();
   void SetConnectionPtr(_ConnectionPtr connection);
   void Search(_RecordsetPtr recordset);
   void SelectItem();

// 对话框数据


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT updateLeftListSel(WPARAM waram, LPARAM param);

private:	
    DECLARE_DYNAMIC(CSplitterDialog)
    CSplitterWnd m_wndSplitter;
    CFrameWnd* m_pMyFrame;
    _ConnectionPtr m_connection;
};

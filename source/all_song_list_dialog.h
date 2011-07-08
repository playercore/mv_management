#pragma once
#include "mfc_predefine.h"
#include "resource/resource.h"


// CAllSongListDialog 对话框

class CAllSongListDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CAllSongListDialog)

public:
	CAllSongListDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAllSongListDialog();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};

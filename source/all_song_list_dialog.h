#pragma once
#include "mfc_predefine.h"
#include "resource/resource.h"


// CAllSongListDialog �Ի���

class CAllSongListDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CAllSongListDialog)

public:
	CAllSongListDialog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CAllSongListDialog();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};

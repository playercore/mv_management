#include "all_song_list_dialog.h"
#include "afxdialogex.h"


// CAllSongListDialog �Ի���

IMPLEMENT_DYNAMIC(CAllSongListDialog, CDialogEx)

CAllSongListDialog::CAllSongListDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAllSongListDialog::IDD, pParent)
{

}

CAllSongListDialog::~CAllSongListDialog()
{
}

void CAllSongListDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAllSongListDialog, CDialogEx)
END_MESSAGE_MAP()


// CAllSongListDialog ��Ϣ�������

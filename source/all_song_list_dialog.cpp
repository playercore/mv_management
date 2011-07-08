#include "all_song_list_dialog.h"
#include "afxdialogex.h"


// CAllSongListDialog 对话框

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


// CAllSongListDialog 消息处理程序

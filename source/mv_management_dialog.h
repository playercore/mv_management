#pragma once

#include "mfc_predefine.h"
#include "afxwin.h"
#include "afxcmn.h"

#include "third_party/chromium/base/thread.h"
#include "resource/resource.h" // main symbols

#include "song_info_list_control.h"
#include "all_song_list_dialog.h"
#include "splitter_dialog.h"
#include "list_item_define.h"
#include "collapse_button.h"
#include "guide_control.h"

#import "msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

// CMVManagementDialog �Ի���
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

    CMVManagementDialog(CWnd* parent = NULL);    // ��׼���캯��

protected:
    virtual void DoDataExchange(CDataExchange* dataExch);    // DDX/DDV ֧��
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT id, LPARAM param);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnClose();
    DECLARE_MESSAGE_MAP()
    
private:
    afx_msg void OnBnClickedOtherMVType(); //MV����Ϊ����
    afx_msg void OnBnClickedDefinedMVType(UINT id); //MV����Ϊ�Ѷ����
    afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedButtonSearch();
    void updateSongFullListByRecordset(_RecordsetPtr recordset);
    afx_msg LRESULT updateListSel(WPARAM waram, LPARAM param);
    afx_msg void OnBnClickedButtonSubmit();
    afx_msg void OnCbnSelchangeComboFilterCondition();
    void updateSplitterWnd(_RecordsetPtr recordset);
    afx_msg LRESULT updateRightListSel(WPARAM waram, LPARAM param);
    void getCurTime(CString& time);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    void simpleUpdate(_RecordsetPtr& recordset);
    void updateSongFullListByString(TListItem* item, bool isReplace);
    afx_msg void OnBnClickedButtonOnlyList();
    LRESULT OnSongFullListDisplaySwitch(WPARAM w, LPARAM l);
    LRESULT OnPictureUpLoaded(WPARAM w, LPARAM l);
    void Layout(int cx, int cy);
    void CreateStatusBar();
    void updateStatusForSong();
    void updateStatusForPicture();

    HICON m_icon;
    SongInfoListControl m_songFullList;
    CTabCtrl m_tab;
    CString m_id_from;
    CString m_id_to;
    CSplitterDialog m_splitterDialog;
    int m_filterType;
    int m_page; //0�����и��� ��1��ȥ���ظ�����
    int m_curListSelItem;
    GuideControl guide_;
    CollapseButton collapse_;
    int tabTop_;
    CStatic guideText1_;
    CStatic guideText2_;
};

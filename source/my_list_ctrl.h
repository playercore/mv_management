#pragma once

// CMyListCtrl

class CMyListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CMyListCtrl)

public:
	CMyListCtrl();
	virtual ~CMyListCtrl();
    void SetTrackCountCol(int col);
    void SetPathCol(int col);

protected:
	DECLARE_MESSAGE_MAP()

private:
    afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    void sort(int column, bool isAscending);
    static int CALLBACK compareFunction(LPARAM lParam1, LPARAM lParam2, 
                                        LPARAM lParamData);
	afx_msg void OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
    void playMV(int row);
    void previewMV(int item);

    bool m_isAscending;
    int m_sortColumn;
    int m_trackCountCol;
    int m_pathCol;
};



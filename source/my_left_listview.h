#pragma once

#include "afxcview.h"

class CMyLeftListView : public CListView
{
public:
    CMyLeftListView();
    virtual ~CMyLeftListView();

protected:
    DECLARE_MESSAGE_MAP()

private:
    DECLARE_DYNCREATE(CMyLeftListView)
    afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    void sort(int column, bool isAscending);
    static int CALLBACK compareFunction(LPARAM lParam1, LPARAM lParam2, 
        LPARAM lParamData);
    afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult); 


    bool m_isAscending;
    int m_sortColumn;
};


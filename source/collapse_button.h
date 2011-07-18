#ifndef _COLLAPSE_BUTTON_H_
#define _COLLAPSE_BUTTON_H_

#include "mfc_predefine.h"
#include <afxwin.h>

class CollapseButton : public CButton
{
public:
    CollapseButton();
    virtual ~CollapseButton();

    bool IsCollapsed() const { return collapsed_; }

protected:
    DECLARE_MESSAGE_MAP()

    LRESULT OnClicked(WPARAM w, LPARAM l);
    void OnPaint();

private:
    void Paint(bool erase);

    bool collapsed_;
};

#endif  // _COLLAPSE_BUTTON_H_
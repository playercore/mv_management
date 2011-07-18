#include "collapse_button.h"

CollapseButton::CollapseButton()
    : CButton()
    , collapsed_(false)
{
}

CollapseButton::~CollapseButton()
{
}

BEGIN_MESSAGE_MAP(CollapseButton, CButton)
    ON_MESSAGE(WM_LBUTTONUP, &CollapseButton::OnClicked)
    ON_WM_PAINT()
END_MESSAGE_MAP()

LRESULT CollapseButton::OnClicked(WPARAM w, LPARAM l)
{
    collapsed_ = !collapsed_;
    return CButton::DefWindowProc(WM_LBUTTONUP, w, l);
}

void CollapseButton::OnPaint()
{
    CButton::OnPaint();
    Paint(false);
}

void CollapseButton::Paint(bool erase)
{
    CRect rect;
    GetClientRect(&rect);

    const int width = rect.Width();
    const int height = rect.Height();
    const int radius = 4;
    const int left = (width - radius * 2) / 2 - 1;
    const int top = (height - radius) / 2;

    CDC* clientDc = GetDC();
    if (collapsed_) {
        clientDc->MoveTo(left, top);
        clientDc->LineTo(left + radius, top + radius);
        clientDc->LineTo(left + radius * 2 + 1, top - 1);
    } else {
        clientDc->MoveTo(left, top + radius);
        clientDc->LineTo(left + radius, top);
        clientDc->LineTo(left + radius * 2 + 1, top + radius + 1);
    }
    ReleaseDC(clientDc);
}
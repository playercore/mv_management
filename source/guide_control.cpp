#include "guide_control.h"

#include "resource/resource.h"
#include "util.h"

GuideControl::GuideControl()
    : CStatic()
    , uploadDone_()
    , uploadDoneMask_()
    , uploadFailed_()
    , uploadFailedMask_()
{
}

GuideControl::~GuideControl()
{
}

void GuideControl::Init()
{
    ModifyStyle(SS_GRAYFRAME, 0);

    LoadResizedNotificationImage(IDB_PNG_UPLOAD_DONE, &uploadDone_,
                                 &uploadDoneMask_);
    LoadResizedNotificationImage(IDB_PNG_UPLOAD_FAILED, &uploadFailed_,
                                 &uploadFailedMask_);
}

BEGIN_MESSAGE_MAP(GuideControl, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void GuideControl::OnPaint()
{
    CStatic::OnPaint();

    CDC* dc = GetDC();
    if (!dc)
        return;

    BITMAP markInfo;
    uploadDone_.GetBitmap(&markInfo);

    CRect rect;
    GetClientRect(&rect);

    CDC markDc;
    markDc.CreateCompatibleDC(dc);

    CGdiObject* o = markDc.SelectObject(&uploadDone_);

    dc->MaskBlt(0, 0, markInfo.bmWidth, markInfo.bmHeight, &markDc, 0, 0,
                uploadDoneMask_, 0, 0, MAKEROP4(SRCPAINT, SRCCOPY));

    markDc.SelectObject(&uploadFailed_);
    dc->MaskBlt(170, 0, markInfo.bmWidth, markInfo.bmHeight, &markDc, 0, 0,
                uploadFailedMask_, 0, 0, MAKEROP4(SRCPAINT, SRCCOPY));

    markDc.SelectObject(o);

    ReleaseDC(dc);
}

void GuideControl::LoadResizedNotificationImage(int resourceID, CBitmap* mark,
                                                CBitmap* mask)
{
    CBitmap b1;
    CBitmap b2;
    LoadNotificationImage(resourceID, &b1, &b2);

    BITMAP info;
    b1.GetBitmap(&info);

    CDC* dc = GetDC();
    CDC memDC1;
    memDC1.CreateCompatibleDC(dc);

    CDC memDC2;
    memDC2.CreateCompatibleDC(dc);
    
    const int length = 16;
    mark->CreateCompatibleBitmap(dc, length, length);
    memDC2.SetStretchBltMode(COLORONCOLOR);
    CGdiObject* o1 = memDC1.SelectObject(&b1);
    CGdiObject* o2 = memDC2.SelectObject(mark);

    memDC2.StretchBlt(0, 0, length, length, &memDC1, 0, 0, info.bmWidth,
                      info.bmHeight, SRCCOPY);

    mask->CreateBitmap(length, length, 1, 1, NULL);
    memDC1.SelectObject(&b2);
    memDC2.SelectObject(mask);

    memDC2.StretchBlt(0, 0, length, length, &memDC1, 0, 0, info.bmWidth,
                      info.bmHeight, SRCCOPY);

    memDC2.SelectObject(o2);
    memDC1.SelectObject(o1);
    ReleaseDC(dc);
}
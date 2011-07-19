#ifndef _GUIDE_CONTROL_H_
#define _GUIDE_CONTROL_H_

#include "mfc_predefine.h"
#include <afxwin.h>

class GuideControl : public CStatic
{
public:
    GuideControl();
    ~GuideControl();

    void Init();

protected:
    DECLARE_MESSAGE_MAP()

    void OnPaint();

private:
    void LoadResizedNotificationImage(int resourceID, CBitmap* mark,
                                      CBitmap* mask);

    CBitmap uploadDone_;
    CBitmap uploadDoneMask_;
    CBitmap uploadFailed_;
    CBitmap uploadFailedMask_;
};

#endif  // _GUIDE_CONTROL_H_
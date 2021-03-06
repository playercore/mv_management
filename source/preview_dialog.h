#ifndef _PREVIEW_DIALOG_H_
#define _PREVIEW_DIALOG_H_

#include <memory>
#include <string>

#include <boost/filesystem.hpp>
#include "mfc_predefine.h"
#include <afxwin.h>

#include "resource/resource.h" // main symbols
#include "afxcmn.h"
#include "basictypes.h"

struct AVFrame;
class PictureControl : public CStatic
{
public:
    PictureControl();
    ~PictureControl();

    void SetPicture(AVFrame* frame);

protected:
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()

private:
    void Paint();

    std::unique_ptr<void, void (__stdcall*)(void*)> pic_;
};

//------------------------------------------------------------------------------
struct AVFormatContext;
struct AVCodecContext;
class PreviewDialog : public CDialog
{
public:
    enum { IDD = IDD_DIALOG_PREVIEW };

    PreviewDialog(CWnd* parent, const boost::filesystem3::path& mvPath,
                  const boost::filesystem3::path& previewPath,
                  int64 initialPreviewTime, const std::wstring& songName);
    virtual ~PreviewDialog();

    int64 GetPreivewTime();

protected:
    virtual void DoDataExchange(CDataExchange* dataExch); // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnHScroll(UINT code, UINT pos, CScrollBar* scrollBar);
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCheck1MinOnly();

private:
    void Open();
    void GeneratePreview(int64 time);
    std::wstring CreatePreviewTime(int64 t);

    boost::filesystem3::path mvPath_;
    boost::filesystem3::path previewPath_;
    std::wstring songName_;
    CSliderCtrl timeSlider_;
    PictureControl preview_;
    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext*)> media_;
    std::unique_ptr<AVCodecContext, int (*)(AVCodecContext*)> codecCont_;
    std::unique_ptr<AVFrame, void (*)(void*)> frameRGB_;
    int videoStreamIndex_;
    int64 duraion_;
    int64 lastPreviewTime_;
    CStatic tips_;
    CButton firstMinOnly_;
    CStatic timeRangeText_;
    int64 timeRange_;
};

#endif  // _PREVIEW_DIALOG_H_
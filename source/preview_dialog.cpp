#include "preview_dialog.h"

#include <cassert>
#include <string>
#include <sstream>
#include <memory>

#include <boost/algorithm/string.hpp>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
#include "util.h"
#include "jpeg_tool.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using boost::filesystem3::path;
using boost::filesystem3::is_regular_file;
using boost::algorithm::iequals;

PictureControl::PictureControl()
    : CStatic()
    , pic_(NULL, reinterpret_cast<void (__stdcall*)(void*)>(DeleteObject))
{
}

PictureControl::~PictureControl()
{
}

void PictureControl::SetPicture(AVFrame* frame)
{
    assert(frame);
    BITMAPINFOHEADER infoHeader = {0};
    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = frame->width;
    infoHeader.biHeight = -frame->height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
    CDC* dc = GetDC();
    HBITMAP b = CreateDIBitmap(dc->GetSafeHdc(), &infoHeader, CBM_INIT,
                               frame->data[0],
                               reinterpret_cast<BITMAPINFO*>(&infoHeader),
                               DIB_RGB_COLORS);
    pic_.reset(b);
    ReleaseDC(dc);
    Paint();
}

BEGIN_MESSAGE_MAP(PictureControl, CStatic)
    ON_WM_PAINT()
    ON_WM_CREATE()
END_MESSAGE_MAP()

void PictureControl::OnPaint()
{
    CStatic::OnPaint();
    Paint();
}

void PictureControl::Paint()
{
    if (pic_) {
        CDC* d = GetDC();
        if (d) {
            d->SetStretchBltMode(HALFTONE);

            CDC c;
            if (c.CreateCompatibleDC(d)) {
                RECT r;
                GetClientRect(&r);

                BITMAP i = {0};
                GetObject(pic_.get(), sizeof(i), &i);

                c.SelectObject(pic_.get());
                d->StretchBlt(0, 0, r.right, r.bottom, &c, 0, 0, i.bmWidth,
                              i.bmHeight, SRCCOPY);
            }
            ReleaseDC(d);
        }
    }
}

//------------------------------------------------------------------------------
PreviewDialog::PreviewDialog(CWnd* parent, const path& mvPath,
                             const path& previewPath, int64 initialPreviewTime)
    : CDialog(PreviewDialog::IDD, parent)
    , mvPath_(mvPath)
    , previewPath_(previewPath)
    , timeSlider_()
    , preview_()
    , media_(NULL, av_close_input_file)
    , codecCont_(NULL, avcodec_close)
    , frameRGB_(avcodec_alloc_frame(), av_free)
    , videoStreamIndex_(-1)
    , duraion_(0)
    , lastPreviewTime_(initialPreviewTime)
{
    assert(is_regular_file(mvPath_));
    av_register_all();
    avcodec_register_all();
}

PreviewDialog::~PreviewDialog()
{
}

int64 PreviewDialog::GetPreivewTime()
{
    return lastPreviewTime_;
}

void PreviewDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialog::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_SLIDER_TIMESTAMP, timeSlider_);
    DDX_Control(dataExch, IDC_STATIC_PREVIEW, preview_);
    DDX_Control(dataExch, IDC_STATIC_TIPS, tips_);
}

BEGIN_MESSAGE_MAP(PreviewDialog, CDialog)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDOK, &PreviewDialog::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL PreviewDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    wstring caption = L"Preview of file : " + mvPath_.filename().wstring();
    SetWindowText(caption.c_str());

    Open();

    timeSlider_.SetRange(0, 100);
    timeSlider_.SetPos(static_cast<int>(lastPreviewTime_ * 100 / duraion_));
    GeneratePreview(lastPreviewTime_);
    UpdatePreviewTime();
    return TRUE;
}

void PreviewDialog::OnHScroll(UINT code, UINT pos, CScrollBar* scrollBar)
{
    if ((SB_THUMBTRACK == LOWORD(code)) || (SB_ENDSCROLL == LOWORD(code)))
        if (duraion_) {
            int64 time = timeSlider_.GetPos() * duraion_ / 100;
            if (lastPreviewTime_ != time) {
                GeneratePreview(time);
                lastPreviewTime_ = time;
                UpdatePreviewTime();
            }
        }

    CDialog::OnHScroll(code, pos, scrollBar);
}

void PreviewDialog::OnBnClickedOk()
{
    Jpeg::SaveToJPEGFile(previewPath_, frameRGB_.get());
    CDialog::OnOK();
}

void PreviewDialog::Open()
{
    assert(is_regular_file(mvPath_));
    assert(iequals(mvPath_.extension().wstring(), L".mkv"));

    AVFormatContext* media;
    if (av_open_input_file(&media,
                           WideCharToMultiByte(mvPath_.wstring()).c_str(), NULL,
                           0, NULL))
        return;

    media_.reset(media);
    if (av_find_stream_info(media) < 0)
        return;

    // Get video stream.
    videoStreamIndex_ = -1;
    for (int i = 0; i < static_cast<int>(media->nb_streams); ++i) {
        if (AVMEDIA_TYPE_VIDEO == media->streams[i]->codec->codec_type) {
            videoStreamIndex_ = i;
            break;
        }
    }

    if (videoStreamIndex_ < 0)
        return;

    // Initialize decoding context.
    AVCodec* codec = avcodec_find_decoder(
        media->streams[videoStreamIndex_]->codec->codec_id);
    if (!codec)
        return;

    codecCont_.reset(media->streams[videoStreamIndex_]->codec);
    if (avcodec_open(codecCont_.get(), codec) < 0)
        return;

    duraion_ = media_->duration / 1000;

    // Initialize RGB format frame.
    const int previewWidth = codecCont_->width / 6;
    const int previewHeight = codecCont_->height / 6;
    const int bufSize = avpicture_get_size(PIX_FMT_RGB24, previewWidth,
                                           previewHeight);
    avpicture_fill(reinterpret_cast<AVPicture*>(frameRGB_.get()),
                   reinterpret_cast<uint8_t*>(av_malloc(bufSize)),
                   PIX_FMT_RGB24, previewWidth, previewHeight);
    frameRGB_->width = previewWidth;
    frameRGB_->height = previewHeight;
}

void PreviewDialog::GeneratePreview(int64 time)
{
    if (!media_ || (videoStreamIndex_ < 0) || !codecCont_)
        return;

    // Seek to the specified position.
    if (av_seek_frame(media_.get(), videoStreamIndex_, time, 0) < 0)
        return;

    unique_ptr<AVFrame, void (*)(void*)> frame(avcodec_alloc_frame(),
                                               av_free);
    do {
        AVPacket packet = {0};
        if (av_read_frame(media_.get(), &packet) < 0)
            return;

        if (packet.stream_index != videoStreamIndex_)
            continue;
        
        // Decode.
        int frameFinished;
        int bytesDecoded = avcodec_decode_video2(codecCont_.get(), frame.get(),
                                                 &frameFinished, &packet);
        av_free_packet(&packet);
        if (bytesDecoded < 0)
            return;

        if (frameFinished && bytesDecoded)
            break;
    } while (true);

    SwsContext* scaleCont = sws_getContext(codecCont_->width,
                                           codecCont_->height,
                                           codecCont_->pix_fmt,
                                           frameRGB_->width,
                                           frameRGB_->height, PIX_FMT_RGB24,
                                           SWS_BICUBIC, NULL, NULL, NULL);
    if (!scaleCont)
        return;

    sws_scale(scaleCont, frame.get()->data, frame.get()->linesize, 0,
              codecCont_->height, frameRGB_.get()->data,
              frameRGB_.get()->linesize);
    preview_.SetPicture(frameRGB_.get());
}

void PreviewDialog::UpdatePreviewTime()
{
    int64 sec = lastPreviewTime_ / 1000;
    int64 min = sec / 60;
    sec = sec - min * 60;
    wstringstream m;
    m.fill('0');
    m.width(2);
    m << min;

    wstringstream s;
    s.fill('0');
    s.width(2);
    s << sec;
    tips_.SetWindowText((L"Preview at " + m.str() + L" : " + s.str()).c_str());
}
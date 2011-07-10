#include "jpeg_tool.h"

#include <cstdio>
#include <memory>

#include <boost/filesystem.hpp>
#include <windows.h>

extern "C" {
#include "package/ffmpeg/include/libavformat/avformat.h"
#include "package/ffmpeg/include/libavcodec/avcodec.h"
#include "package/ffmpeg/include/libswscale/swscale.h"
#include "package/jpeg/include/jconfig.h"
#include "package/jpeg/include/jpeglib.h"
}
#include "basictypes.h"

using std::unique_ptr;
using boost::filesystem3::path;
using boost::filesystem3::file_size;

namespace {
void MyErrorExit(j_common_ptr info)
{
    char buffer[JMSG_LENGTH_MAX];
    info->err->format_message(info, buffer);
    throw std::exception(buffer);
}
}

void Jpeg::SaveToJPEGFile(const path& jpegPath, AVFrame* frame)
{
    assert(frame);
    unique_ptr<FILE, int (*)(FILE*)> jpegFile(
        _wfopen(jpegPath.wstring().c_str(), L"wb"), fclose);
    if (!jpegFile)
        return;

    jpeg_error_mgr errHandler;
    jpeg_compress_struct comp;
    comp.err = jpeg_std_error(&errHandler);
    errHandler.error_exit = MyErrorExit;
    try {
        jpeg_create_compress(&comp);
        unique_ptr<jpeg_compress_struct, void (*)(jpeg_compress_struct*)>
            autoDestroy(&comp, jpeg_destroy_compress);
        jpeg_stdio_dest(&comp, jpegFile.get());

        comp.image_width = frame->width;
        comp.image_height = frame->height;
        comp.input_components = 3;
        comp.in_color_space = JCS_RGB;
        jpeg_set_defaults(&comp);
        jpeg_set_quality(&comp, 80, true);
        jpeg_start_compress(&comp, true);
        while (comp.next_scanline < comp.image_height) {
            JSAMPROW data[1];
            data[0] = reinterpret_cast<JSAMPROW>(
                frame->data[0] + comp.next_scanline * frame->linesize[0]);
            jpeg_write_scanlines(&comp, data, 1);
        }

        jpeg_finish_compress(&comp);
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Error in loading JPEG file.", MB_OK);
    }
}

void* Jpeg::LoadFromJPEGFile(const path& jpegPath)
{
    unique_ptr<FILE, int (*)(FILE*)> jpegFile(
        _wfopen(jpegPath.wstring().c_str(), L"rb"), fclose);
    if (!jpegFile)
        return NULL;

    jpeg_error_mgr errHandler;
    jpeg_decompress_struct decomp;
    decomp.err = jpeg_std_error(&errHandler);
    errHandler.error_exit = MyErrorExit;
    try {
        jpeg_create_decompress(&decomp);
        unique_ptr<jpeg_decompress_struct, void (*)(jpeg_decompress_struct*)>
            autoDestroy(&decomp, jpeg_destroy_decompress);

        jpeg_stdio_src(&decomp, jpegFile.get());
        jpeg_read_header(&decomp, true);

        const int lineSize = (decomp.image_width * 3 + 3) / 4 * 4;
        const int dataSize = lineSize * decomp.image_height;
        int8* outBuf = new int8[sizeof(BITMAPINFOHEADER) + dataSize];

        BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(outBuf);
        memset(header, 0, sizeof(*header));
        header->biSize = sizeof(*header);
        header->biWidth = decomp.image_width;
        header->biHeight = -static_cast<int>(decomp.image_height);
        header->biPlanes = 1;
        header->biBitCount = 24;
        header->biCompression = BI_RGB;

        int8* dataStart = reinterpret_cast<int8*>(++header);
        jpeg_start_decompress(&decomp);
        while (decomp.output_scanline < decomp.output_height) {
            JSAMPROW data[1];
            data[0] = reinterpret_cast<JSAMPROW>(
                dataStart + decomp.output_scanline * lineSize);
            jpeg_read_scanlines(&decomp, data, 1);
        }

        jpeg_finish_decompress(&decomp);
        return outBuf;
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Error in loading JPEG file.", MB_OK);
    }

    return NULL;
}
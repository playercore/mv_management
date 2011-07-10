#include "png_tool.h"

#include <cassert>
#include <memory>
#include <functional>

#include <windows.h>

#include "basictypes.h"
#include "package/libpng/include/png.h"
#include "util.h"

using std::unique_ptr;
using std::function;
using std::bind;
using std::placeholders::_1;

void* Png::LoadFromFile(const wchar_t* pngPath)
{
    unique_ptr<FILE, int (*)(FILE*)> pngFile(_wfopen(pngPath, L"rb"), fclose);
    if (!pngFile)
        return NULL;

    png_structp png =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
       return NULL;

    png_infop pngInfo = png_create_info_struct(png);
    if (!pngInfo) {
        png_destroy_read_struct(&png, NULL, NULL);
        return NULL;
    }

    function<void (png_structpp)> releaseFunc =
            bind(png_destroy_read_struct, _1, &pngInfo,
                 reinterpret_cast<png_infopp>(NULL));
    SmartInvokation<png_structpp, function<void (png_structpp)>> autoRelease(
        &png, releaseFunc);

    png_init_io(png, pngFile.get());
    png_read_info(png, pngInfo);
    png_set_bgr(png);

    png_bytepp data = png_get_rows(png, pngInfo);
    const int width = png_get_image_width(png, pngInfo);
    const int height = png_get_image_height(png, pngInfo);
    const int colorType = png_get_color_type(png, pngInfo);
    const int rowSize = png_get_rowbytes(png, pngInfo);

    // Create a 24-bit RGB DIB Section buffer.
    const int lineSize = (width * 3 + 3) / 4 * 4;
    const int dataSize = lineSize * height;
    int8* outBuf = new int8[sizeof(BITMAPINFOHEADER) + dataSize];

    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(outBuf);
    memset(header, 0, sizeof(*header));
    header->biSize = sizeof(*header);
    header->biWidth = width;
    header->biHeight = -height;
    header->biPlanes = 1;
    header->biBitCount = 24;
    header->biCompression = BI_RGB;

    int8* dataStart = reinterpret_cast<int8*>(++header);
    for (int y = 0; y < height; ++y)
        png_read_row(png, reinterpret_cast<png_bytep>(dataStart + y * lineSize),
                     NULL);

    return outBuf;
}

namespace {
class MyPngMemReader
{
public:
    MyPngMemReader(const void* buf, int size) : buf_(buf), size_(size) {}
    ~MyPngMemReader() {}

    static void Read(png_structp png, png_bytep data, png_size_t size)
    {
        MyPngMemReader* instance =
            reinterpret_cast<MyPngMemReader*>(png_get_io_ptr(png));

        if (instance) {
            if (size <= instance->size_) {
                memcpy(data, instance->buf_, size);
                instance->buf_ =
                    reinterpret_cast<const int8*>(instance->buf_) + size;
                instance->size_ -= size;
                return;
            }
        }

        png_error(png, "Read error.");
    }

private:
    const void* buf_;
    png_size_t size_;
};
}

void* Png::LoadFromMemory(const void* buf, int size)
{
    assert(buf);
    png_structp png =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
       return NULL;

    png_infop pngInfo = png_create_info_struct(png);
    if (!pngInfo) {
        png_destroy_read_struct(&png, NULL, NULL);
        return NULL;
    }

    function<void (png_structpp)> releaseFunc =
            bind(png_destroy_read_struct, _1, &pngInfo,
                 reinterpret_cast<png_infopp>(NULL));
    SmartInvokation<png_structpp, function<void (png_structpp)>> autoRelease(
        &png, releaseFunc);

    MyPngMemReader reader(buf, size);
    png_set_read_fn(png, &reader, &MyPngMemReader::Read);
    png_read_info(png, pngInfo);
    png_set_bgr(png);

    png_bytepp data = png_get_rows(png, pngInfo);
    const int width = png_get_image_width(png, pngInfo);
    const int height = png_get_image_height(png, pngInfo);
    const int colorType = png_get_color_type(png, pngInfo);
    const int rowSize = png_get_rowbytes(png, pngInfo);

    // Create a 24-bit RGB DIB Section buffer.
    const int lineSize = (width * 3 + 3) / 4 * 4;
    const int dataSize = lineSize * height;
    int8* outBuf = new int8[sizeof(BITMAPINFOHEADER) + dataSize];

    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(outBuf);
    memset(header, 0, sizeof(*header));
    header->biSize = sizeof(*header);
    header->biWidth = width;
    header->biHeight = -height;
    header->biPlanes = 1;
    header->biBitCount = 24;
    header->biCompression = BI_RGB;

    int8* dataStart = reinterpret_cast<int8*>(++header);
    for (int y = 0; y < height; ++y)
        png_read_row(png, reinterpret_cast<png_bytep>(dataStart + y * lineSize),
                     NULL);

    return outBuf;
}
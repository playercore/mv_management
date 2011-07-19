#include "util.h"

#include <memory>

#include "mfc_predefine.h"
#include <afx.h>

#include "basictypes.h"
#include "png_tool.h"

using std::string;
using std::wstring;
using std::unique_ptr;

string WideCharToMultiByte(const wstring& from)
{
    int size = ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, NULL, 0, NULL,
                                     NULL);
    if (1 > size)
        return string();

    unique_ptr<char[]> dst(new char[size]);
    ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, dst.get(), size, NULL,
                          NULL);
    return string(dst.get());
}

wstring MultiByteToWideChar(const string& from)
{
    setlocale(LC_ALL, "chs");
    wstring result;
    do
    {
        const int bufSize = mbstowcs(NULL, from.c_str(), 0) + 1;
        if (bufSize < 2)
            break;

        unique_ptr<wchar_t[]> buf(new wchar_t[bufSize]);
        if (mbstowcs(buf.get(), from.c_str(), bufSize) < 2)
            break;

        buf[bufSize - 1] = '\0';
        result = buf.get();
    } while (0);

    setlocale(LC_ALL, "C");
    return result;
}

wchar_t* GetMvPreviewPath()
{
    return L"\\\\192.168.0.200\\mv_preview\\";
}

void LoadNotificationImage(int id, CBitmap* target, CBitmap* mask)
{
    assert(!target->GetSafeHandle());
    assert(!mask->GetSafeHandle());
    HRSRC res = FindResource(NULL, MAKEINTRESOURCE(id), L"PNG");
    if (!res)
        return;

    HGLOBAL handle = LoadResource(NULL, res);
    unique_ptr<void, void (__stdcall *)(void*)> resData(
        LockResource(handle),
        reinterpret_cast<void (__stdcall *)(void*)>(UnlockResource));
    void* decoded =
        Png::LoadFromMemory(resData.get(), SizeofResource(NULL, res));
    if (!decoded)
        return;

    unique_ptr<int8> autoRelease(reinterpret_cast<int8*>(decoded));
    BITMAPINFOHEADER* header = reinterpret_cast<BITMAPINFOHEADER*>(decoded);
    void* bits = NULL;
    HBITMAP h = CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(header),
                                 DIB_RGB_COLORS, &bits, NULL, 0);
    if (!h || !bits)
        return;

    int width = header->biWidth;
    int height = abs(header->biHeight);
    memcpy(bits,
           header + 1, (width * header->biBitCount + 31) / 32 * 4 * height);
    target->Attach(h);

    // Prepare mask.
    BITMAP targetInfo;
    if (sizeof(targetInfo) != target->GetBitmap(&targetInfo))
        return;

    const int targetWidth = targetInfo.bmWidth;
    const int targetHeight = targetInfo.bmHeight;

    mask->CreateBitmap(targetWidth, targetHeight, 1, 1, NULL);

    CDC* dc = CDC::FromHandle(GetDC(NULL));
    if (!dc)
        return;

    CDC targetDc;
    targetDc.CreateCompatibleDC(dc);

    CDC maskDc;
    maskDc.CreateCompatibleDC(dc);

    CGdiObject* o1 = targetDc.SelectObject(target);
    CGdiObject* o2 = maskDc.SelectObject(mask);

    targetDc.SetBkColor(RGB(255, 0, 255));
    maskDc.BitBlt(0, 0, targetWidth, targetHeight, &targetDc, 0, 0, SRCCOPY);

    targetDc.SetTextColor(RGB(255, 255, 255));
    targetDc.SetBkColor(0);
    targetDc.BitBlt(0, 0, targetWidth, targetHeight, &maskDc, 0, 0, SRCAND);

    maskDc.SelectObject(o2);
    targetDc.SelectObject(o1);
}
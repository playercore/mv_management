#include "image_cache.h"

#include <string>

#include <boost/filesystem.hpp>
#include <boost/intrusive_ptr.hpp>

#include "task_canceler.h"
#include "jpeg_tool.h"
#include "intrusive_ptr_helper.h"

using std::function;
using std::wstring;
using boost::filesystem3::path;
using boost::intrusive_ptr;

namespace {
void DowngradePriority(PlatformThreadHandle threadHandle)
{
    SetThreadPriority(threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
}

void LoadJPEGImpl(wstring jpegPath, intrusive_ptr<CTaskCanceler> canceler,
                  function<void (void*)> callback)
{
    if (canceler->IsCanceled())
        return;

    callback(Jpeg::LoadFromJPEGFile(jpegPath));
}
}

ImageCache::ImageCache()
    : thread_("image caching")
    , loadingDoneSignal_()
{
    thread_.Start();
    thread_.message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(&DowngradePriority, thread_.thread_handle()));
}

ImageCache::~ImageCache()
{
}

void ImageCache::LoadJPEG(const wchar_t* imagePath, CTaskCanceler* canceler,
                          const function<void (void*)>& callback)
{
    thread_.message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(&LoadJPEGImpl, wstring(imagePath),
                            intrusive_ptr<CTaskCanceler>(canceler), callback));
}
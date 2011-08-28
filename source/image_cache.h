#ifndef _IMAGE_CACHE_H_
#define _IMAGE_CACHE_H_

#include <functional>
#include <string>

#include "third_party/chromium/base/singleton.h"
#include "third_party/chromium/base/thread.h"

class CTaskCanceler;
class ImageCache : public Singleton<ImageCache>
{
public:
    ImageCache();
    ~ImageCache();

    void LoadJPEG(const std::wstring& imagePath, CTaskCanceler* canceler,
                  const std::function<void (void*)>& callback);

private:
    base::Thread thread_;
};

#endif  // _IMAGE_CACHE_H_
#ifndef _IMAGE_CACHE_H_
#define _IMAGE_CACHE_H_

#include <functional>
#include <string>

#include <boost/signals2.hpp>

#include "third_party/chromium/base/singleton.h"
#include "third_party/chromium/base/thread.h"

class CTaskCanceler;
class ImageCache : public Singleton<ImageCache>
{
public:
    typedef boost::signals2::signal<void (int)> LoadingDoneSignal;
    typedef LoadingDoneSignal::slot_type LoadingDoneSlot;

    ImageCache();
    ~ImageCache();

    void LoadJPEG(const std::wstring& imagePath, CTaskCanceler* canceler,
                  const std::function<void (void*)>& callback);

private:
    base::Thread thread_;
    LoadingDoneSignal loadingDoneSignal_;
};

#endif  // _IMAGE_CACHE_H_
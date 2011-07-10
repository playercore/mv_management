#include "preview_upload.h"

#include <sstream>
#include <functional>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

#include "third_party/chromium/base/singleton.h"
#include "third_party/chromium/base/thread.h"
#include "package/libcurl/include/include/curl/curl.h"
extern "C" {
#include "package/libcurl/include/lib/curl_md5.h"
};
#include "util.h"

using std::unique_ptr;
using std::shared_ptr;
using std::string;
using std::wstring;
using std::stringstream;
using std::function;
using std::bind;
using std::placeholders::_1;
using boost::gregorian::date;
using boost::gregorian::day_clock;
using boost::gregorian::to_iso_string;
using boost::filesystem3::path;
using boost::filesystem3::file_size;

namespace {
void Report(UploadCallback* c, int id, int* r)
{
    if (c && r)
        c->Done(id, *r);
}

class AsyncUpload : public Singleton<AsyncUpload>
{
public:
    AsyncUpload()
        : thread_("upload thread")
    {
        thread_.Start();
    }
    ~AsyncUpload() {}

    void Upload(const path& previewPath, shared_ptr<UploadCallback> callback,
                int identifier)
    {
        thread_.message_loop()->PostTask(
            FROM_HERE,
            NewRunnableFunction(&AsyncUpload::UploadImpl, previewPath, callback,
                                identifier));
    }

private:
    static void UploadImpl(const path previewPath,
                           shared_ptr<UploadCallback> callback, int identifier)
    {
        int result = -1;
        function<void (UploadCallback*)> reportFunc =
            bind(Report, _1, identifier, &result);
        SmartInvokation<UploadCallback*, function<void (UploadCallback*)>>
            autoReport(callback.get(), reportFunc);

        unique_ptr<CURL, void(*)(CURL*)> curl(curl_easy_init(),
                                              curl_easy_cleanup);
        if (!curl)
            return;

        date today = day_clock::local_day();
        string md5Raw = to_iso_string(today) + "hewry678WEK23D";

        unsigned char md5Buf[16];
        Curl_md5it(md5Buf,
                   reinterpret_cast<const unsigned char*>(md5Raw.c_str()));
        string md5;
        for (int i = 0; i < 16; ++i) {
            stringstream s;
            s.setf(std::ios::hex, std::ios::basefield);
            s.fill('0');
            s.width(2);
            s << static_cast<int>(md5Buf[i]);
            md5 += s.str();
        }

        const int fileSize = static_cast<int>(file_size(previewPath));
        unique_ptr<FILE, int (*)(FILE*)> previewFile(
            _wfopen(previewPath.wstring().c_str(), L"rb"), fclose);
        if (!previewFile)
            return;

        stringstream url;
        url << "http://image4.kugou.com/imageupload/stream.php?"
            "type=mvpic&extendName=.jpg&fileName=" <<
            WideCharToMultiByte(previewPath.filename().wstring()).c_str() <<
            "&md5=" << md5;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.str().c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
        curl_easy_setopt(curl.get(), CURLOPT_READDATA, previewFile.get());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, fileSize);
        CURLcode r = curl_easy_perform(curl.get());
        if (CURLE_OK == r)
            result = 0;
    }

    base::Thread thread_;
};
}

//------------------------------------------------------------------------------
void PreviewUpload::Upload(const path& previewPath,
                           shared_ptr<UploadCallback> callback, int identifier)
{
    AsyncUpload::get()->Upload(previewPath, callback, identifier);
}

PreviewUpload::PreviewUpload()
{
}

PreviewUpload::~PreviewUpload()
{
}
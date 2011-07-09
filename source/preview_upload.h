#ifndef _PREVIEW_UPLOAD_H_
#define _PREVIEW_UPLOAD_H_

#include <memory>

namespace boost
{
namespace filesystem3
{
class path;
}
}

class UploadCallback
{
public:
    virtual void Done(int identifier, int result) = 0;
};

//------------------------------------------------------------------------------
class PreviewUpload
{
public:
    static void Upload(const boost::filesystem3::path& previewPath,
                       std::shared_ptr<UploadCallback> callback,
                       int identifier);

private:
    PreviewUpload();
    ~PreviewUpload();
};

#endif  // _PREVIEW_UPLOAD_H_
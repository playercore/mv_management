#ifndef _COMMON_H_
#define _COMMON_H_

#include <memory>
#include <string>

//#include "third_party/chromium/base/ref_counted.h"
#include "basictypes.h"

enum { NUM_COLUMNS_EXCLUDED = 9 };
enum { COLUMN_WIDTH = 80 };

std::wstring GetLocalIP();

class BufferBase// : public base::RefCountedThreadSafe<BufferBase>
{
public:
    explicit BufferBase(int size) : impl_(new int8[size]) {}
    ~BufferBase() {}

    void* GetPointer() { return impl_.get(); }

private:
    std::unique_ptr<int8[]> impl_;
};

#endif
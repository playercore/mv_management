#ifndef _TASK_CANCELZER_H_
#define _TASK_CANCELZER_H_

#include "third_party/chromium/base/ref_counted.h"

#ifndef NDEBUG
class CPartlyThreadSafe
{
public:
    CPartlyThreadSafe() : m_validThreadId(PlatformThread::CurrentId()) {}
    ~CPartlyThreadSafe() {}

    bool CalledOnValidThread() const
    {
        return m_validThreadId == PlatformThread::CurrentId();
    }

private:
    PlatformThreadId m_validThreadId;
};
#else
class CPartlyThreadSafe
{
public:
    CPartlyThreadSafe() {}
    ~CPartlyThreadSafe() {}

    inline bool CalledOnValidThread() const { return true; }
};
#endif

//------------------------------------------------------------------------------
class CTaskCanceler : public base::RefCountedThreadSafe<CTaskCanceler>,
                      public CPartlyThreadSafe
{
public:
    CTaskCanceler() : m_canceled(false) {}
    ~CTaskCanceler() {}

    void Cancel() { m_canceled = true; }
    bool IsCanceled() const { return m_canceled; }

private:
    bool m_canceled;
};

#endif  // _TASK_CANCELZER_H_
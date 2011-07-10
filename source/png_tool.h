#ifndef _PNG_TOOL_H_
#define _PNG_TOOL_H_

class Png
{
public:
    static void* LoadFromFile(const wchar_t* pngPath);
    static void* LoadFromMemory(const void* buf, int size);

private:
    Png();
    ~Png();
};

#endif  // _PNG_TOOL_H_
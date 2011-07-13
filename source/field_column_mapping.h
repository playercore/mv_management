#ifndef _FIELD_COLUMN_MAPPING_H_
#define _FIELD_COLUMN_MAPPING_H_

#include "third_party/chromium/base/singleton.h"

class FieldColumnMapping : public Singleton<FieldColumnMapping>
{
public:
    enum FieldName
    {
        kSongFullListSongId,
        kSongFullListFilePath,
        kSongFullListMd5,
        kSongFullListNumOfTracks,
        kRightViewFilePath,
    };

    FieldColumnMapping() {}
    ~FieldColumnMapping() {}

    int GetColumnIndex(FieldName fieldName);
};

#endif  // _FIELD_COLUMN_MAPPING_H_
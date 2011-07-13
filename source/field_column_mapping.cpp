#include "field_column_mapping.h"

namespace {
const int mapping[] = { 0, 1, 2, 3, 10, 6, 8, 7, 6, 4, 11, 5, 9 };
}

int FieldColumnMapping::GetColumnIndex(FieldName fieldName)
{
    const int index = fieldName;
    if (index >= arraysize(mapping))
        return -1;

    return mapping[index];
}
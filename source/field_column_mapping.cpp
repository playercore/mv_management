#include "field_column_mapping.h"

namespace {
const int mapping[] = { 0, 1, 2, 10, 6 };
}

int FieldColumnMapping::GetColumnIndex(FieldName fieldName)
{
    const int index = fieldName;
    if (index >= arraysize(mapping))
        return -1;

    return mapping[index];
}
#ifndef _LIST_ITEM_DEFINE_H_
#define _LIST_ITEM_DEFINE_H_

#define UPDATESELITEM WM_USER + 1
#define LEFTLISTITEM WM_USER + 2
#define RIGHTLISTITEM WM_USER + 3

struct TListItem
{
    CString id;
    CString OldHash;
    CString Name;
    CString IsInterlace;
    CString Notes;
    CString TrackType;
    CString MVType;
    CString NewHash;
    CString Quality;
};

struct TLeftListItem
{
    CString Name;
    CString OldHash;
};
#endif // _LIST_ITEM_DEFINE_H_
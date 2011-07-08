#pragma once

#include "mfc_predefine.h"
#include "resource/resource.h"		// 主符号

// Cmv_managementApp:
// 有关此类的实现，请参阅 mv_management.cpp
//
#include "mfc_predefine.h"
#include "resource/resource.h" // main symbols

class CMVManagementApp : public CWinAppEx
{
public:
	CMVManagementApp();
	virtual BOOL InitInstance();
    virtual int ExitInstance();
// 实现

	DECLARE_MESSAGE_MAP()

};

extern CMVManagementApp theApp;

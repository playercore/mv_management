#pragma once

#include "mfc_predefine.h"
#include "resource/resource.h"		// ������

// Cmv_managementApp:
// �йش����ʵ�֣������ mv_management.cpp
//
#include "mfc_predefine.h"
#include "resource/resource.h" // main symbols

class CMVManagementApp : public CWinAppEx
{
public:
	CMVManagementApp();
	virtual BOOL InitInstance();
    virtual int ExitInstance();
// ʵ��

	DECLARE_MESSAGE_MAP()

};

extern CMVManagementApp theApp;

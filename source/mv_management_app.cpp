#include "mv_management_app.h"

#include <cassert>

#include "third_party/chromium/base/at_exit.h"
#include "third_party/chromium/base/time.h"
#include "mv_management_dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
base::AtExitManager* atExit = NULL;
}

// CMVManagementApp
BEGIN_MESSAGE_MAP(CMVManagementApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CMVManagementApp::CMVManagementApp()
{
    // ֧����������������
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: �ڴ˴���ӹ�����룬
    // ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}

CMVManagementApp theApp;
BOOL CMVManagementApp::InitInstance()
{
    assert(!atExit);
    atExit = new base::AtExitManager;

    // Make sure this singleton is the last one to be destroyed.
    base::TimeTicks::Now();
    
    CoInitialize(NULL);
    // ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
    // ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
    //����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
    // �����ؼ��ࡣ
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinAppEx::InitInstance();

    if (!AfxSocketInit())
    {
        AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
        return FALSE;
    }

    if (!AfxOleInit())
        MessageBox(NULL, L"OLE��ʼ��ʧ��", L"", MB_OK);

    AfxEnableControlContainer();
// 
//     // ���� shell ���������Է��Ի������
//     // �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
//     CShellManager *pShellManager = new CShellManager;
// 
//     // ��׼��ʼ��
//     // ���δʹ����Щ���ܲ�ϣ����С
//     // ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
//     // ����Ҫ���ض���ʼ������
//     // �������ڴ洢���õ�ע�����
//     // TODO: Ӧ�ʵ��޸ĸ��ַ�����
//     // �����޸�Ϊ��˾����֯��
//     SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

    CMVManagementDialog dialog;
    m_pMainWnd = &dialog;
    INT_PTR nResponse = dialog.DoModal();
    if (nResponse == IDOK)
    {
        // TODO: �ڴ˷��ô����ʱ��
        //  ��ȷ�������رնԻ���Ĵ���
    }
    else if (nResponse == IDCANCEL)
    {
        // TODO: �ڴ˷��ô����ʱ��
        //  ��ȡ�������رնԻ���Ĵ���
    }

//     // ɾ�����洴���� shell ��������
//     if (pShellManager != NULL)
//     {
//         delete pShellManager;
//     }

    // ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
    //  ����������Ӧ�ó������Ϣ�á�
    return FALSE;
}

int CMVManagementApp::ExitInstance()
{
    int result = CWinAppEx::ExitInstance();

    CoUninitialize();

    assert(atExit);
    if (atExit) {
        delete atExit;
        atExit = NULL;
    }

    return result;
}
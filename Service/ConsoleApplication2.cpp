#include <WINDOWS.H>
#include <stdio.h>
#include <tchar.h>
#include <Shlwapi.h>
//#include "process_of_communication.h"
#include "process_of_communication.h"
#pragma comment(lib,"shlwapi.lib") 

//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

DWORD WINAPI Event_ThreadProc2(LPVOID pVoid)
{

	return 0;
}
#if CONSOLE
int main(int argc, _TCHAR* argv[])
{
#else
int WINAPI WinMain(__in HINSTANCE hInstance,
	__in_opt HINSTANCE hPrevInstance,
	__in LPSTR lpCmdLine,
	__in int nShowCmd)
{

#endif
	CProcessOfCommunication a(L"fanhuangzhidun", CProcessOfCommunication::PORT_SERVICE);
	if (a.HandscakeWait())//双方握手成功之后才可以通讯
	{
		if (!PathFileExists(L"c:/temp"))//如果不存在
		{
			CreateDirectoryW(L"c:/temp",NULL);
		}
		while (true)
		{
			a.ServiceDispose();
		}
	}

	getchar();
}
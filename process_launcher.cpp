#include <cstdio>
#include <Windows.h>
#include <atlbase.h>
#include <TlHelp32.h>
#include <iostream>
using namespace std;

BOOL EnablePrivilege( LPCTSTR name )
{
	// Elevate process privilege
	BOOL bRet = FALSE;
	// Get privilege
	TOKEN_PRIVILEGES priv = { 1, { 0, 0, SE_PRIVILEGE_ENABLED } };
	BOOL bLookup = LookupPrivilegeValue( NULL, name, &priv.Privileges[0].Luid );
	if ( !bLookup )
		return bRet;

	// Open process token
	HANDLE hToken = NULL;
	BOOL bOpenToken = OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken );
	if ( !bOpenToken )
		return bRet;

	// Adjust process privilege
	BOOL bAdjustPriv = AdjustTokenPrivileges( hToken, false, &priv, sizeof( priv ), 0, 0 );
	if ( !bAdjustPriv )
	{
		CloseHandle( hToken );
		return bRet;
	}

	CloseHandle( hToken);
	return TRUE;
}

// Find process id by name 
DWORD FindProcessIDByProcessName(LPCTSTR lpszProcessName)
{
	int nStrLen = lstrlen(lpszProcessName);
	if ( 0 == nStrLen )
		return 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( NULL == hSnapshot )
		return 0;

	PROCESSENTRY32 stProcEntry32 = { 0 };
	stProcEntry32.dwSize = sizeof( PROCESSENTRY32 );
	Process32First( hSnapshot, &stProcEntry32 );
	BOOL bFind = FALSE;
	do 
	{
		if ( lstrcmp( stProcEntry32.szExeFile, lpszProcessName ) == 0 )
		{
			bFind = TRUE;
			break;
		}
	} while ( Process32Next( hSnapshot, &stProcEntry32 ) );

	CloseHandle( hSnapshot );
	if ( bFind )
		return stProcEntry32.th32ProcessID;

	return 0;
}

int main(int argc, char **argv)
{
	if ( argc != 3)
	{
		cout << _T("usage: trigger.exe [parent process name] [process name]") << endl;
		return 0;
	}

	EnablePrivilege( SE_SECURITY_NAME );
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOEX si = { sizeof( STARTUPINFOEX ) };

	SIZE_T cbAttrListSize = 0;
	InitializeProcThreadAttributeList( NULL, 1, 0, &cbAttrListSize );
	PPROC_THREAD_ATTRIBUTE_LIST pAttrList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc( GetProcessHeap(), 0, cbAttrListSize );
	InitializeProcThreadAttributeList( pAttrList, 1, 0, &cbAttrListSize );

	DWORD dwPid = FindProcessIDByProcessName( argv[1] );
	if ( 0 == dwPid)
	{
		goto END;
	}
	HANDLE hParent = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwPid );
	UpdateProcThreadAttribute( pAttrList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hParent, sizeof( HANDLE ), NULL, NULL );
	si.lpAttributeList = pAttrList;
	
	LPSTR lpProcName = argv[2];
	CreateProcess( NULL, lpProcName, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &si.StartupInfo, &pi );

END:
	DeleteProcThreadAttributeList( pAttrList );
	HeapFree( GetProcessHeap(), 0, pAttrList );
	return 0;
}
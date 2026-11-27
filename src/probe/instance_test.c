/* Explicit second-client diagnostic. Called only after observer SHA256 and
   isolated-profile installation, before the owned child's EXE entry. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef HANDLE (WINAPI *CreateMutexType)(LPSECURITY_ATTRIBUTES,BOOL,LPCSTR);
static CreateMutexType original_create;
static char private_name[96];
static wchar_t evidence[MAX_PATH];
static void record(const char *line) {
    DWORD error=GetLastError(),written;
    HANDLE f=CreateFileW(evidence,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(f!=INVALID_HANDLE_VALUE){WriteFile(f,line,(DWORD)strlen(line),&written,NULL);CloseHandle(f);}
    SetLastError(error);
}
static HANDLE create_for_caller(LPSECURITY_ATTRIBUTES security,BOOL owner,LPCSTR name,uintptr_t caller) {
    /* Only the pinned WinMain guard's exact literal and return address.
       The installer mutex and all synchronization primitives pass unchanged. */
    if(name==(LPCSTR)(uintptr_t)0x18edc00 &&
       caller==0x401baa) {
        HANDLE result=original_create(security,owner,private_name);
        record("instance_test=launcher_mutex_isolated\r\n");return result;
    }
    return original_create(security,owner,name);
}
static HANDLE WINAPI instance_mutex(LPSECURITY_ATTRIBUTES security,BOOL owner,LPCSTR name) {
    return create_for_caller(security,owner,name,(uintptr_t)__builtin_return_address(0));
}
DWORD CoopInstanceTestInstall(const wchar_t *log) {
    if(!log || wcslen(log)>=MAX_PATH)return 30;
    wchar_t ini[MAX_PATH];wcscpy(ini,log);wchar_t *tail=wcsrchr(ini,L'\\');if(!tail)return 30;
    *tail=0;if(wcslen(ini)+20>=MAX_PATH)return 30;wcscat(ini,L"\\instance-test.ini");
    if(GetPrivateProfileIntW(L"test",L"second_visual_client",0,ini)!=1)return 0;
    const unsigned char call[]={0xff,0x15,0x58,0x21,0x8e,0x01,0xff,0x15,0x54,0x21,0x8e,0x01,0x3d,0xb7,0x00,0x00,0x00};
    if(memcmp((void *)(uintptr_t)0x401ba4,call,sizeof(call)) ||
       strcmp((char *)(uintptr_t)0x18edc00,"Mafia2 Launcher Super Mutex"))return 31;
    FILETIME c,e,k,u;if(!GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u))return 32;
    snprintf(private_name,sizeof(private_name),"MafiaII-Coop-VisualTest-%lu-%08lx%08lx",
        GetCurrentProcessId(),c.dwHighDateTime,c.dwLowDateTime);
    wcscpy(evidence,log);
    void **slot=(void **)(uintptr_t)0x18e2158;
    void *expected=(void *)(uintptr_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"CreateMutexA");
    if(!expected || *slot!=expected)return 33;
    original_create=(CreateMutexType)expected;DWORD old,ignored;
    if(!VirtualProtect(slot,sizeof(*slot),PAGE_READWRITE,&old))return 34;
    void *prior=InterlockedCompareExchangePointer(slot,(void *)instance_mutex,expected);
    VirtualProtect(slot,sizeof(*slot),old,&ignored);
    if(prior!=expected)return 35;
    record("instance_test=guarded_import_installed\r\n");return 0;
}

/* Explicit local two-client diagnostic; never enabled by a normal launch.
   Pinned WndProc: skip only WM_ACTIVATE / WM_ACTIVATEAPP engine handlers.
   DefWindowProc still receives the original message and parameters. Native
   Escape pause, lifecycle events and network pause guards remain intact. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
DWORD CoopBackgroundTestInstall(const wchar_t *log) {
    wchar_t ini[MAX_PATH];
    if(!log || wcslen(log)+24>=MAX_PATH)return 40;
    wcscpy(ini,log);wchar_t *tail=wcsrchr(ini,L'\\');if(!tail)return 40;
    wcscpy(tail,L"\\background-test.ini");
    if(GetPrivateProfileIntW(L"test",L"background_visual_client",0,ini)!=1)return 0;
    unsigned char *a=(unsigned char *)(uintptr_t)0xac6d22;
    unsigned char *b=(unsigned char *)(uintptr_t)0xac6e57;
    const unsigned char expected_a[]={0x8b,0x44,0x24,0x68,0x83,0xe8,0x00};
    const unsigned char expected_b[]={0x83,0x7c,0x24,0x68,0x00};
    const unsigned char epilogue[]={0x8b,0x44,0x24,0x6c,0x8b,0x4c,0x24,0x68};
    if(memcmp(a,expected_a,sizeof(expected_a)) || memcmp(b,expected_b,sizeof(expected_b)) ||
       memcmp((void *)(uintptr_t)0xac6f6d,epilogue,sizeof(epilogue)))return 41;
    DWORD old,ignored;
    /* Both sites share one page. Installation precedes the game's entry. */
    if(!VirtualProtect(a,0x200,PAGE_EXECUTE_READWRITE,&old))return 42;
    unsigned char jump[5]={0xe9};int32_t relative=(int32_t)(0xac6f6d-0xac6d22-5);
    memcpy(jump+1,&relative,4);memcpy(a,jump,5);
    relative=(int32_t)(0xac6f6d-0xac6e57-5);
    memcpy(jump+1,&relative,4);memcpy(b,jump,5);
    BOOL restored=VirtualProtect(a,0x200,old,&ignored);
    BOOL flushed=FlushInstructionCache(GetCurrentProcess(),a,0x200);
    if(!restored || !flushed)return 43;
    HANDLE file=CreateFileW(log,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE)return 44;
    const char line[]="background_test=focus_handlers_bypassed\r\n";DWORD written;
    BOOL ok=WriteFile(file,line,sizeof(line)-1,&written,NULL);CloseHandle(file);
    return ok && written==sizeof(line)-1 ? 0 : 45;
}

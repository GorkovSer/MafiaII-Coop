/* GUI-subsystem entry point. Launch only the bundled UI, with no console.
   No game launch, elevation, user input interpolation or process termination. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

static int append(wchar_t *path,const wchar_t *suffix) {
    if(wcslen(path)+wcslen(suffix)>=MAX_PATH)return 0;
    wcscat(path,suffix);return 1;
}
static int problem(const wchar_t *message) {
    MessageBoxW(NULL,message,L"Mafia II Coop",MB_OK|MB_ICONERROR);return 1;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR arguments,int show) {
    (void)instance;(void)previous;(void)arguments;(void)show;
    wchar_t root[MAX_PATH],script[MAX_PATH],powershell[MAX_PATH],modules[MAX_PATH];
    DWORD length=GetModuleFileNameW(NULL,root,MAX_PATH);
    if(!length || length>=MAX_PATH)return problem(L"Распакуй лаунчер в короткую папку, например C:\\MafiaCoop.");
    wchar_t *tail=wcsrchr(root,L'\\');if(!tail)return 1;tail[1]=0;
    wcscpy(script,root);
    if(!append(script,L"scripts\\coop-launcher.ps1") || GetFileAttributesW(script)==INVALID_FILE_ATTRIBUTES)
        return problem(L"Не найден интерфейс лаунчера. Распакуй весь архив, сохранив папку scripts.");
    if(!GetSystemDirectoryW(powershell,MAX_PATH) ||
       !append(powershell,L"\\WindowsPowerShell\\v1.0\\powershell.exe"))return 1;
    wcscpy(modules,powershell);tail=wcsrchr(modules,L'\\');if(!tail)return 1;*tail=0;
    if(!append(modules,L"\\Modules") || !SetEnvironmentVariableW(L"PSModulePath",modules))
        return problem(L"Не удалось подготовить запуск Windows PowerShell.");
    wchar_t log[MAX_PATH];wcscpy(log,root);
    if(!append(log,L"work"))return 1;
    if(!CreateDirectoryW(log,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)
        return problem(L"Нет доступа к папке лаунчера. Распакуй архив в доступную для записи папку.");
    if(!append(log,L"\\crashlogs"))return 1;
    if(!CreateDirectoryW(log,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)return 1;
    SYSTEMTIME time;GetSystemTime(&time);
    wchar_t name[96];swprintf(name,96,L"\\bootstrap-%04u%02u%02u-%02u%02u%02u-%03u-%lu.txt",
        time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,
        time.wMilliseconds,GetCurrentProcessId());
    if(!append(log,name))return problem(L"Слишком длинный путь к журналам. Используй короткую папку.");
    SECURITY_ATTRIBUTES security={sizeof(security),NULL,TRUE};
    HANDLE output=CreateFileW(log,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE)return problem(L"Не удалось создать журнал запуска в work\\crashlogs.");
    HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,NULL);
    if(input==INVALID_HANDLE_VALUE){CloseHandle(output);return 1;}
    wchar_t command[1024];
    int count=swprintf(command,1024,L"\"%ls\" -NoLogo -NoProfile -NonInteractive -STA -ExecutionPolicy Bypass -File \"%ls\"",powershell,script);
    if(count<0 || count>=1024){CloseHandle(input);CloseHandle(output);return 1;}
    STARTUPINFOW startup={0};startup.cb=sizeof(startup);
    startup.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow=SW_SHOWNORMAL; /* Do not hide the first WinForms window. */
    startup.hStdInput=input;startup.hStdOutput=startup.hStdError=output;
    PROCESS_INFORMATION child={0};
    BOOL ok=CreateProcessW(powershell,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,root,&startup,&child);
    if(!ok){DWORD error=GetLastError(),written;char line[80];
        int n=wsprintfA(line,"CreateProcess Windows error: %lu\r\n",error);
        WriteFile(output,line,(DWORD)n,&written,NULL);
    }
    CloseHandle(input);CloseHandle(output);
    if(!ok)return problem(L"Не удалось открыть лаунчер. Подробности в work\\crashlogs.");
    CloseHandle(child.hThread);CloseHandle(child.hProcess);return 0;
}

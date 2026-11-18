/* Optional UI diagnostic, not a game client. No injection or global hooks.
   Records only Enter/Up/Down while this owned foreground window is active. */
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#define COBJMACROS
#include <windows.h>
#include <dinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

static HANDLE evidence=INVALID_HANDLE_VALUE;
static IDirectInput8A *input;
static IDirectInputDevice8A *keyboard;
static int buffer_ready;

static void record(const char *format,...) {
    char line[256];va_list args;va_start(args,format);
    int size=vsnprintf(line,sizeof(line),format,args);va_end(args);
    if(size<=0 || (size_t)size>=sizeof(line))return;
    DWORD written;WriteFile(evidence,line,(DWORD)size,&written,NULL);
}
static int watched_vk(WPARAM key) {return key==VK_RETURN || key==VK_UP || key==VK_DOWN;}
static int watched_scan(DWORD scan) {return scan==DIK_RETURN || scan==DIK_UP || scan==DIK_DOWN;}
static LRESULT CALLBACK on_message(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if((message==WM_KEYDOWN || message==WM_KEYUP) && watched_vk(wp) && GetForegroundWindow()==window)
        record("window_key down=%d vk=%u scan=%u extended=%u\r\n",message==WM_KEYDOWN,
               (unsigned)wp,(unsigned)((lp>>16)&255),(unsigned)((lp>>24)&1));
    if(message==WM_TIMER && GetForegroundWindow()==window && keyboard) {
        DIDEVICEOBJECTDATA events[32];DWORD count=32;
        HRESULT result=IDirectInputDevice8_GetDeviceData(keyboard,sizeof(events[0]),events,&count,0);
        if(FAILED(result)) {IDirectInputDevice8_Acquire(keyboard);return 0;}
        if(!buffer_ready){record("directinput_buffer_ready=1\r\n");buffer_ready=1;}
        for(DWORD i=0;i<count;++i)if(watched_scan(events[i].dwOfs))
            record("directinput_key down=%d scan=%lu\r\n",(events[i].dwData&128)!=0,(unsigned long)events[i].dwOfs);
        return 0;
    }
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcA(window,message,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR args,int show) {
    (void)previous;(void)args;(void)show;
    wchar_t path[MAX_PATH];DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    if(!length || length>=MAX_PATH)return 1;
    wchar_t *tail=wcsrchr(path,L'\\');if(!tail)return 1;
    size_t remaining=MAX_PATH-(size_t)(tail+1-path);
    if(swprintf(tail+1,remaining,L"input-check-%lu-%llu.log",(unsigned long)GetCurrentProcessId(),
                (unsigned long long)GetTickCount64())<0)return 1;
    evidence=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(evidence==INVALID_HANDLE_VALUE)return 2;
    WNDCLASSA cls={0};cls.lpfnWndProc=on_message;cls.hInstance=instance;
    cls.lpszClassName="CoopInputCheck";cls.hCursor=LoadCursor(NULL,IDC_ARROW);
    RegisterClassA(&cls);
    HWND window=CreateWindowA(cls.lpszClassName,"MafiaII-Coop buffered keyboard diagnostic",
        WS_OVERLAPPEDWINDOW,100,100,640,260,NULL,NULL,instance,NULL);
    if(!window){CloseHandle(evidence);return 3;}
    HRESULT result=DirectInput8Create(instance,DIRECTINPUT_VERSION,&IID_IDirectInput8A,(void **)&input,NULL);
    if(SUCCEEDED(result))result=IDirectInput8_CreateDevice(input,&GUID_SysKeyboard,&keyboard,NULL);
    if(SUCCEEDED(result))result=IDirectInputDevice8_SetDataFormat(keyboard,&c_dfDIKeyboard);
    if(SUCCEEDED(result))result=IDirectInputDevice8_SetCooperativeLevel(keyboard,window,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
    DIPROPDWORD buffer={{sizeof(buffer),sizeof(buffer.diph),0,DIPH_DEVICE},32};
    if(SUCCEEDED(result))result=IDirectInputDevice8_SetProperty(keyboard,DIPROP_BUFFERSIZE,&buffer.diph);
    record("directinput_setup=%08lx\r\n",(unsigned long)result);
    if(FAILED(result)){DestroyWindow(window);CloseHandle(evidence);return 4;}
    SetTimer(window,1,10,NULL);ShowWindow(window,SW_SHOW);UpdateWindow(window);
    MSG message;while(GetMessage(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessage(&message);}
    IDirectInputDevice8_Release(keyboard);IDirectInput8_Release(input);CloseHandle(evidence);return 0;
}

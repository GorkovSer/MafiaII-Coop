#define WIN32_LEAN_AND_MEAN
#include <windows.h>
__declspec(dllexport) volatile LONG ProbeHostEntrySeen = 0;
int WINAPI WinMain(HINSTANCE a, HINSTANCE b, LPSTR c, int d) {
    (void)a; (void)b; (void)c; (void)d;
    ProbeHostEntrySeen = 1;
    Sleep(20000);
    return 0;
}

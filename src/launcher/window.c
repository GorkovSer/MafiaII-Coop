#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

static HMONITOR second;
static RECT work;
static BOOL CALLBACK find_monitor(HMONITOR monitor, HDC dc, LPRECT bounds, LPARAM param) {
    (void)dc; (void)bounds; (void)param;
    MONITORINFOEXW info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(monitor, (MONITORINFO *)&info) && !_wcsicmp(info.szDevice, L"\\\\.\\DISPLAY2")) {
        second = monitor;
        work = info.rcWork;
    }
    return TRUE;
}

int CoopSecondMonitorAvailable(void) {
    second = NULL;
    EnumDisplayMonitors(NULL, NULL, find_monitor, 0);
    return second != NULL;
}

typedef struct { DWORD pid; HWND window; } search;
static BOOL CALLBACK find_window(HWND window, LPARAM param) {
    search *target = (search *)param;
    DWORD pid;
    GetWindowThreadProcessId(window, &pid);
    if (pid == target->pid && IsWindowVisible(window) && !GetWindow(window, GW_OWNER)) {
        RECT rect;
        GetClientRect(window, &rect);
        if (rect.right > 100 && rect.bottom > 100) { target->window = window; return FALSE; }
    }
    return TRUE;
}

int CoopPlaceOnSecondMonitor(DWORD pid, HANDLE process) {
    if (!CoopSecondMonitorAvailable()) return 0;
    search target = {pid, NULL};
    ULONGLONG deadline = GetTickCount64() + 15000;
    do {
        if (WaitForSingleObject(process, 0) != WAIT_TIMEOUT) return 0;
        EnumWindows(find_window, (LPARAM)&target);
        if (target.window) break;
        Sleep(50);
    } while (GetTickCount64() < deadline);
    if (!target.window) return 0;
    RECT rect;
    if (!GetWindowRect(target.window, &rect)) return 0;
    /* Startup can resize the same window after the splash. Center only once
       its outer bounds have remained stable for one second. */
    ULONGLONG stable_since = GetTickCount64();
    while (GetTickCount64() < deadline && GetTickCount64() - stable_since < 1000) {
        if (WaitForSingleObject(process, 100) != WAIT_TIMEOUT) return 0;
        RECT next;
        if (!GetWindowRect(target.window, &next)) return 0;
        if (!EqualRect(&rect, &next)) { rect = next; stable_since = GetTickCount64(); }
    }
    if (GetTickCount64() - stable_since < 1000) return 0;
    int width = rect.right - rect.left, height = rect.bottom - rect.top;
    if (width > work.right - work.left || height > work.bottom - work.top) {
        fprintf(stderr, "Window exceeds DISPLAY2: check isolated windowed video config\n");
        return 0;
    }
    int x = work.left + (work.right - work.left - width) / 2;
    int y = work.top + (work.bottom - work.top - height) / 2;
    if (!SetWindowPos(target.window, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) return 0;
    if (MonitorFromWindow(target.window, MONITOR_DEFAULTTONEAREST) != second) return 0;
    RECT placed;
    if (!GetWindowRect(target.window, &placed) || placed.left < work.left ||
        placed.top < work.top || placed.right > work.right || placed.bottom > work.bottom) {
        fprintf(stderr, "Game window is not fully inside DISPLAY2 work area\n");
        return 0;
    }
    RECT client;
    GetClientRect(target.window, &client);
    printf("window_display=DISPLAY2 x=%d y=%d client=%ldx%ld\n", x, y, client.right, client.bottom);
    printf("window_contained=1 work=%ld,%ld,%ld,%ld outer=%ld,%ld,%ld,%ld\n",
        work.left, work.top, work.right, work.bottom,
        placed.left, placed.top, placed.right, placed.bottom);
    fflush(stdout);
    return 1;
}

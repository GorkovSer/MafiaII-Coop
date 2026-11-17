#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
int WINAPI WinMain(HINSTANCE a, HINSTANCE b, LPSTR c, int d) {
    (void)a; (void)b; (void)c; (void)d;
    char root[MAX_PATH], expected[MAX_PATH], windows[MAX_PATH], actual_windows[MAX_PATH];
    if (!GetEnvironmentVariableA("COOP_PROFILE_TEST_ROOT", expected, MAX_PATH)) return 40;
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, root))) return 41;
    if (_stricmp(root, expected)) return 42; /* Never write into an unredirected user profile. */
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_WINDOWS, NULL, 0, windows))) return 43;
    if (!GetWindowsDirectoryA(actual_windows, MAX_PATH) || _stricmp(windows, actual_windows)) return 44;
    if (strlen(root) + strlen("\\profile-test.marker") >= MAX_PATH) return 45;
    strcat(root, "\\profile-test.marker");
    HANDLE file = CreateFileA(root, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 46;
    DWORD written;
    const char message[] = "real host wrote into isolated profile\r\n";
    BOOL ok = WriteFile(file, message, sizeof(message) - 1, &written, NULL);
    CloseHandle(file);
    return ok ? 0 : 47;
}

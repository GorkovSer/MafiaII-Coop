/* Process-local SHGetFolderPathA redirect. No directory renames or changes to
   the real user profile. Enable before EXE entry, verify before story tests. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

static char isolated_local[MAX_PATH];
static wchar_t evidence_path[MAX_PATH];
static volatile LONG redirected;
int CoopProfileWasRedirected(void) {return InterlockedCompareExchange(&redirected,0,0)>0;}
typedef HRESULT (WINAPI *folder_type)(HWND, int, HANDLE, DWORD, LPSTR);
static folder_type original_folder;

static void evidence(const char *text) {
    HANDLE file = CreateFileW(evidence_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
    CloseHandle(file);
}

static HRESULT WINAPI isolated_folder(HWND owner, int folder, HANDLE token, DWORD flags, LPSTR path) {
    if ((folder & 0xff) != CSIDL_LOCAL_APPDATA)
        return original_folder(owner, folder, token, flags, path);
    if (!path) return E_INVALIDARG;
    strcpy(path, isolated_local);  /* SHGetFolderPath contract: MAX_PATH buffer. */
    if (InterlockedIncrement(&redirected) == 1)
        evidence("profile_local_appdata_redirected=1\r\n");
    return S_OK;
}

DWORD CoopProfileInstall(const wchar_t *log_path) {
    if (!log_path || wcslen(log_path) >= MAX_PATH) return 20;
    wcscpy(evidence_path, log_path);
    wchar_t root[MAX_PATH];
    wcscpy(root, log_path);
    wchar_t *tail = wcsrchr(root, L'\\');
    if (!tail) return 21;
    *tail = 0;
    if (wcslen(root) + wcslen(L"\\profile-localappdata") >= MAX_PATH) return 22;
    wcscat(root, L"\\profile-localappdata");
    DWORD attr = GetFileAttributesW(root);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return 23;
    BOOL substituted = FALSE;
    if (!WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, root, -1, isolated_local,
                            MAX_PATH, NULL, &substituted) || substituted) return 24;
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    DWORD imports_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!imports_rva) return 25;
    IMAGE_IMPORT_DESCRIPTOR *imports = (IMAGE_IMPORT_DESCRIPTOR *)(base + imports_rva);
    for (; imports->Name; ++imports) {
        if (_stricmp((const char *)(base + imports->Name), "shell32.dll")) continue;
        if (!imports->OriginalFirstThunk) return 26;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + imports->OriginalFirstThunk);
        IMAGE_THUNK_DATA *slots = (IMAGE_THUNK_DATA *)(base + imports->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++slots) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *symbol = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
            if (strcmp((const char *)symbol->Name, "SHGetFolderPathA")) continue;
            original_folder = (folder_type)(uintptr_t)slots->u1.Function;
            DWORD old, ignored;
            if (!VirtualProtect(&slots->u1.Function, sizeof(slots->u1.Function), PAGE_READWRITE, &old)) return 27;
            slots->u1.Function = (ULONG_PTR)isolated_folder;
            if (!VirtualProtect(&slots->u1.Function, sizeof(slots->u1.Function), old, &ignored)) return 28;
            evidence("profile_import_hook_installed=1\r\n");
            return 0;
        }
    }
    return 29;
}

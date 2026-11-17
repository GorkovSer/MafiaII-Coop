/* Diagnostic only: no game hooks, SDK initialization, network, or save writes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(instance);
    return TRUE;
}

/* Invoked explicitly after LoadLibrary has returned, outside the loader lock. */
__declspec(dllexport) DWORD WINAPI CoopProbeStart(void *log_path) {
    if (!log_path) return 2;
    HANDLE file = CreateFileW((const wchar_t *)log_path, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 3;
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)GetModuleHandleW(NULL);
    const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)((const char *)dos + dos->e_lfanew);
    char line[512];
    int count = snprintf(line, sizeof(line),
        "probe_version=0.0.1\r\nprocess_id=%lu\r\npointer_bits=%u\r\n"
        "pe_machine=%04x\r\nimage_base=%p\r\nentry_rva=%08lx\r\n"
        "game_api_calls=0\r\ngame_patches=0\r\nstatus=loaded_inside_process\r\n",
        (unsigned long)GetCurrentProcessId(), (unsigned)(sizeof(void *) * 8),
        (unsigned)nt->FileHeader.Machine, (void *)dos,
        (unsigned long)nt->OptionalHeader.AddressOfEntryPoint);
    DWORD written = 0;
    BOOL ok = count > 0 && (size_t)count < sizeof(line) &&
        WriteFile(file, line, (DWORD)count, &written, NULL) && written == (DWORD)count;
    FlushFileBuffers(file);
    CloseHandle(file);
    return ok ? 0 : 4;
}

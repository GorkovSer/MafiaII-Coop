/* Launches a new owned process and loads our diagnostic DLL. Never attaches
   to arbitrary PIDs or changes game code/files. Must be compiled as x86. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
int CoopSecondMonitorAvailable(void);
int CoopPlaceOnSecondMonitor(DWORD pid, HANDLE process);
BOOL CoopCreateUserProcess(const wchar_t *, wchar_t *, const wchar_t *, STARTUPINFOW *, PROCESS_INFORMATION *);

static uintptr_t remote_module(DWORD pid, const wchar_t *name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    MODULEENTRY32W entry = {0};
    entry.dwSize = sizeof(entry);
    uintptr_t result = 0;
    if (snapshot != INVALID_HANDLE_VALUE && Module32FirstW(snapshot, &entry)) do {
        if (!_wcsicmp(entry.szModule, name)) {
            result = (uintptr_t)entry.modBaseAddr;
            break;
        }
    } while (Module32NextW(snapshot, &entry));
    if (snapshot != INVALID_HANDLE_VALUE) CloseHandle(snapshot);
    if (!result) {
        /* Before loader initialization the PEB module list can still be empty.
           Query mapped image allocations directly rather than guessing ASLR. */
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!process) return 0;
        MEMORY_BASIC_INFORMATION region;
        uintptr_t cursor = 0;
        while (VirtualQueryEx(process, (void *)cursor, &region, sizeof(region)) == sizeof(region)) {
            if (region.Type == MEM_IMAGE && region.BaseAddress == region.AllocationBase) {
                wchar_t mapped[MAX_PATH];
                DWORD n = GetMappedFileNameW(process, region.AllocationBase, mapped, MAX_PATH);
                if (n && n < MAX_PATH) {
                    wchar_t *base = wcsrchr(mapped, L'\\');
                    if (!_wcsicmp(base ? base + 1 : mapped, name)) {
                        result = (uintptr_t)region.AllocationBase;
                        break;
                    }
                }
            }
            uintptr_t next = (uintptr_t)region.BaseAddress + region.RegionSize;
            if (next <= cursor) break;
            cursor = next;
        }
        CloseHandle(process);
    }
    return result;
}

static DWORD call_remote(HANDLE process, uintptr_t address, void *arg, DWORD *code) {
    HANDLE thread = CreateRemoteThread(process, NULL, 0,
        (LPTHREAD_START_ROUTINE)address, arg, 0, NULL);
    if (!thread) return GetLastError();
    DWORD wait = WaitForSingleObject(thread, 15000);
    DWORD result = 0;
    if (wait != WAIT_OBJECT_0) result = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
    else if (!GetExitCodeThread(thread, code)) result = GetLastError();
    CloseHandle(thread);
    return result;
}

int wmain(int argc, wchar_t **argv) {
    int monitor = 0, early = 0, second_screen = 0, user = 0;
    for (int i = 4; i < argc; ++i) {
        if (!wcscmp(argv[i], L"--wait")) monitor = 1;
        else if (!wcscmp(argv[i], L"--early")) early = 1;
        else if (!wcscmp(argv[i], L"--monitor2")) second_screen = 1;
        else if (!wcscmp(argv[i], L"--user")) user = 1;
        else return 2;
    }
    if (argc < 4 || sizeof(void *) != 4) {
        fwprintf(stderr, L"Usage: probe_launcher.exe <absolute exe> <absolute DLL> <new absolute log> [--wait] [--early] [--monitor2] [--user]\n");
        return 2;
    }
    /* Keep monitor bounds, window rectangles and SetWindowPos in physical
       coordinates on mixed-DPI desktops. This affects this launcher thread
       only, not the child's DPI policy or the user's display settings. */
    typedef DPI_AWARENESS_CONTEXT (WINAPI *set_dpi_type)(DPI_AWARENESS_CONTEXT);
    set_dpi_type set_dpi = (set_dpi_type)(uintptr_t)GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext");
    if (second_screen && (!set_dpi || !set_dpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))) {
        fprintf(stderr, "Cannot establish physical display coordinates; no process launched\n"); return 14;
    }
    if (second_screen && !CoopSecondMonitorAvailable()) {
        fprintf(stderr, "DISPLAY2 unavailable; no process launched\n"); return 14;
    }
    wchar_t exe[MAX_PATH], dll[MAX_PATH], log[MAX_PATH], cwd[MAX_PATH], cmd[MAX_PATH + 4];
    wchar_t *paths[] = {exe, dll, log};
    for (int i = 0; i < 3; ++i) {
        DWORD length = GetFullPathNameW(argv[i + 1], MAX_PATH, paths[i], NULL);
        if (!length || length >= MAX_PATH) return 3;
    }
    if (GetFileAttributesW(log) != INVALID_FILE_ATTRIBUTES) {
        fwprintf(stderr, L"Refusing to overwrite existing log\n");
        return 4;
    }
    HMODULE local_dll = LoadLibraryExW(dll, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!local_dll) { fprintf(stderr, "Invalid x86 probe DLL: %lu\n", GetLastError()); return 5; }
    FARPROC probe = GetProcAddress(local_dll, "CoopProbeStart");
    if (!probe) { FreeLibrary(local_dll); return 6; }
    uintptr_t probe_rva = (uintptr_t)probe - (uintptr_t)local_dll;
    FreeLibrary(local_dll);
    wcscpy(cwd, exe);
    wchar_t *separator = wcsrchr(cwd, L'\\');
    if (!separator) return 7;
    *separator = 0;
    swprintf(cmd, MAX_PATH + 4, L"\"%ls\"", exe);
    STARTUPINFOW startup = {0};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child = {0};
    BOOL created = user ? CoopCreateUserProcess(exe, cmd, cwd, &startup, &child) :
        CreateProcessW(exe, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, cwd, &startup, &child);
    if (!created) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError()); return 8;
    }
    DWORD failure = 0, exit_code = 0;
    void *remote = NULL;
    if (!early) {
        if (ResumeThread(child.hThread) == (DWORD)-1) { failure = GetLastError(); goto cleanup; }
        /* GUI input idle is only startup coordination, not evidence of gameplay. */
        WaitForInputIdle(child.hProcess, 10000);
    }
    HMODULE local_owner = NULL;
    FARPROC load = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!load || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)load, &local_owner)) {
        failure = ERROR_PROC_NOT_FOUND; goto cleanup;
    }
    wchar_t owner_path[MAX_PATH];
    if (!GetModuleFileNameW(local_owner, owner_path, MAX_PATH)) { failure = GetLastError(); goto cleanup; }
    wchar_t *owner_name = wcsrchr(owner_path, L'\\');
    uintptr_t owner = remote_module(child.dwProcessId, owner_name ? owner_name + 1 : owner_path);
    if (!owner && early) {
        /* Let Windows initialize the new process' DLL loader on a temporary
           thread while the primary thread remains suspended before entry.
           Its only user routine exits itself. Resolve the actual export RVA
           from the system's ntdll, never assume equal DLL base addresses. */
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        FARPROC exit_thread = GetProcAddress(ntdll, "RtlExitUserThread");
        uintptr_t remote_ntdll = remote_module(child.dwProcessId, L"ntdll.dll");
        if (!exit_thread || !remote_ntdll) { failure = ERROR_PROC_NOT_FOUND; goto cleanup; }
        failure = call_remote(child.hProcess,
            remote_ntdll + (uintptr_t)exit_thread - (uintptr_t)ntdll, NULL, &exit_code);
        if (failure) goto cleanup;
        owner = remote_module(child.dwProcessId, owner_name ? owner_name + 1 : owner_path);
    }
    if (!owner) { failure = ERROR_MOD_NOT_FOUND; goto cleanup; }
    size_t dll_bytes = (wcslen(dll) + 1) * sizeof(wchar_t);
    size_t log_bytes = (wcslen(log) + 1) * sizeof(wchar_t);
    remote = VirtualAllocEx(child.hProcess, NULL, dll_bytes + log_bytes,
                           MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!remote) { failure = GetLastError(); goto cleanup; }
    SIZE_T written = 0;
    if (!WriteProcessMemory(child.hProcess, remote, dll, dll_bytes, &written) || written != dll_bytes ||
        !WriteProcessMemory(child.hProcess, (char *)remote + dll_bytes, log, log_bytes, &written) || written != log_bytes) {
        failure = ERROR_WRITE_FAULT; goto cleanup;
    }
    failure = call_remote(child.hProcess, owner + (uintptr_t)load - (uintptr_t)local_owner, remote, &exit_code);
    if (failure) goto cleanup;
    if (!exit_code) { failure = ERROR_DLL_INIT_FAILED; goto cleanup; }
    uintptr_t loaded_dll = exit_code;
    failure = call_remote(child.hProcess, loaded_dll + probe_rva, (char *)remote + dll_bytes, &exit_code);
    if (!failure && exit_code) {
        fprintf(stderr, "probe_export_return=%lu\n", exit_code);
        failure = ERROR_FUNCTION_FAILED;
    }
    if (!failure && early && ResumeThread(child.hThread) == (DWORD)-1) failure = GetLastError();
    if (!failure && second_screen && !CoopPlaceOnSecondMonitor(child.dwProcessId, child.hProcess))
        failure = ERROR_INVALID_WINDOW_HANDLE;
cleanup:
    if (failure) {
        fprintf(stderr, "Probe failed: %lu; newly launched process terminated\n", failure);
        TerminateProcess(child.hProcess, 1);
        WaitForSingleObject(child.hProcess, 5000);
    } else {
        if (remote) VirtualFreeEx(child.hProcess, remote, 0, MEM_RELEASE);
        printf("probe_loaded pid=%lu log_written=1\n", child.dwProcessId);
        fflush(stdout);
        if (monitor) {
            DWORD wait = WaitForSingleObject(child.hProcess, INFINITE);
            DWORD child_exit = 0;
            if (wait != WAIT_OBJECT_0 || !GetExitCodeProcess(child.hProcess, &child_exit))
                failure = ERROR_FUNCTION_FAILED;
            else {
                printf("child_exit=%lu\n", child_exit);
                if (child_exit) failure = ERROR_PROCESS_ABORTED;
            }
        }
    }
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
    return failure ? 9 : 0;
}

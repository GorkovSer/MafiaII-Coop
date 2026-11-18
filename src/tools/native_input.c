/* User-authorized menu input/quit for an explicitly selected diagnostic game.
   No global hooks, arbitrary text, focus stealing or process memory writes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static int image_ok(HANDLE process) {
    static const unsigned char expected[32] = {
        0x16,0xc7,0x9a,0x3b,0xe4,0x83,0xe5,0x82,0x71,0x9e,0x43,0xd7,0xb4,0x51,0xe8,0x87,
        0xe8,0x94,0x14,0xbe,0x81,0xca,0x7e,0xcf,0xc0,0x53,0x6d,0xd8,0x18,0x5e,0x6c,0x2f};
    wchar_t path[32768]; DWORD size = 32768;
    if (!QueryFullProcessImageNameW(process, 0, path, &size)) return 0;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    HCRYPTPROV provider = 0; HCRYPTHASH hash = 0;
    unsigned char block[8192], digest[32]; DWORD count, length = sizeof(digest);
    int ok = CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash);
    while (ok) {
        if (!ReadFile(file, block, sizeof(block), &count, NULL)) { ok = 0; break; }
        if (!count) break;
        ok = CryptHashData(hash, block, count, 0);
    }
    if (ok) ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &length, 0) &&
        length == sizeof(expected) && !memcmp(digest, expected, sizeof(expected));
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    CloseHandle(file); return ok;
}
static int number(const wchar_t *text, unsigned long long *value) {
    if (!*text) return 0;
    for (const wchar_t *p = text; *p; ++p) if (*p < L'0' || *p > L'9') return 0;
    wchar_t *end; *value = wcstoull(text, &end, 10);
    return !*end && *value != ULLONG_MAX;
}
static int focused(DWORD pid, HANDLE process) {
    DWORD foreground = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    return foreground == pid && WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
}
int wmain(int argc, wchar_t **argv) {
    unsigned long long pid64, expected_creation = 0, hold_ms = 100;
    int inspect = argc == 3 && !wcscmp(argv[1], L"inspect");
    int quit = argc == 4 && !wcscmp(argv[1], L"quit");
    if ((!inspect && !quit && ((argc != 5 && argc != 6) || wcscmp(argv[1], L"key"))) ||
        !number(argv[2], &pid64) || !pid64 || pid64 > MAXDWORD) {
        fputs("usage: coop_input inspect PID | quit PID CREATION | key PID CREATION up|down|left|right|enter [hold_ms:50..2000]\n", stderr); return 2;
    }
    WORD scan = 0; DWORD flags = KEYEVENTF_SCANCODE;
    if (!inspect) {
        if (!number(argv[3], &expected_creation)) return 2;
    }
    if (!inspect && !quit) {
        if (argc == 6 && (!number(argv[5], &hold_ms) || hold_ms < 50 || hold_ms > 2000)) return 2;
        if (!wcscmp(argv[4], L"enter")) scan = 0x1c;
        else {
            flags |= KEYEVENTF_EXTENDEDKEY;
            if (!wcscmp(argv[4], L"up")) scan = 0x48;
            else if (!wcscmp(argv[4], L"down")) scan = 0x50;
            else if (!wcscmp(argv[4], L"left")) scan = 0x4b;
            else if (!wcscmp(argv[4], L"right")) scan = 0x4d;
            else return 2;
        }
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid64);
    FILETIME created, exited, kernel, user;
    if (!process) return 3;
    if (!GetProcessTimes(process, &created, &exited, &kernel, &user) ||
        WaitForSingleObject(process, 0) != WAIT_TIMEOUT || !image_ok(process)) {
        CloseHandle(process); fputs("process/image validation failed\n", stderr); return 3;
    }
    unsigned long long creation = ((uint64_t)created.dwHighDateTime << 32) | created.dwLowDateTime;
    if (inspect) { printf("pid=%lu creation=%llu image_verified=1\n", (DWORD)pid64, creation); CloseHandle(process); return 0; }
    if (creation != expected_creation || !focused((DWORD)pid64, process)) {
        CloseHandle(process); fputs("creation/focus validation failed; no input sent\n", stderr); return 4;
    }
    if (quit) {
        /* User requested Enter immediately after Alt+F4, without a screenshot/tool
           round trip. Validate once before the chord, never steal focus. */
        ULONGLONG begin = GetTickCount64();
        INPUT chord[4] = {0};
        for (int i = 0; i < 4; ++i) {
            chord[i].type = INPUT_KEYBOARD;
            chord[i].ki.dwFlags = KEYEVENTF_SCANCODE | (i >= 2 ? KEYEVENTF_KEYUP : 0);
        }
        chord[0].ki.wScan = chord[3].ki.wScan = 0x38; /* left Alt */
        chord[1].ki.wScan = chord[2].ki.wScan = 0x3e; /* F4 */
        if (SendInput(4, chord, sizeof(INPUT)) != 4) {
            SendInput(2, &chord[2], sizeof(INPUT));
            CloseHandle(process); return 5;
        }
        unsigned int keys = 0;
        while (WaitForSingleObject(process, 0) == WAIT_TIMEOUT && GetTickCount64() - begin < 5000) {
            if (!focused((DWORD)pid64, process)) break;
            INPUT enter = {0}; enter.type = INPUT_KEYBOARD;
            enter.ki.wScan = 0x1c; enter.ki.dwFlags = KEYEVENTF_SCANCODE;
            if (SendInput(1, &enter, sizeof(enter)) != 1) break;
            ++keys;
            ULONGLONG down = GetTickCount64();
            while (GetTickCount64() - down < 100 && focused((DWORD)pid64, process)) Sleep(5);
            enter.ki.dwFlags |= KEYEVENTF_KEYUP;
            if (SendInput(1, &enter, sizeof(enter)) != 1) { CloseHandle(process); return 5; }
            if (WaitForSingleObject(process, 50) == WAIT_OBJECT_0) break;
        }
        /* The window can disappear before the process handle signals. Send no more keys. */
        int exited_ok = WaitForSingleObject(process, 1000) == WAIT_OBJECT_0;
        DWORD exit_code = STILL_ACTIVE;
        GetExitCodeProcess(process, &exit_code);
        printf("quit_enter_keys=%u elapsed_ms=%llu process_exited=%d exit_code=%lu\n",
            keys, (unsigned long long)(GetTickCount64() - begin), exited_ok, exit_code);
        CloseHandle(process);
        return exited_ok && exit_code == 0 ? 0 : 5;
    }
    INPUT input = {0}; input.type = INPUT_KEYBOARD; input.ki.wScan = scan; input.ki.dwFlags = flags;
    if (SendInput(1, &input, sizeof(input)) != 1) { CloseHandle(process); return 5; }
    ULONGLONG started = GetTickCount64();
    while (GetTickCount64() - started < hold_ms && focused((DWORD)pid64, process)) Sleep(5);
    /* Always release a pressed key, including after an intervening focus change. */
    int retained_focus = focused((DWORD)pid64, process);
    input.ki.dwFlags |= KEYEVENTF_KEYUP;
    UINT released = SendInput(1, &input, sizeof(input));
    CloseHandle(process);
    printf("scan=%u released=%u focus_retained=%d\n", scan, released, retained_focus);
    return released == 1 && retained_focus ? 0 : 5;
}

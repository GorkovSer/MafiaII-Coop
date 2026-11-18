/* User-authorized menu input for an explicitly selected diagnostic game.
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
    unsigned long long pid64, expected_creation = 0;
    int inspect = argc == 3 && !wcscmp(argv[1], L"inspect");
    if ((!inspect && (argc != 5 || wcscmp(argv[1], L"key"))) ||
        !number(argv[2], &pid64) || !pid64 || pid64 > MAXDWORD) {
        fputs("usage: coop_input inspect PID | key PID CREATION up|down|left|right|enter\n", stderr); return 2;
    }
    WORD scan = 0; DWORD flags = KEYEVENTF_SCANCODE;
    if (!inspect) {
        if (!number(argv[3], &expected_creation)) return 2;
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
    INPUT input = {0}; input.type = INPUT_KEYBOARD; input.ki.wScan = scan; input.ki.dwFlags = flags;
    if (SendInput(1, &input, sizeof(input)) != 1) { CloseHandle(process); return 5; }
    Sleep(100);
    /* Always release a pressed key, including after an intervening focus change. */
    int retained_focus = focused((DWORD)pid64, process);
    input.ki.dwFlags |= KEYEVENTF_KEYUP;
    UINT released = SendInput(1, &input, sizeof(input));
    CloseHandle(process);
    printf("scan=%u released=%u focus_retained=%d\n", scan, released, retained_focus);
    return released == 1 && retained_focus ? 0 : 5;
}

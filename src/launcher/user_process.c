/* Lower only our child's privileges. Prefer this account's linked limited
   token; when no split token exists, restrict our own token instead.
   No other process tokens, credentials or system policy changes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static int token_is_medium(HANDLE token) {
    DWORD size;
    struct {TOKEN_MANDATORY_LABEL label; BYTE sid[SECURITY_MAX_SID_SIZE];} integrity;
    if (!GetTokenInformation(token, TokenIntegrityLevel, &integrity, sizeof(integrity), &size)) return -1;
    DWORD count=*GetSidSubAuthorityCount(integrity.label.Label.Sid);
    if (!count) return -1;
    return *GetSidSubAuthority(integrity.label.Label.Sid,count-1)<=SECURITY_MANDATORY_MEDIUM_RID;
}
static HANDLE restrict_own_token(HANDLE original) {
    BYTE admin_sid[SECURITY_MAX_SID_SIZE]; DWORD size=sizeof(admin_sid);
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid,NULL,admin_sid,&size)) return NULL;
    SID_AND_ATTRIBUTES disable={admin_sid,0};
    HANDLE restricted=NULL;
    if (!CreateRestrictedToken(original,DISABLE_MAX_PRIVILEGE,1,&disable,0,NULL,0,NULL,&restricted)) return NULL;
    SID_IDENTIFIER_AUTHORITY authority=SECURITY_MANDATORY_LABEL_AUTHORITY;
    PSID medium=NULL;
    if (!AllocateAndInitializeSid(&authority,1,SECURITY_MANDATORY_MEDIUM_RID,0,0,0,0,0,0,0,&medium)) {
        CloseHandle(restricted);return NULL;
    }
    TOKEN_MANDATORY_LABEL label={{medium,SE_GROUP_INTEGRITY}};
    BOOL ok=SetTokenInformation(restricted,TokenIntegrityLevel,&label,sizeof(label)+GetLengthSid(medium));
    DWORD error=GetLastError();FreeSid(medium);
    if (!ok) {CloseHandle(restricted);SetLastError(error);return NULL;}
    return restricted;
}
BOOL CoopCreateUserProcess(const wchar_t *exe, wchar_t *command, const wchar_t *cwd,
                          STARTUPINFOW *startup, PROCESS_INFORMATION *child) {
    HANDLE token=NULL,limited=NULL;
    int restricted_own=0;
    if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_DEFAULT,&token)) return FALSE;
    int medium=token_is_medium(token);
    if (medium<0) {CloseHandle(token);return FALSE;}
    if (medium) {
        CloseHandle(token);
        return CreateProcessW(exe,command,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,cwd,startup,child);
    }
    DWORD size; TOKEN_ELEVATION_TYPE elevation;
    if (!GetTokenInformation(token,TokenElevationType,&elevation,sizeof(elevation),&size)) {
        DWORD error=GetLastError();CloseHandle(token);SetLastError(error);return FALSE;
    }
    if (elevation==TokenElevationTypeFull) {
        TOKEN_LINKED_TOKEN linked={0};
        if (GetTokenInformation(token,TokenLinkedToken,&linked,sizeof(linked),&size)) limited=linked.LinkedToken;
    } else {limited=restrict_own_token(token);restricted_own=1;}
    CloseHandle(token);
    if (!limited) return FALSE;
    BOOL ok=token_is_medium(limited)==1;
    if (!ok) SetLastError(ERROR_ACCESS_DENIED);
    if (ok) {
        if (restricted_own) ok=CreateProcessAsUserW(limited,exe,command,NULL,NULL,FALSE,CREATE_SUSPENDED,
                                                  NULL,cwd,startup,child);
        else ok=CreateProcessWithTokenW(limited,0,exe,command,CREATE_SUSPENDED,NULL,cwd,startup,child);
    }
    DWORD error=GetLastError(); CloseHandle(limited); SetLastError(error);
    if (ok) {printf("child_token=medium_same_account\n");fflush(stdout);}
    return ok;
}

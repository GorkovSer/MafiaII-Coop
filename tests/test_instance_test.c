/* Import adapter contract without loading or touching Mafia II. */
#include "../src/probe/instance_test.c"
static LPCSTR seen_name;static LPSECURITY_ATTRIBUTES seen_security;static BOOL seen_owner;
static HANDLE answer;static DWORD answer_error;
static HANDLE WINAPI fake_create(LPSECURITY_ATTRIBUTES security,BOOL owner,LPCSTR name) {
    seen_name=name;seen_security=security;seen_owner=owner;SetLastError(answer_error);return answer;
}
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL instance import contract line %d\n",__LINE__);return 1;}}while(0)
int main(void) {
    SECURITY_ATTRIBUTES security={sizeof(security),NULL,FALSE};
    original_create=fake_create;strcpy(private_name,"fixture-private-instance");
    answer=(HANDLE)(uintptr_t)123;answer_error=ERROR_ALREADY_EXISTS;
    CHECK(create_for_caller(&security,TRUE,(LPCSTR)(uintptr_t)0x18edc00,0x401baa)==answer);
    CHECK(seen_name==private_name && seen_security==&security && seen_owner==TRUE && GetLastError()==ERROR_ALREADY_EXISTS);
    CHECK(create_for_caller(NULL,FALSE,(LPCSTR)(uintptr_t)0x18edc00,0x401bab)==answer);
    CHECK(seen_name==(LPCSTR)(uintptr_t)0x18edc00 && !seen_security && !seen_owner && GetLastError()==ERROR_ALREADY_EXISTS);
    const char installer[]="Mafia2 Installer Super Mutex";
    create_for_caller(NULL,FALSE,installer,0x401baa);CHECK(seen_name==installer);
    const char copy[]="Mafia2 Launcher Super Mutex";
    create_for_caller(NULL,FALSE,copy,0x401baa);CHECK(seen_name==copy);
    create_for_caller(NULL,FALSE,NULL,0x401baa);CHECK(!seen_name);
    answer=NULL;answer_error=ERROR_ACCESS_DENIED;
    CHECK(!create_for_caller(&security,TRUE,(LPCSTR)(uintptr_t)0x18edc00,0x401baa));
    CHECK(GetLastError()==ERROR_ACCESS_DENIED);
    puts("PASS instance import: exact caller/literal, unrelated and installer passthrough, unchanged security/ownership, handle and last-error preservation");
    return 0;
}

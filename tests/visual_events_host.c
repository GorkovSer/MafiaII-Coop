/* Production visual event policy with no game/network calls. Profile guard is
   always false; the stub receiver never supplies an actor-creation candidate. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/game/visual_test.h"
#include "../src/net/game_bridge.h"
#include <stdio.h>
#include <string.h>
static uint32_t published;
int CoopProfileWasRedirected(void) {return 0;}
void CoopNetworkPublishVisualScene(uint32_t epoch) {published=epoch;}
int CoopNetworkReadRemoteVisual(CoopRemoteSnapshot *out) {memset(out,0,sizeof(*out));return 0;}
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL visual events line %d\n",__LINE__);return 1;}}while(0)
int wmain(int argc,wchar_t **argv) {
    if(argc!=2) return 2;
    CHECK(!CoopVisualTestStart(argv[1]));
    CoopVisualTestEvent(10); CoopVisualTestEvent(13); CoopVisualTestEvent(32);
    CHECK(!published); /* Initial pre-load teardown must not consume approval. */
    CoopVisualTestEvent(33); CHECK(published==1);
    CoopVisualTestEvent(34); CHECK(!published);
    CoopVisualTestEvent(4); CoopVisualTestEvent(3); CHECK(!published);
    CoopVisualTestEvent(35); CHECK(published==1);
    CoopVisualTestEvent(4); CoopVisualTestEvent(3); CHECK(published==1);
    CoopVisualTestEvent(32); CHECK(!published);
    CoopVisualTestEvent(33); CoopVisualTestEvent(35); CoopVisualTestEvent(3); CHECK(!published);
    CoopVisualTestEvent(10); CoopVisualTestEvent(13); CoopVisualTestEvent(33); CHECK(!published);
    puts("PASS visual event policy: initial load, pause/resume ordering, one-load scene approval");
    return 0;
}

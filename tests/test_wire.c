/* Protocol fixtures only; not game or Radmin VPN acceptance. */
#include "../src/net/wire.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    CoopMessage state={0}, decoded;
    unsigned char bytes[COOP_WIRE_MAX], original[COOP_WIRE_MAX];
    state.type=COOP_STATE; state.session=0x0102030405060708ull;
    state.sequence=0xfffffffdu; state.player_id=2; state.epoch=7; state.active=1;
    state.position[0]=1.25f; state.position[1]=-32.5f; state.position[2]=3.0f;
    size_t size=CoopWireEncode(&state, bytes);
    assert(size==60 && bytes[8]==1 && bytes[15]==8 && bytes[24]==0 && bytes[27]==2);
    assert(CoopWireDecode(bytes,size,&decoded) && decoded.session==state.session && decoded.position[1]==-32.5f);
    state.heading_valid=1; state.heading=-COOP_HEADING_MAX;
    assert(CoopWireEncode(&state,bytes)==60 && CoopWireDecode(bytes,60,&decoded));
    assert(decoded.heading_valid && decoded.heading==state.heading);
    bytes[5]=2; assert(!CoopWireDecode(bytes,60,&decoded)); /* Old protocol refused. */
    state.heading=COOP_HEADING_MAX; assert(CoopWireEncode(&state,bytes)==60);
    state.heading=NAN; assert(!CoopWireEncode(&state,bytes));
    state.heading=INFINITY; assert(!CoopWireEncode(&state,bytes));
    state.heading=4; assert(!CoopWireEncode(&state,bytes));
    state.heading=0; state.heading_valid=2; assert(!CoopWireEncode(&state,bytes));
    state.heading=1; state.heading_valid=0; assert(!CoopWireEncode(&state,bytes));
    state.heading=0; state.heading_valid=1; state.active=0;
    assert(!CoopWireEncode(&state,bytes));
    state.heading_valid=0; state.active=1;
    assert(CoopWireEncode(&state,bytes)==60); memcpy(original,bytes,sizeof(bytes));
    memcpy(original,bytes,sizeof(bytes));
    for(size_t i=0;i<size;++i) assert(!CoopWireDecode(bytes,i,&decoded));
    assert(!CoopWireDecode(bytes,size+1,&decoded));
    bytes[5]++; assert(!CoopWireDecode(bytes,size,&decoded)); memcpy(bytes,original,sizeof(bytes));
    bytes[22]=1; assert(!CoopWireDecode(bytes,size,&decoded)); memcpy(bytes,original,sizeof(bytes));
    bytes[7]=99; assert(!CoopWireDecode(bytes,size,&decoded)); memcpy(bytes,original,sizeof(bytes));
    bytes[36]=0x7f; bytes[37]=0xc0; bytes[38]=bytes[39]=0; assert(!CoopWireDecode(bytes,size,&decoded));
    state.position[0]=NAN; assert(!CoopWireEncode(&state,bytes));
    state.position[0]=0; state.active=0; assert(!CoopWireEncode(&state,bytes));
    memset(state.position,0,sizeof(state.position)); assert(CoopWireEncode(&state,bytes)==60);
    assert(CoopSequenceNewer(0,0xffffffffu) && !CoopSequenceNewer(3,3) && !CoopSequenceNewer(2,3));
    assert(!CoopSequenceNewer(0x80000000u,0));
    CoopMessage hello={0}; hello.type=COOP_HELLO; hello.module_version=COOP_MODULE_VERSION;
    memcpy(hello.build,CoopGameBuild,32); hello.capabilities=COOP_CAP_TELEMETRY;
    assert(CoopWireEncode(&hello,bytes)==64 && CoopWireDecode(bytes,64,&decoded));
    assert(decoded.module_version==COOP_MODULE_VERSION && !memcmp(decoded.build,CoopGameBuild,32));
    CoopMessage offer = {0}; offer.type = COOP_WORLD_OFFER;
    offer.world.generation = 10; offer.world.epoch = 11; offer.world.key[31] = 7;
    assert(CoopWireEncode(&offer, bytes) == 64 && CoopWireDecode(bytes, 64, &decoded));
    assert(decoded.world.generation == 10 && decoded.world.epoch == 11 && decoded.world.key[31] == 7);
    for (size_t i = 0; i < 64; ++i) assert(!CoopWireDecode(bytes, i, &decoded));
    offer.world.epoch = 0; assert(!CoopWireEncode(&offer, bytes));
    memset(offer.world.key, 0, 32); assert(CoopWireEncode(&offer, bytes) == 64);
    offer.type = COOP_WORLD_COMMIT; assert(!CoopWireEncode(&offer, bytes));
    puts("PASS wire encoding, truncation, versions, invalid states and sequence wrap");
    return 0;
}

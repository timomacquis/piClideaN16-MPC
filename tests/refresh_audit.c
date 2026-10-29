/* Offline notification audit. No ALSA device is opened. */
#define main existing_mod_tests_main
#include "mod_engine_test.c"
#undef main
static int drain_audit(Engine *e,int *visuals){char key[64];int n=0;*visuals=0;while(mpc_engine_changed(e,key,sizeof key)>0){n++;if(key[0]=='g'||!strncmp(key,"cell",4)||!strncmp(key,"track",5)||!strncmp(key,"lane_flash",10)||!strncmp(key,"mod_trace",9))(*visuals)++;assert(n<20000);}return n;}
static void audit_case(const char *name,const char *key,int value){Engine *e=fresh();int v;changed_all(e);visual_snapshot(e,0,1);drain_audit(e,&v);put(e,key,value);int ready=e->visual_ready;visual_snapshot(e,2048,0);int n=drain_audit(e,&v);printf("AUDIT %s | key=%s | cache_retained=%d | notifications=%d | graphical=%d\n",name,key,ready,n,v);visual_snapshot(e,2048,0);assert(drain_audit(e,&v)==0);done(e);}
static void slot_regressions(void){
 Engine *e=fresh();int v;changed_all(e);visual_snapshot(e,0,1);drain_audit(e,&v);
 put(e,"mod_choose1",1);assert(e->visual_ready);visual_snapshot(e,2048,0);assert(drain_audit(e,&v)<8&&v==0);
 put(e,"mod0_wave",1);put(e,"mod0_depth",90);visual_snapshot(e,2048,0);drain_audit(e,&v);
 int curve[128];memcpy(curve,e->mod_trace_cache[1][0],sizeof curve);
 put(e,"mod_choose0",1);visual_snapshot(e,2048,0);drain_audit(e,&v);assert(memcmp(curve,e->mod_trace_cache[0][0],sizeof curve));
 put(e,"mod_choose1",1);visual_snapshot(e,2048,0);assert(drain_audit(e,&v)>0);assert(!memcmp(curve,e->mod_trace_cache[1][0],sizeof curve));
 for(int pad=1;pad<=16;pad++){put(e,"open_rhythm",1);assert(e->changes<=3);put(e,"mod_pad",pad);assert(e->dialog==0&&e->mod_pad[1]==pad-1&&e->visual_ready);visual_snapshot(e,2048,0);drain_audit(e,&v);}
 put(e,"mod_choose0",1);visual_snapshot(e,2048,0);drain_audit(e,&v);put(e,"open_rhythm",1);put(e,"mod_pad",16);assert(e->dialog==6&&e->mod_pad[0]==-1);put(e,"apply_rhythm",1);assert(e->dialog==0&&e->mod_pad[0]==-1);visual_snapshot(e,2048,0);drain_audit(e,&v);
 put(e,"mod_choose1",1);put(e,"mod0_on",1);put(e,"mod0_dest",7);Hit on={15,0,0,100,0,0,2};ns=0;mod_trigger(e,&on,1);assert(e->mod_owner[15]==0);put(e,"open_rhythm",1);put(e,"mod_pad",1);assert(e->release_pending&(1u<<15));release(e,15);assert(e->active_note[15]<0);
 mod_advance(e,48,99);visual_snapshot(e,2048,0);drain_audit(e,&v);assert(!e->mod_trace_dirty||!(e->mod_trace_dirty&(1u<<e->mod_selected)));
 done(e);puts("PASS MOD cached slot switch / distinct curves / all 16 assignments / conflict / NONE / held-note release / tempo");
}
int main(void){slot_regressions();audit_case("MOD slot","mod_choose1",1);audit_case("MOD same slot","mod_choose0",1);audit_case("MOD pad layer open","open_rhythm",1);audit_case("MOD assignment","mod_pad",1);audit_case("MOD ON","mod0_on",1);audit_case("MOD wave","mod0_wave",1);audit_case("MOD depth","mod0_depth",37);audit_case("MOD threshold","mod0_threshold",20);audit_case("MOD mix","mod_mix",20);audit_case("TRACKS lower mute","toggle_l2_mute",1);audit_case("TRACKS upper solo","toggle_l12_solo",1);audit_case("TRACKS auto rotate","toggle_auto_rotate",1);audit_case("CONFIG HOLD","l12_hold",1);audit_case("CONFIG note","l12_dnote",79);return 0;}

#define main unused_mod_tests
#include "mod_engine_test.c"
#undef main
static void pause_ms(void){struct timespec t={0,2000000};nanosleep(&t,NULL);}
static void finish(Engine *e,int slot){for(int k=0;k<1000&&(e->mod_trace_dirty&(1u<<slot));k++){mod_preview(e,slot);pause_ms();}assert(!(e->mod_trace_dirty&(1u<<slot)));}
static void check_walk(void){
 Engine *e=fresh();put(e,"mod_pad",3);put(e,"mod1_on",1);put(e,"mod1_wave",8);put(e,"mod1_rate",0);put(e,"mod1_c0",-20);put(e,"mod1_dest",9);put(e,"mod0_on",1);put(e,"mod0_wave",10);mod_preview(e,0);
 int changes=0;for(int i=1;i<128;i++)changes+=e->mod_trace_cache[0][1][i]!=e->mod_trace_cache[0][1][0];assert(changes>80);assert(mod_style(e,1)!=2);
 for(int j=0;j<2;j++)for(int k=0;k<128;k++){int packed=mod_segment(e,0,j,k);assert(packed>=0&&packed<=3071);assert(packed%1024==e->mod_trace_cache[0][j][k]);}
 done(e);puts("PASS chained RANDOM WALK at 1/16 has a trajectory, not a full rectangle; atlas style packing bounded");
}
int main(void){
 check_walk();Engine *e=fresh(),*reference=fresh();put(e,"mod_pad",1);put(e,"mod0_wave",9);put(e,"mod0_on",1);put(e,"mod0_dest",7);put(e,"mod1_on",1);put(e,"mod1_wave",8);put(e,"mod1_dest",9);
 reference->mod[0]=e->mod[0];mod_coefficients(reference,0);mod_preview(reference,0);
 assert(mod_preview_start(e));ModPreviewWorker *w=e->mod_preview_worker;finish(e,0);
 assert(!memcmp(e->mod_trace_cache[0],reference->mod_trace_cache[0],sizeof e->mod_trace_cache[0]));
 assert(e->mod_preview_calculations==0&&atomic_load(&w->completed)>0);
 unsigned completed=atomic_load(&w->completed);
 char text[64];for(int n=0;n<10000;n++){get_param(e,"g0_0",text,sizeof text);assert(atoi(text)>=0);}
 assert(e->mod_preview_calculations==0&&atomic_load(&w->completed)==completed);
 /* Simulate an unavailable preview worker: reads and the MIDI path still run,
    and no synchronous image computation is used as a fallback. */
 assert(atomic_load(&w->state)==0);atomic_store(&w->state,2);put(e,"mod0_depth",31);
 for(int k=0;k<1000;k++){get_param(e,"g0_0",text,sizeof text);mod_advance(e,48,120);}
 assert(e->mod_preview_calculations==0&&(e->mod_trace_dirty&1));atomic_store(&w->state,0);finish(e,0);assert(e->mod_preview_calculations==0);
 /* A queued outdated configuration must not become the displayed result. */
 put(e,"mod0_depth",12);w->requested_at=0;mod_preview(e,0);put(e,"mod0_depth",87);finish(e,0);
 reference->mod[0]=e->mod[0];mod_coefficients(reference,0);reference->mod_trace_dirty=1;mod_preview(reference,0);
 assert(!memcmp(e->mod_trace_cache[0],reference->mod_trace_cache[0],sizeof e->mod_trace_cache[0]));
 assert(e->mod_preview_calculations==0);
 mod_preview_stop(e);assert(!e->mod_preview_worker);done(e);done(reference);
 for(int k=0;k<20;k++){e=fresh();assert(mod_preview_start(e));mod_preview(e,0);mod_preview_stop(e);done(e);}
 puts("PASS asynchronous immutable snapshots / cache-only host reads / busy-worker fallback / stale-result rejection / clean shutdown");return 0;
}

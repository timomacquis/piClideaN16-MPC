/* Reuse the real engine and MIDI capture harness, not a replica of the DSP. */
#define main original_mod_tests
#include "mod_engine_test.c"
#undef main
#include <time.h>
static void shapes(void){
 Engine *e=fresh();put(e,"mod_pad",1);put(e,"mod0_on",1);
 for(int shape=0;shape<MOD_WAVE_COUNT;shape++){
  put(e,"mod0_wave",shape);
  for(int n=0;n<2000;n++){mod_advance(e,48,120);double v=mod_wave_slot(e,0,0,.0005);assert(isfinite(v)&&fabs(v)<=1.000001);}
 }
 ModOsc *o=&e->mod[0].osc[0];o->f[1]=9;o->extra[0]=0;o->extra[1]=-100;
 for(int k=0;k<32;k++)assert(mod_shape(o,k+.1,123)==mod_shape(o,k+16+.1,123));
 o->extra[1]=100;int changes=0;for(int k=0;k<16;k++)changes+=mod_shape(o,k+.1,123)!=mod_shape(o,k+16+.1,123);assert(changes>12);
 o->f[1]=4;o->extra[0]=-60;assert(mod_shape(o,.1,1)==1&&mod_shape(o,.3,1)==-1);
 o->extra[0]=60;assert(mod_shape(o,.7,1)==1&&mod_shape(o,.9,1)==-1);
 o->f[1]=8;ModWalk w={0};double previous=mod_walk_value(&w,o,0,123);
 for(int k=1;k<10000;k++){double next=mod_walk_value(&w,o,k*.001,123);assert(fabs(next-previous)<.01);previous=next;}
 done(e);puts("PASS eleven bounded generators / pulse width / random loop repeat-evolve / walk continuity");
}
static void controls_and_state(void){
 Engine *e=fresh(),*t=fresh();put(e,"mod0_wave",7);put(e,"mod0_c0",-73);
 put(e,"mod0_wave",9);put(e,"mod0_c0",34);put(e,"mod0_c1",-100);put(e,"mod1_dest",10);
 assert(e->mod[0].osc[0].f[10]==-73&&e->mod[0].osc[0].extra[0]==34);
 char value[64];get_param(e,"mod0_c0",value,sizeof value);assert(!strcmp(value,"34"));
 put(e,"mod0_wave",7);get_param(e,"mod0_c0",value,sizeof value);assert(!strcmp(value,"-73"));
 char saved[32768];int len=write_evolution(e,saved,sizeof saved);assert(len>0&&read_evolution(t,saved));assert(!memcmp(e->mod,t->mod,sizeof e->mod));
 char broken[32768];strcpy(broken,saved);char *p=strstr(broken,"EXTRA ");assert(p);strcpy(p,"EXTRA 999 END");assert(!read_evolution(t,broken));assert(!memcmp(e->mod,t->mod,sizeof e->mod));
 strcpy(broken,saved);broken[4]='6';p=strstr(broken,"EXTRA ");strcpy(p,"END");assert(read_evolution(t,broken));assert(t->mod[0].osc[0].f[10]==-73);
 put(e,"open_sound7",1);assert(e->dialog==2);put(e,"map_sound",10);assert(e->dialog==0&&e->mod[0].osc[1].f[8]==10);
 put(e,"open_sound0",1);put(e,"map_sound",10);assert(e->dialog==0&&e->mod[0].osc[0].f[1]==10);
 done(e);done(t);puts("PASS contextual extras preserve FREE CURVE / PICN7 atomic state / PICN6 load / eleven-choice panels");
}
static void chains(void){
 Engine *e=fresh();put(e,"mod_pad",1);put(e,"mod0_on",1);put(e,"mod1_on",1);put(e,"mod0_dest",1);put(e,"mod1_dest",8);
 put(e,"mod1_depth",0);put(e,"mod1_offset",100);double hz=e->mod_hz[0][0];mod_advance(e,480,120);assert(e->mod_phase[0][0]>.01*hz*2);
 assert(e->mod[0].osc[0].f[3]==2&&e->mod_hz[0][0]==hz);
 put(e,"mod1_dest",9);put(e,"mod1_offset",-100);mod_advance(e,480,120);assert(mod_chain_read(e,0,0,.005,1)<0);
 put(e,"mod1_dest",10);mod_advance(e,480,120);assert(mod_chain_read(e,0,0,.005,2)==-1);
 put(e,"mod0_dest",8);put(e,"mod1_dest",8);put(e,"mod1_depth",100);put(e,"mod1_offset",0);
 for(int k=0;k<4000;k++){
  mod_advance(e,48,120);for(int j=0;j<2;j++)for(int f=0;f<3;f++){double v=mod_chain_read(e,0,j,.0007,f);assert(isfinite(v)&&fabs(v)<=1.000001);}
 }
 put(e,"l0_mute",1);double phase=e->mod_phase[0][0];mod_advance(e,480,120);assert(e->mod_phase[0][0]==phase&&!e->mod_chain_valid[0]);
 put(e,"l0_mute",0);put(e,"mod0_on",0);put(e,"mod1_on",0);phase=e->mod_phase[0][0];mod_advance(e,480,120);assert(e->mod_phase[0][0]==phase);
 done(e);puts("PASS RATE / DEPTH / OFFSET chains preserve recorded bases / bounded feedback / mute-OFF suspension");
}
static void chain_notes_and_cache(void){
 Engine *e=fresh();ns=removed=0;put(e,"mod_pad",1);put(e,"mod0_on",1);put(e,"mod0_dest",7);put(e,"mod0_wave",4);put(e,"mod0_depth",100);put(e,"mod_mix",-100);put(e,"mod1_on",1);put(e,"mod1_dest",8);put(e,"mod1_wave",9);put(e,"mod1_depth",30);
 for(int k=0;k<4000;k++){mod_advance(e,48,120);int budget=EVENTS;assert(span(e,k*.002,(k+1)*.002,120,1+k*.001,INFINITY,&budget));}
 assert(ns>10&&!e->errors);panic_owned(e);assert(e->active_note[0]<0&&e->mod_owner[0]<0);
 mod_preview(e,0);int before[128];memcpy(before,e->mod_trace_cache[0][0],sizeof before);unsigned dirty=e->mod_trace_dirty;
 for(int k=0;k<200;k++)mod_advance(e,48,120);mod_preview(e,0);assert(e->mod_trace_dirty==dirty&&!memcmp(before,e->mod_trace_cache[0][0],sizeof before));
 put(e,"mod1_c0",-80);assert(e->mod_trace_dirty&1);mod_preview(e,0);assert(!(e->mod_trace_dirty&1));
 done(e);puts("PASS chained NOTE ON / threshold / stop release / preview cached across playback");
}
static void coverage_and_stress(void){
 Engine *e=fresh();put(e,"mod_pad",1);put(e,"mod0_on",1);put(e,"mod0_dest",7);put(e,"mod1_on",1);put(e,"mod1_dest",8);
 mod_advance(e,48,120);assert(e->mod_chain_duration[0]<.002);Hit hits[EVENTS];int n=0,budget=EVENTS;
 assert(mod_events(e,hits,&n,0,.04,120,0,&budget));assert(e->mod_chain_duration[0]>=.02);
 assert(mod_chain_read(e,0,0,.015,0)!=mod_chain_read(e,0,0,.001,0));done(e);
 for(int wave=8;wave<11;wave++){
  e=fresh();ns=removed=0;
  for(int s=0;s<4;s++){e->mod_pad[s]=s;for(int j=0;j<2;j++){ModOsc *o=&e->mod[s].osc[j];o->f[0]=1;o->f[1]=wave;o->f[2]=0;o->f[4]=1000;o->f[5]=100;o->f[8]=7;o->f[7]=j*90;}mod_coefficients(e,s);}
  for(int k=0;k<1000;k++){mod_advance(e,48,120);budget=EVENTS;assert(span(e,k*.002,(k+1)*.002,120,1+k*.001,INFINITY,&budget));}
  assert(!e->errors);panic_owned(e);for(int i=0;i<16;i++)assert(e->active_note[i]<0&&e->mod_owner[i]<0);done(e);
 }
 puts("PASS cache covers host-jitter watermark / eight new generators at 20 Hz / shared event budget / stop release");
}
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void benchmark(void){
 Engine *e=fresh();double start=seconds();for(int k=0;k<200000;k++)mod_advance(e,48,120);double off=seconds()-start;
 for(int s=0;s<4;s++){e->mod_pad[s]=s;for(int j=0;j<2;j++){e->mod[s].osc[j].f[0]=1;e->mod[s].osc[j].f[8]=8;e->mod[s].osc[j].f[1]=j?9:8;}mod_coefficients(e,s);}
 start=seconds();for(int k=0;k<20000;k++)mod_advance(e,48,120);double chain=seconds()-start;
 start=seconds();for(int k=0;k<100;k++){e->mod_trace_dirty=15;mod_preview(e,k%4);}double preview=seconds()-start;
 printf("PC benchmark (not Force CPU): OFF %.3f us/block; four reciprocal pairs %.3f us/block; static preview %.3f ms/edit; Engine %zu bytes\n",off*1e6/200000,chain*1e6/20000,preview*1000/100,sizeof *e);done(e);
}
int main(void){shapes();controls_and_state();chains();chain_notes_and_cache();coverage_and_stress();benchmark();puts("ALL MOD PALETTE TESTS PASSED");}

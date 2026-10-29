#define main previous_test_main
#include "mod_engine_test.c"
#undef main
#include "../third_party/mpc-vst-wrapper/wrapper/vst2_wrap.c"
static int rebuilds,automations,max_burst;
static intptr_t host_callback(AEffect *e,int32_t op,int32_t i,intptr_t v,void *p,float f){(void)e;(void)i;(void)v;(void)p;(void)f;if(op==audioMasterUpdateDisplay)rebuilds++;if(op==audioMasterAutomate)automations++;return 0;}
static void step_ui(wrap_t *w){int before=automations;housekeeping(&w->fx,128);int burst=automations-before;if(burst>max_burst)max_burst=burst;}
static int index_of(const char *key){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,key))return i;assert(0);return -1;}
int main(void){Engine *e=fresh();wrap_t *w=calloc(1,sizeof *w);assert(w);w->dsp=e;w->master=host_callback;w->fx.object=w;pthread_mutex_init(&w->lock,NULL);g_api=mpc_engine();memset(w->last_on,-1,sizeof w->last_on);for(int i=0;i<NPARAMS;i++)w->last_pos[i]=-1;changed_all(e);visual_snapshot(e,0,1);for(int i=0;i<500;i++)step_ui(w);rebuilds=automations=max_burst=0;
/* Force different contextual option values and captions between slots. */
put(e,"mod_choose1",1);put(e,"mod0_wave",5);put(e,"mod0_sync",0);put(e,"mod0_dest",7);put(e,"mod_choose0",1);visual_snapshot(e,2048,0);for(int i=0;i<500;i++)step_ui(w);rebuilds=automations=max_burst=0;
for(int slot=0;slot<4;slot++){char key[32];snprintf(key,sizeof key,"mod_choose%d",slot);set_param_now(&w->fx,index_of(key),1);visual_snapshot(e,2048,0);for(int j=0;j<40;j++)step_ui(w);}printf("MOD slot test: rebuilds=%d automate=%d peak_callbacks_per_block=%d\n",rebuilds,automations,max_burst);assert(rebuilds==0&&automations>0&&max_burst<=34);
set_param_now(&w->fx,index_of("mod0_on"),1);set_param_now(&w->fx,index_of("mod0_phase"),.5);visual_snapshot(e,2048,0);for(int j=0;j<40;j++)step_ui(w);assert(!rebuilds);
for(int pick=0;pick<8;pick++){char key[32];snprintf(key,sizeof key,"open_sound%d",pick);set_param_now(&w->fx,index_of(key),1);for(int j=0;j<30;j++)step_ui(w);set_param_now(&w->fx,index_of("map_sound"),1.f/7);visual_snapshot(e,2048,0);for(int j=0;j<30;j++)step_ui(w);}assert(!rebuilds);
 /* A full burst retains every latest value, including the highest index. */
 for(int i=0;i<NPARAMS;i++)w->cc_changed[i]=1;
 for(int j=0;j<200;j++)step_ui(w);
 for(int i=0;i<NPARAMS;i++)assert(!w->cc_changed[i]);
 assert(max_burst<=34);
 pthread_mutex_destroy(&w->lock);free(w);done(e);puts("PASS MOD native notifications without host screen reconstruction");return 0;}

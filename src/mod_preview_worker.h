/* A single bounded mailbox. The audio/UI owner never waits for the worker.
   Only immutable MOD configuration crosses threads; no MIDI or host calls. */
typedef struct {
 _Atomic int state,stop;
 _Atomic unsigned completed;
 pthread_t thread;
 Engine *scratch;
 ModLane config;
 double bpm,hz[2],depth[2],offset[2],requested_at;
 int slot,trace[2][128],style[2];
} ModPreviewWorker;
static double mod_preview_clock(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void *mod_preview_thread(void *arg){
 ModPreviewWorker *w=arg;prctl(PR_SET_NAME,"PiCN Preview",0,0,0);
 const struct timespec pause={0,5000000};
 while(!atomic_load_explicit(&w->stop,memory_order_relaxed)){
  if(atomic_load_explicit(&w->state,memory_order_acquire)!=1){nanosleep(&pause,NULL);continue;}
  atomic_store_explicit(&w->state,2,memory_order_relaxed);
  Engine *p=w->scratch;p->mod[0]=w->config;p->mod_selected=0;p->mod_bpm=w->bpm;p->mod_trace_dirty=1;
  for(int j=0;j<2;j++){p->mod_hz[0][j]=w->hz[j];p->mod_depth[0][j]=w->depth[j];p->mod_offset[0][j]=w->offset[j];}
  mod_preview_compute(p,0);
  memcpy(w->trace,p->mod_trace_cache[0],sizeof w->trace);
  for(int j=0;j<2;j++)w->style[j]=mod_style(p,j);
  atomic_fetch_add_explicit(&w->completed,1,memory_order_relaxed);
  atomic_store_explicit(&w->state,3,memory_order_release);
 }
 return NULL;
}
static int mod_preview_start(Engine *e){
 e->mod_preview_async=1;
 ModPreviewWorker *w=calloc(1,sizeof *w);if(!w)return 0;
 w->scratch=calloc(1,sizeof *w->scratch);if(!w->scratch){free(w);return 0;}
 atomic_init(&w->state,0);atomic_init(&w->stop,0);atomic_init(&w->completed,0);
 pthread_attr_t attr;int initialized=pthread_attr_init(&attr)==0;
 int ok=initialized&&pthread_attr_setstacksize(&attr,128*1024)==0;
 int error=ok?pthread_create(&w->thread,&attr,mod_preview_thread,w):-1;
 if(initialized)pthread_attr_destroy(&attr);
 if(error){free(w->scratch);free(w);return 0;}
 e->mod_preview_worker=w;return 1;
}
static void mod_preview_stop(Engine *e){
 ModPreviewWorker *w=e->mod_preview_worker;if(!w)return;
 atomic_store_explicit(&w->stop,1,memory_order_relaxed);pthread_join(w->thread,NULL);
 free(w->scratch);free(w);e->mod_preview_worker=NULL;
}
static void mod_preview_async_poll(Engine *e,int s){
 ModPreviewWorker *w=e->mod_preview_worker;if(!w)return; /* MIDI remains functional on allocation failure. */
 int state=atomic_load_explicit(&w->state,memory_order_acquire);
 if(state==3){
  int slot=w->slot;
  if(w->bpm==e->mod_bpm&&!memcmp(&w->config,&e->mod[slot],sizeof w->config)){
   memcpy(e->mod_trace_cache[slot],w->trace,sizeof w->trace);
   memcpy(e->mod_published_style[slot],w->style,sizeof w->style);
   e->mod_trace_dirty&=~(1u<<slot);e->revision++;
  }
  atomic_store_explicit(&w->state,0,memory_order_release);state=0;
 }
 if(state!=0||!(e->mod_trace_dirty&(1u<<s)))return;
 double now=mod_preview_clock();if(now-w->requested_at<.1)return;
 w->requested_at=now;w->slot=s;w->config=e->mod[s];w->bpm=e->mod_bpm;
 for(int j=0;j<2;j++){w->hz[j]=e->mod_hz[s][j];w->depth[j]=e->mod_depth[s][j];w->offset[j]=e->mod_offset[s][j];}
 atomic_store_explicit(&w->state,1,memory_order_release);
}

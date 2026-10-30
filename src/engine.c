/* piClideaN MPC -- independent MIDI-only Euclidean engine. Copyright 2026 Timo. MIT. */
#include "engine.h"
#include "drum_notes.h"
#include <alsa/asoundlib.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <sys/prctl.h>
#if defined(__arm__) && defined(__GLIBC__)
__asm__(".symver pthread_create,pthread_create@GLIBC_2.4");
__asm__(".symver pthread_join,pthread_join@GLIBC_2.4");
__asm__(".symver pthread_attr_init,pthread_attr_init@GLIBC_2.4");
__asm__(".symver pthread_attr_destroy,pthread_attr_destroy@GLIBC_2.4");
__asm__(".symver pthread_attr_setstacksize,pthread_attr_setstacksize@GLIBC_2.4");
#endif
#define LANES 16
#define NF 14
#define EVENTS 512
#define EXTRA_PARAMS (14+LANES*4+LANES*32*5+8)
#define LEGACY_CHANGED (LANES*NF+19)
#define SELECT16_ID (LEGACY_CHANGED+EXTRA_PARAMS)
#define HOLD_ID (SELECT16_ID+1)
#define RAND_ID (HOLD_ID+LANES)
#define MOD_ID (RAND_ID+9)
#include "mod_types.h"
#define CHANGED_COUNT (MOD_ID+MOD_PARAMS)
#define VIS_LANE 288
#define VIS_PREVIEW (VIS_LANE+LANES)
#define VIS_CELL (VIS_PREVIEW+32)
#define VIS_PHASE (VIS_CELL+32)
#define VIS_CUSTOM (VIS_PHASE+2)
#define VIS_POSITION (VIS_CUSTOM+4)
#define VIS_TRACK (VIS_POSITION+1)
#define VIS_FLASH (VIS_TRACK+LANES*32)
#define VIS_MOD (VIS_FLASH+LANES)
#define VISUALS (VIS_MOD+MOD_VIS_COUNT)
#define FLASH_EVENTS 64
#define FLASH_SECONDS 0.140
static const char *field_names[NF]={"steps","hits","rotate","division","velocity","humanize","prob","gate","inote","dnote","pad","sound","mute","solo"};
static const int mins[NF]={1,0,0,0,1,0,0,10,0,0,0,0,0,0};
static const int maxs[NF]={32,32,31,4,127,50,100,95,127,127,15,11,1,1};
static const char *sounds[]={"KICK","MBRANE","CL HAT","OP HAT","CLAP","RIM","CRASH","RIDE","X1","X2","FM","CUSTOM"};
static const double step_beats[]={0.125,0.25,0.5,1.,2.};
static _Atomic unsigned slots;
typedef struct {int f[NF];uint32_t pattern;int values[32][5];int playmode,auto_rotate,shift,every,hold;unsigned seed;double origin;} Lane;
typedef struct {int line,source,repeat,velocity;double beat,duration;int kind;} Hit;
typedef struct {double requested;int actual,line,step,view,lo,hi;} QMove;
typedef struct {
 void *mod_preview_worker;unsigned mod_preview_calculations;int mod_preview_async;int mod_published_style[MOD_SLOTS][2];
 ModLane mod[MOD_SLOTS];int mod_pad[MOD_SLOTS],mod_selected;double mod_phase[MOD_SLOTS][2],mod_span_offset;
 /* Bounded control cache only for chained slots; old unchained path is intact. */
 ModWalk mod_walk[MOD_SLOTS][2];
 double mod_chain_cache[MOD_SLOTS][2][MOD_CHAIN_POINTS][3];
 double mod_chain_duration[MOD_SLOTS],mod_chain_source[MOD_SLOTS][2];
 int mod_chain_count[MOD_SLOTS],mod_chain_valid[MOD_SLOTS],mod_chain_dense[MOD_SLOTS][2];
 double mod_hz[MOD_SLOTS][2],mod_depth[MOD_SLOTS][2],mod_offset[MOD_SLOTS][2],mod_weight[MOD_SLOTS][2];
 int mod_trace_cache[MOD_SLOTS][2][128];unsigned mod_trace_dirty;int mod_running_cache[MOD_SLOTS][2];double mod_seconds,mod_beats,mod_bpm;
 int mod_owner[LANES],mod_high[LANES][2],mod_armed[LANES][2];
 double mod_latch_tick[LANES];int mod_latch[LANES][6],mod_latch_valid[LANES];
 Lane lane[LANES];int selected,mode,channel,bank,step,rhythm,dialog,row,panic;int scale_choice,scale_start,scale_applied_choice,scale_applied_start;unsigned scale_ui_pending;int mod_pick;unsigned ui_actions;
 int edit_view,edit_tool,state_valid;
 int rand_mode,rand_line;unsigned rand_mask;
 QMove qmove[8];unsigned q_valid;
 unsigned revision,random,errors;int slot;
 snd_seq_t *seq;int port,queue;snd_seq_remove_events_t *rem;
 int active_note[LANES],active_ch[LANES];
 double active_off[LANES],queue_now,queue_end,expected_ppq,last_host_ppq;
 /* Visual-only bounded rings; no influence on note scheduling or errors. */
 double flash_at[LANES][FLASH_EVENTS],flash_until[LANES];
 unsigned flash_read[LANES],flash_count[LANES];
 unsigned release_pending; snd_seq_queue_status_t *queue_status;
 double sample_rate,cycle_a,cycle_b;int cycle,playing,valid;long head[LANES];double display_tick[LANES],reset_at[LANES];
 int changed[CHANGED_COUNT],changes,change_read,change_write;unsigned char queued[CHANGED_COUNT];unsigned visual_revision;
 unsigned char visual_dirty[VISUALS]; int visual_last[VISUALS],visual_ready,visual_frames,visual_cursor;int ui_offset[LANES];
 double last_changed[CHANGED_COUNT];unsigned char have_changed[CHANGED_COUNT]; /* bounded numeric identifiers, no allocation during callback */
} Engine;
static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}
static unsigned rng(Engine *e){unsigned x=e->random;x^=x<<13;x^=x>>17;x^=x<<5;return e->random=x?x:0x67419321u;}
static int integer(const char *s,int *out){char *end;errno=0;double n=strtod(s,&end);if(errno||end==s||*end||!isfinite(n)||n<INT_MIN||n>INT_MAX)return 0;*out=(int)lround(n);return 1;}
static void pattern(Lane *l){int n=l->f[0],k=l->f[1],r=l->f[2];uint32_t bits=0;for(int i=0;i<n;i++){int j=(i-r+n)%n;if(k&&(j*k)%n<k)bits|=UINT32_C(1)<<i;}l->pattern=bits;}
static void sanitize(Lane *l,int geometry){for(int j=0;j<NF;j++)l->f[j]=clampi(l->f[j],mins[j],maxs[j]);l->f[1]=clampi(l->f[1],0,l->f[0]);l->f[2]%=l->f[0];if(geometry)pattern(l);}
static int audible(Engine *e,int i){int any=0;for(int j=0;j<LANES;j++)any|=e->lane[j].f[13];return !e->lane[i].f[12]&&(!any||e->lane[i].f[13]);}
/* CONFIG has fixed lane labels; preserve the previously audible note on load. */
static void normalize_config(Engine *e){for(int i=0;i<LANES;i++){int n=e->lane[i].f[e->mode?9:8];e->lane[i].f[8]=e->lane[i].f[9]=n;e->lane[i].f[10]=i;e->lane[i].f[11]=11;}e->mode=1;e->bank=0;e->channel=0;}
static int note(Engine *e,int i){return e->lane[i].f[9];}
static int duplicates(Engine *e){for(int i=0;i<LANES;i++)for(int j=i+1;j<LANES;j++)if(note(e,i)==note(e,j))return 1;return 0;}
/* Coalesced bounded queue: UI gestures publish their actual dependencies only. */
static void notify_id(Engine *e,int id){if(id<0||id>=CHANGED_COUNT||e->queued[id])return;e->queued[id]=1;e->changed[e->change_write]=id;e->change_write=(e->change_write+1)%CHANGED_COUNT;e->changes++;}
static void changed_lane(Engine *e,int line);
/* Transient UI selection: never serialized, never a MIDI scheduling input. */
static void rand_clear(Engine *e){e->rand_mode=0;e->rand_mask=0;e->q_valid=0;for(int j=0;j<9;j++)notify_id(e,RAND_ID+j);}
static void rand_press(Engine *e){
 if(e->edit_view){rand_clear(e);return;}
 if(!e->rand_mode){e->rand_mode=1;e->rand_line=e->selected;e->rand_mask=0;e->q_valid=0;for(int j=0;j<9;j++)notify_id(e,RAND_ID+j);return;}
 unsigned mask=e->rand_mask;
 if(e->rand_line!=e->selected||!mask){rand_clear(e);return;}
 Lane *l=&e->lane[e->selected];
 static const int fields[8]={4,6,5,7,0,1,2,3};
 int before[4];memcpy(before,l->f,sizeof before);
 /* Preserve unselected HITS/ROTATE by constraining a new length. */
 if(mask&(1u<<4)){
  int lo=1;if(!(mask&(1u<<5))&&l->f[1]>lo)lo=l->f[1];
  if(!(mask&(1u<<6))&&l->f[2]+1>lo)lo=l->f[2]+1;
  l->f[0]=lo+(int)(rng(e)%(unsigned)(33-lo));
 }
 for(int q=0;q<8;q++)if(q!=4&&(mask&(1u<<q))){
  int f=fields[q],lo=mins[f],hi=f==1?l->f[0]:f==2?l->f[0]-1:maxs[f];
  l->f[f]=lo+(int)(rng(e)%(unsigned)(hi-lo+1));
 }
 int geometry=memcmp(before,l->f,3*sizeof(int))!=0;
 if(geometry){
  for(int k=0;k<32;k++){l->values[k][0]=-1;notify_id(e,LEGACY_CHANGED+14+LANES*4+e->selected*160+k*5);}
  pattern(l);
 }
 if(memcmp(before,l->f,sizeof before))e->release_pending|=1u<<e->selected;
 e->step=clampi(e->step,0,l->f[0]-1);
 rand_clear(e);changed_lane(e,e->selected);
}
static void mod_notify_selected(Engine *e);
/* PATTERN selection is independent of MOD: do not invalidate every visual readout. */
static void changed_selected(Engine *e){notify_id(e,SELECT16_ID);for(int id=LANES*NF;id<LEGACY_CHANGED+14;id++)notify_id(e,id);for(int q=0;q<8;q++)notify_id(e,LEGACY_CHANGED+14+LANES*4+LANES*160+q);e->revision++;}
static void changed_lane(Engine *e,int line){for(int f=0;f<NF;f++)notify_id(e,line*NF+f);for(int f=0;f<4;f++)notify_id(e,LEGACY_CHANGED+14+line*4+f);changed_selected(e);}
static void changed_step(Engine *e,int line,int step,int field){notify_id(e,LEGACY_CHANGED+14+LANES*4+line*160+step*5+field);changed_selected(e);}
static void changed_dialog(Engine *e){notify_id(e,LANES*NF+15);notify_id(e,LANES*NF+17);notify_id(e,LANES*NF+18);e->revision++;}
static void changed_all(Engine *e){for(int id=0;id<CHANGED_COUNT;id++)notify_id(e,id);e->revision++;}
/* Nonblocking ALSA work is bounded; failures retain ownership for later release. */
static int cancel_tag(Engine *e,int lane){
 if(!e->seq||!e->rem)return 0;
 snd_seq_remove_events_set_queue(e->rem,e->queue);
 snd_seq_remove_events_set_condition(e->rem,SND_SEQ_REMOVE_OUTPUT|SND_SEQ_REMOVE_TAG_MATCH);
 snd_seq_remove_events_set_tag(e->rem,lane+1);
 if(snd_seq_remove_events(e->seq,e->rem)<0){e->errors++;return 0;}return 1;
}
static int send_event(Engine *e,int line,int on,int n,int ch,int vel,double at){
 if(!e->seq)return 0;snd_seq_event_t v;snd_seq_ev_clear(&v);snd_seq_ev_set_source(&v,e->port);snd_seq_ev_set_subs(&v);v.tag=(unsigned char)(line+1);
 if(on)snd_seq_ev_set_noteon(&v,ch,n,vel);else snd_seq_ev_set_noteoff(&v,ch,n,0);
 if(at>=0){
  if(!isfinite(at)||at>UINT_MAX-1){e->errors++;return 0;}
  snd_seq_real_time_t t;t.tv_sec=(unsigned)at;t.tv_nsec=(unsigned)((at-t.tv_sec)*1e9);
  snd_seq_ev_schedule_real(&v,e->queue,0,&t);
 }else snd_seq_ev_set_direct(&v);
 if(snd_seq_event_output_direct(e->seq,&v)<0){e->errors++;return 0;}return 1;
}
static void flash_enqueue(Engine *e,int line,double at){
 unsigned count=e->flash_count[line],read=e->flash_read[line];
 /* Adjacent sub-hits can share one visible impulse. Overflow drops visuals only. */
 if(count&&at-e->flash_at[line][(read+count-1)%FLASH_EVENTS]<1./120.)return;
 if(count>=FLASH_EVENTS)return;
 e->flash_at[line][(read+count)%FLASH_EVENTS]=at;e->flash_count[line]++;
}
static int flash_current(Engine *e,int line){
 while(e->flash_count[line]){
  double at=e->flash_at[line][e->flash_read[line]];if(at>e->queue_now)break;
  e->flash_read[line]=(e->flash_read[line]+1)%FLASH_EVENTS;e->flash_count[line]--;
  if(at+FLASH_SECONDS>e->flash_until[line])e->flash_until[line]=at+FLASH_SECONDS;
 }
 return e->playing&&audible(e,line)&&e->queue_now<e->flash_until[line];
}
static int release(Engine *e,int i){
 e->flash_count[i]=e->flash_read[i]=0;e->flash_until[i]=0;
 e->mod_owner[i]=-1;for(int j=0;j<2;j++)e->mod_armed[i][j]=0;
 if(!e->seq){e->active_note[i]=-1;e->release_pending&=~(1u<<i);return 1;}
 int cancelled=cancel_tag(e,i),off=e->active_note[i]<0||send_event(e,i,0,e->active_note[i],e->active_ch[i],0,-1);
 if(cancelled&&off){e->active_note[i]=-1;e->release_pending&=~(1u<<i);return 1;}
 e->release_pending|=1u<<i;return 0;
}
static void panic_owned(Engine *e){memset(e->mod_latch_valid,0,sizeof e->mod_latch_valid);for(int i=0;i<LANES;i++)release(e,i);e->panic=0;}
static int parse_suffix(const char *,const char *,int,int *);
static int parse_lane(const char *,int *,int *);
static int lane_prefix(const char *,int *,const char **);
static void set_param(void *,const char *,const char *);
static int get_param(void *,const char *,char *,int);
static void visual_key(int,char *,int);
static void mod_init(Engine *e);
static void mod_defaults(ModLane *m);
static void mod_coefficients(Engine *e,int s);
static const int mod_lo[MOD_FIELDS],mod_hi[MOD_FIELDS];
static int scale_notes(int,int,int [LANES]);
#include "evolution.h"
#include "mod_lfo.h"
/* HOLD uses the same queue and timestamps as GATE; no additional MIDI path.
   Each effective retrigger releases this lane first. Other lanes retain ownership. */
static void trigger(Engine *e,const Hit *h,double bpm,double at,double max_duration){
 int i=h->line;if(mod_pattern_blocked(e,i))return;if(!audible(e,i)||(e->release_pending&(1u<<i)))return;int n=note(e,i),hold=e->lane[i].hold;
 for(int k=0;k<LANES;k++){
  if(k==i&&hold)continue;
  if(e->active_note[k]==n&&e->active_ch[k]==e->channel&&((e->release_pending&(1u<<k))||e->active_off[k]>at+1e-7)){e->errors++;return;}
 }
 double dur=fmin(h->duration*60./bpm,max_duration);if(!(dur>0))return;
 if(hold&&e->active_note[i]>=0){
  if(!send_event(e,i,0,e->active_note[i],e->active_ch[i],0,at)){
   e->release_pending|=1u<<i;release(e,i);return;
  }
 }
 if(!send_event(e,i,1,n,e->channel,h->velocity,at)){
  if(hold){e->release_pending|=1u<<i;release(e,i);}return;
 }
 e->active_note[i]=n;e->active_ch[i]=e->channel;e->active_off[i]=hold?INFINITY:at+dur;
 if(!hold&&!send_event(e,i,0,n,e->channel,0,at+dur)){e->release_pending|=1u<<i;release(e,i);}
 if(e->active_note[i]>=0&&!(e->release_pending&(1u<<i)))flash_enqueue(e,i,at);
}
static void *create(const char *dir){(void)dir;Engine *e=calloc(1,sizeof *e);if(!e)return NULL;e->port=e->queue=e->slot=-1;e->sample_rate=44100;e->random=0xa8e17f21u;e->valid=0;e->mode=1;e->edit_view=1;e->state_valid=1;e->scale_start=e->scale_applied_start=36;
 for(int i=0;i<32;i++){unsigned mask=1u<<i,old=atomic_load(&slots);while(!(old&mask)){if(atomic_compare_exchange_weak(&slots,&old,old|mask)){e->slot=i;break;}}if(e->slot>=0)break;}
 if(e->slot<0){free(e);return NULL;}e->random^=(unsigned)e->slot*0x9e3779b9u;
 mod_init(e);
 for(int i=0;i<LANES;i++){init_lane(&e->lane[i],i);e->active_note[i]=-1;e->head[i]=-1;e->reset_at[i]=-1;}

 if(snd_seq_open(&e->seq,"default",SND_SEQ_OPEN_OUTPUT,SND_SEQ_NONBLOCK)>=0){char name[64];snprintf(name,sizeof name,"piClideaN MPC%s%d",e->slot?" ":"",e->slot?e->slot+1:1);if(e->slot==0)strcpy(name,"piClideaN MPC");snd_seq_set_client_name(e->seq,name);e->port=snd_seq_create_simple_port(e->seq,"Out",SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_APPLICATION);char qname[80];snprintf(qname,sizeof qname,"%s Clock",name);e->queue=snd_seq_alloc_named_queue(e->seq,qname);
 if(snd_seq_set_client_pool_output(e->seq,4096)<0)e->errors++;
 if(e->port>=0&&e->queue>=0&&snd_seq_remove_events_malloc(&e->rem)>=0&&snd_seq_queue_status_malloc(&e->queue_status)>=0){if(snd_seq_start_queue(e->seq,e->queue,NULL)<0||snd_seq_drain_output(e->seq)<0){snd_seq_close(e->seq);e->seq=NULL;e->errors++;}}else{snd_seq_close(e->seq);e->seq=NULL;e->errors++;}}
 else{e->seq=NULL;e->errors++;}mod_preview_start(e);changed_all(e);return e;}
static void destroy(void *inst){Engine *e=inst;if(!e)return;mod_preview_stop(e);panic_owned(e);for(int retry=0;retry<2&&e->release_pending;retry++)for(int i=0;i<LANES;i++)if(e->release_pending&(1u<<i))release(e,i);if(e->seq){if(e->queue>=0){snd_seq_stop_queue(e->seq,e->queue,NULL);snd_seq_free_queue(e->seq,e->queue);}snd_seq_close(e->seq);}if(e->rem)snd_seq_remove_events_free(e->rem);if(e->queue_status)snd_seq_queue_status_free(e->queue_status);if(e->slot>=0)atomic_fetch_and(&slots,~(1u<<e->slot));free(e);}
static void midi(void *inst,const uint8_t *msg,int len){(void)inst;(void)msg;(void)len;/* no MIDI echo, no second note path */}
/* Parse a complete lane index: l9 and l10 must never alias. */
static int lane_prefix(const char *key,int *line,const char **suffix){
 if(key[0]!='l'||key[1]<'0'||key[1]>'9')return 0;
 const char *p=key+1;int n=0,digits=0;
 while(*p>='0'&&*p<='9'){if(++digits>2)return 0;n=n*10+*p++-'0';}
 if(*p!='_'||n>=LANES)return 0;*line=n;*suffix=p+1;return 1;
}
static int parse_lane(const char *key,int *li,int *fi){
 const char *suffix;if(!lane_prefix(key,li,&suffix))return 0;
 for(int j=0;j<NF;j++)if(!strcmp(suffix,field_names[j])){*fi=j;return 1;}return 0;
}
static int parse_suffix(const char *key,const char *prefix,int limit,int *n){size_t z=strlen(prefix);if(strncmp(key,prefix,z))return 0;char *end;long v=strtol(key+z,&end,10);if(end==key+z||*end||v<0||v>=limit)return 0;*n=(int)v;return 1;}
static int read_state(Engine *e,const char *v){
 if(strncmp(v,"PICN1 ",6))return 0;const char *p=v+6;int nums[8+8*NF];for(unsigned i=0;i<sizeof nums/sizeof nums[0];i++){char *end;errno=0;long n=strtol(p,&end,10);if(errno||end==p||n<INT_MIN||n>INT_MAX)return 0;nums[i]=(int)n;p=end;while(*p==' ')p++;}if(strcmp(p,"END"))return 0;
 if(nums[0]<0||nums[0]>7||nums[1]<0||nums[1]>1||nums[2]<0||nums[2]>15||nums[3]<0||nums[3]>7||nums[4]<0||nums[4]>31||nums[5]<0||nums[5]>5||nums[6]<0||nums[6]>7||nums[7]!=1)return 0;
 Lane lanes[LANES];for(int i=0;i<LANES;i++)init_lane(&lanes[i],i);for(int i=0;i<8;i++){for(int j=0;j<NF;j++){int n=nums[8+i*NF+j];if(n<mins[j]||n>maxs[j])return 0;lanes[i].f[j]=n;}if(lanes[i].f[1]>lanes[i].f[0]||lanes[i].f[2]>=lanes[i].f[0])return 0;pattern(&lanes[i]);for(int j=0;j<32;j++)for(int k=0;k<5;k++)lanes[i].values[j][k]=-1;lanes[i].playmode=lanes[i].auto_rotate=0;lanes[i].shift=lanes[i].every=1;lanes[i].origin=0;}
 for(int i=8;i<LANES;i++)lanes[i].f[9]=drum_notes[nums[3]*16+i];
 e->selected=nums[0];e->mode=nums[1];e->channel=0;e->bank=nums[3];e->step=nums[4];e->rhythm=nums[5];e->row=nums[6];memcpy(e->lane,lanes,sizeof lanes);normalize_config(e);e->dialog=0;e->panic=1;e->valid=0;mod_init(e);changed_all(e);return 1;
}
/* CONFIG-only scale preparation. Retired UI handles reused; no index added.
   Prepared choices are transient; the sixteen resulting notes are project state. */
static const unsigned short scale_masks[16]={0,0xAB5,0x5AD,0x9AD,0xAAD,0x295,0x4A9,0x4E9,0xFFF,0xAB5,0x6AD,0x5AB,0xAD5,0x6B5,0x5AD,0x56B};
static int note_display(int n,char *b,int size){static const char *names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};return snprintf(b,size,"%d · %s%d",n,names[n%12],n/12-2);}
static int scale_notes(int choice,int start,int out[LANES]){if(choice<0||choice>=16||start<0||start>127)return 0;if(!choice){for(int i=0;i<LANES;i++)out[i]=36+i;return 1;}int count=0;for(int d=0;start+d<=127&&count<LANES;d++)if(scale_masks[choice]&(1u<<(d%12)))out[count++]=start+d;return count==LANES;}
static int scale_pending(const Engine *e){return (e->scale_choice!=e->scale_applied_choice?1:0)|(e->scale_start!=e->scale_applied_start?2:0);}
static void scale_apply(Engine *e,int reset){int notes[LANES];if(!scale_notes(reset?0:e->scale_choice,e->scale_start,notes))return;for(int i=0;i<LANES;i++){char key[24],value[12];snprintf(key,sizeof key,"l%d_dnote",i);snprintf(value,sizeof value,"%d",notes[i]);set_param(e,key,value);}if(reset){e->scale_choice=0;e->scale_start=36;e->scale_ui_pending|=3;}e->scale_applied_choice=e->scale_choice;e->scale_applied_start=e->scale_start;e->scale_ui_pending|=4;}
/* Explicitly confirmed global reset; musical changes are bounded to 16 lanes. */
static void random_all(Engine *e){
 int notes[LANES],mutes[LANES];for(int i=0;i<LANES;i++){notes[i]=note(e,i);mutes[i]=e->lane[i].f[12];}
 e->panic=1;e->release_pending=(1u<<LANES)-1;rand_clear(e);
 for(int i=0;i<LANES;i++){Lane *l=&e->lane[i];for(int k=0;k<32;k++)for(int f=0;f<5;f++)if(l->values[k][f]>=0)notify_id(e,LEGACY_CHANGED+14+LANES*4+i*160+k*5+f);init_lane(l,i);l->f[12]=mutes[i];l->f[8]=l->f[9]=notes[i];l->f[0]=1+rng(e)%32;l->f[1]=1+rng(e)%l->f[0];l->f[2]=rng(e)%l->f[0];l->f[3]=rng(e)%5;l->f[4]=100;l->f[5]=0;l->f[6]=100;l->f[7]=50;l->hold=0;pattern(l);changed_lane(e,i);notify_id(e,HOLD_ID+i);e->head[i]=-1;e->display_tick[i]=0;e->ui_offset[i]=0;e->reset_at[i]=-1;
 }
 mod_init(e);mod_notify_selected(e);e->step=0;e->scale_choice=e->scale_applied_choice;e->scale_start=e->scale_applied_start;e->scale_ui_pending=7;e->dialog=0;changed_dialog(e);
}
static const char *mod_pick_key(Engine *e){static const char *keys[]={"mod0_wave","mod0_sync","mod0_rate","mod0_dest","mod1_wave","mod1_sync","mod1_rate","mod1_dest"};return keys[clampi(e->mod_pick,0,7)];}
static void set_param(void *inst,const char *key,const char *val){Engine *e=inst;if(!strcmp(key,"state")){e->state_valid=(!strncmp(val,"PICN2 ",6)||!strncmp(val,"PICN3 ",6)||!strncmp(val,"PICN4 ",6)||!strncmp(val,"PICN5 ",6)||!strncmp(val,"PICN6 ",6)||!strncmp(val,"PICN7 ",6))?read_evolution(e,val):read_state(e,val);if(e->state_valid){rand_clear(e);e->q_valid=0;e->visual_ready=0;e->playing=0;for(int k=0;k<LANES;k++){e->reset_at[k]=-1;e->head[k]=-1;e->display_tick[k]=0;e->ui_offset[k]=0;}}return;}if(e->rand_mode&&key[0]=='q'&&key[1]>='0'&&key[1]<='7'&&!key[2])return;if(q_set(e,key,val))return;int v;if(!integer(val,&v))return;int i,j;
 if(!strcmp(key,"open_bank")){if(v>0){e->dialog=3;changed_dialog(e);}return;}
 if(e->dialog==3&&!strcmp(key,"map_sound")){if(v>=0&&v<5){char value[8];snprintf(value,sizeof value,"%d",v);set_param(e,"play_mode",value);e->dialog=0;changed_dialog(e);}return;}
 if(!strcmp(key,"open_target")){if(v>0){rand_clear(e);e->dialog=4;changed_dialog(e);}return;}
 if(e->dialog==4&&!strcmp(key,"apply_rhythm")){if(v>0)random_all(e);return;}
 if(parse_suffix(key,"open_sound",8,&i)){if(v>0){e->mod_pick=i;e->ui_actions|=1;e->dialog=2;changed_dialog(e);}return;}
 if(!strcmp(key,"can_undo"))return; /* read-only MOD choice context */
 if(e->dialog==2&&!strcmp(key,"map_sound")){static const int highs[]={10,1,6,10};if(v<0||v>highs[e->mod_pick%4])return;mod_set(e,mod_pick_key(e),v);e->dialog=0;changed_dialog(e);return;}
 if(!strcmp(key,"close")&&(e->dialog==2||e->dialog==3||e->dialog==4)){if(v>0){e->dialog=0;changed_dialog(e);}return;}

 if(!strcmp(key,"choose_target"))return; /* read-only pending state */
 if(!strcmp(key,"choose_bank")){if(v>0&&e->dialog!=5){e->dialog=5;changed_dialog(e);}return;}
 if(!strcmp(key,"close")&&e->dialog==5){if(v>0){e->dialog=0;changed_dialog(e);}return;}
 if(!strcmp(key,"choose_sound")){int before=scale_pending(e);e->scale_choice=clampi(v,0,15);if(before!=scale_pending(e))e->scale_ui_pending|=4;if(e->dialog==5){e->dialog=0;changed_dialog(e);}return;}
 if(!strcmp(key,"edit_row")){int before=scale_pending(e);e->scale_start=clampi(v,0,127);if(before!=scale_pending(e))e->scale_ui_pending|=4;return;}
 if(!strcmp(key,"apply_mapping")){if(v>0)scale_apply(e,0);return;}
 if(!strcmp(key,"open_mapping")){if(v>0)scale_apply(e,1);return;}

 if(!strcmp(key,"open_rhythm")){if(v>0&&e->dialog!=6){e->dialog=6;changed_dialog(e);}return;}
 if(e->dialog==6&&!strcmp(key,"mod_pad")){int value=clampi(v,0,16);mod_set(e,"mod_pad",value);if(e->mod_pad[e->mod_selected]==value-1){e->dialog=0;changed_dialog(e);}return;}
 if(e->dialog==6&&!strcmp(key,"apply_rhythm")){if(v>0){mod_set(e,"mod_pad",0);e->dialog=0;changed_dialog(e);}return;}
 if(mod_set(e,key,v))return;
 if(!strcmp(key,"rand_mode")||!strncmp(key,"rand_pick",9))return; /* read-only host echoes */
 /* Reject stale touch/data-wheel writes to the read-only LINE row in EDIT.
    Only selection aliases are guarded; lN_* host automation is unchanged. */
 if(e->edit_view&&(!strcmp(key,"sel_velocity")||!strcmp(key,"sel_prob")||!strcmp(key,"sel_humanize")||!strcmp(key,"sel_gate")))return;
 if(parse_suffix(key,"rand_toggle",8,&i)){if(v>0&&e->rand_mode&&e->rand_line==e->selected){e->rand_mask^=1u<<i;notify_id(e,RAND_ID+1+i);}return;}
 if(!strcmp(key,"randomize")){if(v>0)rand_press(e);return;}
 if(e->rand_mode&&(!strcmp(key,"half")||!strcmp(key,"double")||!strncmp(key,"sel_",4)))return;
 if(evolution_set(e,key,v))return;
 if(!strcmp(key,"line")||!strcmp(key,"line_select")||!strcmp(key,"line_select16")){int top=!strcmp(key,"line_select16")?LANES-1:7;if(e->selected==clampi(v,0,top))return;rand_clear(e);e->q_valid=0;e->selected=clampi(v,0,top);e->step=clampi(e->step,0,e->lane[e->selected].f[0]-1);e->dialog=0;changed_selected(e);return;}
 if(!strcmp(key,"target_mode")||!strcmp(key,"choose_target")||!strcmp(key,"choose_bank")||!strcmp(key,"choose_sound")||!strcmp(key,"map_sound")||!strcmp(key,"bank")||!strcmp(key,"open_mapping")||!strcmp(key,"open_target")||!strcmp(key,"open_bank")||!strcmp(key,"apply_mapping"))return; /* retired CONFIG selectors */
 if(!strcmp(key,"channel")){if(e->channel){e->panic=1;e->channel=0;changed_selected(e);}return;}
 if(!strcmp(key,"selected_step")){if(e->step!=clampi(v,0,e->lane[e->selected].f[0]-1))e->q_valid=0;e->step=clampi(v,0,e->lane[e->selected].f[0]-1);changed_selected(e);return;}
 if(!strcmp(key,"rhythm_choice")){if(e->rhythm==clampi(v,0,5))return;e->rhythm=clampi(v,0,5);changed_selected(e);return;}
 if(!strcmp(key,"edit_row")){e->row=clampi(v,0,LANES-1);return;}
 if(!strncmp(key,"sel_",4)){for(j=0;j<NF;j++)if(!strcmp(key+4,field_names[j])){Lane *l=&e->lane[e->selected];int accepted=clampi(v,mins[j],j==2?l->f[0]-1:maxs[j]);if(l->f[j]==accepted)return;l->f[j]=accepted;sanitize(l,j<3);if(j<4)e->release_pending|=1u<<e->selected;e->step=clampi(e->step,0,l->f[0]-1);changed_lane(e,e->selected);return;}}
 if(parse_lane(key,&i,&j)){if(j==10||j==11)return;if(j==8)j=9;int accepted=clampi(v,mins[j],j==2?e->lane[i].f[0]-1:maxs[j]);if(e->lane[i].f[j]==accepted)return;e->lane[i].f[j]=accepted;if(j==9)e->lane[i].f[8]=accepted;if(j>=8&&j<=11)e->release_pending|=1u<<i;sanitize(&e->lane[i],j<3);if(j<4)e->release_pending|=1u<<i;if(i==e->selected)e->step=clampi(e->step,0,e->lane[i].f[0]-1);changed_lane(e,i);return;}
 /* Momentary actions fire only on the press. The wrapper's release must be a no-op. */
 if(v<=0)return;
 if(parse_suffix(key,"step_press",32,&i)){if(i>=e->lane[e->selected].f[0])return;int source=(i-e->ui_offset[e->selected]%e->lane[e->selected].f[0]+e->lane[e->selected].f[0])%e->lane[e->selected].f[0];if(source!=e->step){e->q_valid=0;}e->step=source;if(e->edit_view&&e->edit_tool){Lane *l=&e->lane[e->selected];l->values[e->step][0]=!effective_hit(l,e->step);changed_step(e,e->selected,e->step,0);}else changed_selected(e);return;}
 if(parse_suffix(key,"edit_line",LANES,&i)){rand_clear(e);e->q_valid=0;e->selected=i;e->step=clampi(e->step,0,e->lane[i].f[0]-1);e->dialog=0;changed_selected(e);return;}
 if(parse_suffix(key,"open_sound",LANES,&i))return;
 if(!strcmp(key,"open_rhythm")||!strcmp(key,"apply_rhythm"))return; /* retired IDs remain reserved */
 else if(!strcmp(key,"close"))e->dialog=0;


 else if(!strcmp(key,"half")){e->lane[e->selected].f[3]=clampi(e->lane[e->selected].f[3]+1,0,4);e->release_pending|=1u<<e->selected;}
 else if(!strcmp(key,"double")){e->lane[e->selected].f[3]=clampi(e->lane[e->selected].f[3]-1,0,4);e->release_pending|=1u<<e->selected;}
 else return;
 if(!strcmp(key,"half")||!strcmp(key,"double"))changed_lane(e,e->selected);
 else changed_selected(e);
}
static int get_param(void *inst,const char *key,char *b,int size){if(size<=0)return 0;Engine *e=inst;int n=-1,i,j;int end=0;if(sscanf(key,"g%d_%d%n",&i,&j,&end)==2&&key[end]==0&&i>=0&&i<8&&j>=0&&j<32){mod_preview(e,e->mod_selected);return snprintf(b,size,"%d",mod_segment(e,e->mod_selected,(i*32+j)/128,(i*32+j)%128));}Lane *l=&e->lane[e->selected];
 if(size<=0)return 0;
 if(!strcmp(key,"rand_mode"))return snprintf(b,size,"%d",e->rand_mode);
 if(parse_suffix(key,"rand_pick",8,&i))return snprintf(b,size,"%d",e->rand_mode&&!!(e->rand_mask&(1u<<i)));
 if(parse_suffix(key,"rand_toggle",8,&i)||!strcmp(key,"randomize"))return snprintf(b,size,"0");
 if(parse_suffix(key,"lane_flash",LANES,&i))return snprintf(b,size,"%d",e->visual_ready?e->visual_last[VIS_FLASH+i]:0);
 if(!strcmp(key,"can_undo"))return snprintf(b,size,"%d",e->mod_pick);
 if(!strcmp(key,"map_sound"))return get_param(e,e->dialog==3?"play_mode":mod_pick_key(e),b,size);
 if(parse_suffix(key,"open_sound",8,&i)||!strcmp(key,"open_target")||!strcmp(key,"open_bank"))return snprintf(b,size,"0");
 if(!strcmp(key,"choose_target"))return snprintf(b,size,"%d",scale_pending(e));
 if(!strcmp(key,"choose_bank"))return snprintf(b,size,"0");
 if(!strcmp(key,"choose_sound"))return snprintf(b,size,"%d",e->scale_choice);
 if(!strcmp(key,"edit_row"))return snprintf(b,size,"%d",e->scale_start);
 if(!strcmp(key,"edit_row_display"))return note_display(e->scale_start,b,size);
 {int lane,end=0;if(sscanf(key,"l%d_dnote_display%n",&lane,&end)==1&&end>0&&!key[end]&&lane>=0&&lane<LANES)return note_display(e->lane[lane].f[9],b,size);}
 if(!strcmp(key,"state_valid"))return snprintf(b,size,"%d",e->state_valid);
 if(!strcmp(key,"state"))return write_evolution(e,b,size);
 if(!strncmp(key,"toggle_",7))return snprintf(b,size,"0");
 int mod_result=mod_get(e,key,b,size);if(mod_result>=0)return mod_result;
 int extra=evolution_get(e,key,b,size);if(extra>=0)return extra;
 if(!strcmp(key,"display_rev"))return snprintf(b,size,"%u",e->revision);
 if(!strcmp(key,"choose_target"))n=e->mode;else if(!strcmp(key,"choose_bank"))n=e->bank;else if(!strcmp(key,"choose_sound"))n=e->lane[e->row].f[11];else if(!strcmp(key,"line")||!strcmp(key,"line_select"))n=clampi(e->selected,0,7);else if(!strcmp(key,"line_select16"))n=e->selected;else if(!strcmp(key,"pattern_base"))n=e->dialog!=1;else if(!strcmp(key,"setup_base"))n=e->dialog<2;else if(!strcmp(key,"target_mode"))n=e->mode;else if(!strcmp(key,"channel"))n=e->channel;else if(!strcmp(key,"bank"))n=e->bank;else if(!strcmp(key,"selected_step"))n=e->step;else if(!strcmp(key,"rhythm_choice"))n=e->rhythm;else if(!strcmp(key,"dialog"))n=e->dialog;else if(!strcmp(key,"edit_row"))n=e->row;else if(!strcmp(key,"map_sound"))n=e->lane[e->row].f[11];
 else if(!strncmp(key,"sel_",4)){for(j=0;j<NF;j++)if(!strcmp(key+4,field_names[j])){n=l->f[j];break;}}
 else if(parse_lane(key,&i,&j))n=e->lane[i].f[j];
 else if(parse_suffix(key,"line_on",LANES,&i))n=e->selected==i;
 else if(parse_suffix(key,"preview",32,&i))n=4; /* retired RHYTHM readout */
 else if(parse_suffix(key,"cell",32,&i)){n=i>=l->f[0]?8:((l->pattern>>i)&1);if(e->step==i)n+=i>=l->f[0]?1:2;if(i<l->f[0]&&e->playing&&e->head[e->selected]==i)n+=4;}
 else if(sscanf(key,"g%d_%d",&i,&j)==2&&i>=0&&i<LANES&&j>=0&&j<32){Lane *t=&e->lane[i];n=j>=t->f[0]?4:((t->pattern>>j)&1);if(j<t->f[0]&&e->playing&&e->head[i]==j)n+=2;}
 else if(!strcmp(key,"line_label")){if(!e->mode)return snprintf(b,size,"%02d - %s",e->selected+1,sounds[l->f[11]]);return snprintf(b,size,"%02d - %c%02d",e->selected+1,'A'+e->bank,l->f[10]+1);}
 else if(!strcmp(key,"target_label"))return snprintf(b,size,"%s / CH %d / NOTE %d",e->mode?"DRUM TRACK":"INSTRUMENT",e->channel+1,note(e,e->selected));
 else if(!strcmp(key,"status"))return snprintf(b,size,"%s / %s%s%s",e->playing?"PLAY":"STOP",e->seq?"MIDI READY":"NO MIDI PORT",e->errors?" / CHECK MIDI":"",duplicates(e)?" / SHARED NOTE":"");
 else if(!strcmp(key,"step_label"))return snprintf(b,size,"STEP %02d / %s",e->step+1,e->step>=l->f[0]?"OUTSIDE":((l->pattern>>e->step)&1)?"HIT":"REST");
 else if(!strcmp(key,"mapping_label"))return snprintf(b,size,"%s",e->mode?"FORCE REFERENCE":"INCIPIENCE NOTES");
 else if(!strcmp(key,"port_label")){if(!e->slot)return snprintf(b,size,"piClideaN MPC Out");return snprintf(b,size,"piClideaN MPC %d Out",e->slot+1);}
 else if(parse_suffix(key,"name",LANES,&i)){if(!e->mode)return snprintf(b,size,"%s",sounds[e->lane[i].f[11]]);return snprintf(b,size,"%c%02d",'A'+e->bank,e->lane[i].f[10]+1);}
 else {return 0;}
 return n>=0?snprintf(b,size,"%d",n):0;
}
int mpc_engine_changed(void *inst,char *key,int len){
 Engine *e=inst;
 if(e->ui_actions){e->ui_actions=0;return snprintf(key,len,"can_undo");}
 if(e->scale_ui_pending){unsigned bit=e->scale_ui_pending&1?1:e->scale_ui_pending&2?2:4;e->scale_ui_pending&=~bit;return snprintf(key,len,"%s",bit==1?"choose_sound":bit==2?"edit_row":"choose_target");}
 while(e->changes>0){
  int id=e->changed[e->change_read],k=id;e->change_read=(e->change_read+1)%CHANGED_COUNT;e->changes--;e->queued[id]=0;
  /* Legacy selectors have an eight-lane normalized range. Do not publish
     their clamped mirror: a host echo could otherwise reselect lane 8. */
  if(k==LANES*NF+10||k==LANES*NF+16)continue;
  if(k>=MOD_ID){mod_key(k-MOD_ID,key,len);}
  else if(k>=RAND_ID){if(k==RAND_ID)snprintf(key,len,"rand_mode");else snprintf(key,len,"rand_pick%d",k-RAND_ID-1);}
  else if(k>=HOLD_ID){snprintf(key,len,"l%d_hold",k-HOLD_ID);}
  else if(k==SELECT16_ID){snprintf(key,len,"line_select16");}
  else if(k>=LEGACY_CHANGED){evolution_key(k-LEGACY_CHANGED,key,len);}
  else if(k<LANES*NF)snprintf(key,len,"l%d_%s",k/NF,field_names[k%NF]);
  else{
   k-=LANES*NF;
   if(k<8)snprintf(key,len,"sel_%s",field_names[k]);
   else{static const char *keys[]={"sel_mute","sel_solo","line","target_mode","channel","bank","map_sound","dialog","line_select","pattern_base","setup_base"};snprintf(key,len,"%s",keys[k-8]);}
  }
  char value[32],*end;
  if(get_param(e,key,value,sizeof value)<=0)continue;errno=0;double current=strtod(value,&end);if(errno||end==value||*end||!isfinite(current))continue;
  /* Do not re-report unchanged option lists: the generic wrapper refreshes those.
     Reporting every option on each knob write would resurrect the Q-Link bug. */
  if(e->have_changed[id]&&fabs(e->last_changed[id]-current)<1e-7)continue;
  e->have_changed[id]=1;e->last_changed[id]=current;return (int)strlen(key);
 }
 for(int scanned=0;scanned<VISUALS;scanned++){
  int id=e->visual_cursor;e->visual_cursor=(e->visual_cursor+1)%VISUALS;
  if(!e->visual_dirty[id])continue;e->visual_dirty[id]=0;visual_key(id,key,len);return (int)strlen(key);
 }
 return 0;
}
/* Port-local host bridge: supplied by our documented wrapper patch, not by Euclidier. */
void picn_engine_context(void *inst,double a,double b,int flags,double sr){Engine *e=inst;e->sample_rate=isfinite(sr)&&sr>=8000&&sr<=192000?sr:44100;e->cycle=(flags&(1<<2))&&(flags&(1<<12))&&isfinite(a)&&isfinite(b)&&a>=0&&b<=1e9&&b>a;e->cycle_a=a;e->cycle_b=b;}
/* Each ratchet group is reconstructed from its absolute step identity, so block
   boundaries do not redraw probability or lose later repeats. No catch-up notes. */
static int hit_after(const Hit *a,const Hit *b){return a->beat>b->beat||(a->beat==b->beat&&(a->kind<b->kind||(a->kind==b->kind&&(a->line>b->line||(a->line==b->line&&a->repeat>b->repeat)))));}
static void sift_hits(Hit *hits,int root,int count){
 for(;;){int child=root*2+1;if(child>=count)return;if(child+1<count&&hit_after(&hits[child+1],&hits[child]))child++;if(!hit_after(&hits[child],&hits[root]))return;Hit tmp=hits[root];hits[root]=hits[child];hits[child]=tmp;root=child;}
}
static void sort_hits(Hit *hits,int count){
 for(int root=count/2;root>0;)sift_hits(hits,--root,count);
 for(int end=count-1;end>0;end--){Hit tmp=hits[0];hits[0]=hits[end];hits[end]=tmp;sift_hits(hits,0,end);}
}
static int span(Engine *e,double a,double b,double bpm,double time_base,double cycle_end,int *budget){
 Hit hits[EVENTS];int nh=0;if(!(b>a))return 1;
 if(!mod_events(e,hits,&nh,a,b,bpm,time_base,budget))return 0;
 for(int i=0;i<LANES;i++){Lane *l=&e->lane[i];double d=step_beats[l->f[3]],k0=floor(a/d),k1=ceil(b/d);if(k1-k0>64){e->errors++;return 0;}
  for(double k=k0;k<k1;k++){double start=k*d;if(e->reset_at[i]>=0&&start>=e->reset_at[i]-1e-8){l->origin=start;e->reset_at[i]=-1;}int position=visit_index(l,k),source=source_index(l,position,k);if(!audible(e,i)||!effective_hit(l,source))continue;
   unsigned seed=mix32(l->seed^(unsigned)fmod(k,4294967295.)^mix32((unsigned)floor(k/4294967295.)));int mv[6];mod_step_values(e,i,k,source,(start-a)*60./bpm+e->mod_span_offset,bpm,mv);int prob=mv[1];if(prob<100&&(int)(seed%100)>=prob)continue;
   int human=mv[2],vel=clampi(mv[0]+(human?(int)(mix32(seed^0x984a21u)%(unsigned)(2*human+1))-human:0),1,127);
   int count=mv[4],decay=mv[5];double sub=d/count,velocity=vel;
   for(int r=0;r<count;r++){double beat=start+r*sub;int value=clampi((int)lround(velocity),0,127);velocity*=decay/100.;if(!value||beat<a||beat>=b)continue;
    if(nh>=EVENTS||*budget<=0){e->errors++;return 0;}(*budget)--;hits[nh++]=(Hit){i,source,r,value,beat,sub*mv[3]/100.,0};}
  }
 }
 sort_hits(hits,nh);
 for(int i=0;i<nh;i++){double offset=(hits[i].beat-a)*60./bpm,limit=isfinite(cycle_end)?(cycle_end-hits[i].beat)*60./bpm:INFINITY;if(hits[i].kind)mod_trigger(e,&hits[i],time_base+offset);else trigger(e,&hits[i],bpm,time_base+offset,limit);}return 1;
}
/* Clear only playback indication; the inspected step remains selected. */
static void clear_playhead(Engine *e){
 int changed=e->playing;
 for(int i=0;i<LANES;i++){if(e->head[i]!=-1)changed=1;e->head[i]=-1;e->reset_at[i]=-1;}
 e->playing=0;e->valid=0;
 if(changed)e->revision++;
}
/* No host screen rebuild: publish only changed numeric visual handles. */
static void visual_key(int id,char *key,int size){
 if(id<32)snprintf(key,size,"cell%d",id);
 else if(id<288)snprintf(key,size,"g%d_%d",(id-32)/32,(id-32)%32);
 else if(id<VIS_PREVIEW)snprintf(key,size,"line_on%d",id-VIS_LANE);
 else if(id<VIS_CELL)snprintf(key,size,"preview%d",id-VIS_PREVIEW);
 else if(id<VIS_PHASE)snprintf(key,size,"editcell%d",id-VIS_CELL);
 else if(id<VIS_CUSTOM)snprintf(key,size,"%s",id==VIS_PHASE?"phase_cycle":"phase_offset");
 else if(id<VIS_POSITION)snprintf(key,size,"step_custom%d",id-VIS_CUSTOM);
 else if(id==VIS_POSITION)snprintf(key,size,"step_position");
 else if(id<VIS_FLASH)snprintf(key,size,"trackcell%d_%d",(id-VIS_TRACK)/32,(id-VIS_TRACK)%32);
 else if(id<VIS_MOD)snprintf(key,size,"lane_flash%d",id-VIS_FLASH);
 else mod_visual_key(id-VIS_MOD,key,size);
}
static void visual_snapshot(Engine *e,int frames,int force){
 if(e->mod_preview_async)mod_preview(e,e->mod_selected);
 if(e->visual_ready&&e->visual_revision==e->revision&&!e->playing)return;
 e->visual_frames+=frames;
 if(!force&&e->visual_ready&&e->visual_revision==e->revision&&e->visual_frames<e->sample_rate/30.)return;
 /* Playback and UI writes share the same display cap; MIDI never waits for it. */
 if(!force&&e->visual_ready&&e->visual_frames<e->sample_rate/30.)return;
 e->visual_frames=0;int offsets[LANES];for(int i=0;i<LANES;i++){offsets[i]=rotation_at(&e->lane[i],e->display_tick[i]);e->ui_offset[i]=offsets[i];}
 Lane *selected=&e->lane[e->selected];
 for(int id=0;id<VISUALS;id++){
  int v=0;
  if(id<32){continue;}
  else if(id<288){mod_preview(e,e->mod_selected);int vindex=id-32;v=mod_segment(e,e->mod_selected,vindex/128,vindex%128);} /* Reuse retired, read-only grid indices; no saved musical parameter changes. */
  else if(id<VIS_PREVIEW)v=e->selected==id-VIS_LANE;
  else if(id<VIS_CELL)v=4; /* retired RHYTHM readouts */
  else if(id<VIS_PHASE){int j=id-VIS_CELL,source=j<selected->f[0]?(j-offsets[e->selected]+selected->f[0])%selected->f[0]:j;v=j>=selected->f[0]?16:effective_hit(selected,source)+(source==e->step?2:0)+(e->playing&&e->head[e->selected]==j?4:0)+(personalized(selected,source)?8:0);if(j>=selected->f[0]&&j==e->step)v++;}
  else if(id==VIS_PHASE)v=(int)fmax(1.,fmin(INT_MAX,floor(local_tick(selected,e->display_tick[e->selected])/cycle_size(selected))+1));
  else if(id==VIS_PHASE+1)v=offsets[e->selected];
  else if(id<VIS_POSITION)v=selected->values[e->step][id-VIS_CUSTOM+1]>=0;
  else if(id==VIS_POSITION)v=e->step<selected->f[0]?(e->step+offsets[e->selected])%selected->f[0]+1:e->step+1;
  else if(id<VIS_FLASH){int line=(id-VIS_TRACK)/32,j=(id-VIS_TRACK)%32;Lane *l=&e->lane[line];int source=j<l->f[0]?(j-offsets[line]+l->f[0])%l->f[0]:j;v=j>=l->f[0]?8:effective_hit(l,source)+(e->playing&&e->head[line]==j?2:0)+(personalized(l,source)?4:0);}
  else if(id<VIS_MOD)v=flash_current(e,id-VIS_FLASH);
  else {int m=id-VIS_MOD;if(m>=LANES+5&&m!=LANES+5+31&&m!=LANES+5+63){e->visual_dirty[id]=0;continue;}v=mod_visual(e,m);}
  if(!e->visual_ready||e->visual_last[id]!=v){e->visual_last[id]=v;e->visual_dirty[id]=1;}
 }
 e->visual_ready=1;e->visual_revision=e->revision;
}
static void song_pos(void *inst,double ppq,double bpm,int playing,int frames){
 Engine *e=inst;
 mod_advance(e,frames,bpm);
 if(e->panic)panic_owned(e);
 for(int i=0;i<LANES;i++)if((e->release_pending&(1u<<i))||(e->active_note[i]>=0&&!audible(e,i)))release(e,i);
 if(!playing||!isfinite(ppq)||ppq<0||ppq>1e9||!isfinite(bpm)||bpm<=0||bpm>1000||frames<=0){if(e->playing)panic_owned(e);clear_playhead(e);visual_snapshot(e,0,1);return;}
 double block=(double)frames/e->sample_rate*bpm/60.;
 if(!isfinite(block)||block>4||!e->seq||!e->queue_status||snd_seq_get_queue_status(e->seq,e->queue,e->queue_status)<0){if(e->seq)e->errors++;panic_owned(e);clear_playhead(e);visual_snapshot(e,0,1);return;}
 const snd_seq_real_time_t *qt=snd_seq_queue_status_get_real_time(e->queue_status);
 e->queue_now=qt->tv_sec+qt->tv_nsec/1e9;
 /* The host PPQ is a measured position, not an exact sample-clock increment.
    Keep a coverage watermark as in 0.1.0-candidate.3: overlap never repeats
    notes and ordinary host rounding must never cancel the kernel queue. */
 double host_ppq=ppq;
 double tolerance=fmax(1./960.,block*2.);
 double gap=e->valid?ppq-e->expected_ppq:0.;
 if(e->valid&&e->cycle){double length=e->cycle_b-e->cycle_a;
  if(fabs(gap-length)<=tolerance)gap-=length;
  else if(fabs(gap+length)<=tolerance)gap+=length;
 }
 int cycle_wrap=e->valid&&e->cycle&&fabs(gap)<=tolerance&&
  e->last_host_ppq>=e->cycle_a&&e->last_host_ppq<e->cycle_b&&
  host_ppq>=e->cycle_a&&host_ppq<e->cycle_b;
 int backwards=e->valid&&host_ppq<e->last_host_ppq-tolerance&&!cycle_wrap;
 /* A forward discontinuity larger than four blocks is a seek. Small gaps
    belong to the next covered interval instead of silently losing its hits. */
 int jump=e->valid&&(backwards||fabs(gap)>fmax(.25,block*4.));
 if(jump){panic_owned(e);e->valid=0;for(int i=0;i<LANES;i++)e->reset_at[i]=-1;}
 for(int i=0;i<LANES;i++)if(!(e->release_pending&(1u<<i))&&e->active_note[i]>=0&&e->active_off[i]<=e->queue_now)e->active_note[i]=-1;
 double base=e->valid?e->queue_end:e->queue_now+0.002;
 if(e->valid){
  /* gap is already unwrapped across a predicted loop. Keep the watermark
     inside the musical cycle; only the interval length uses that signed gap. */
  ppq=e->expected_ppq;block+=gap;
  if(block<=1e-10){e->last_host_ppq=host_ppq;visual_snapshot(e,frames,0);return;}
 }
 /* Scheduling late is not a musical seek. Reanchor pending new notes only;
    never erase already queued Note On/Off because of callback jitter. */
 if(base<e->queue_now+0.001)base=e->queue_now+0.002;
 /* Never pull a future boundary backwards: that would overlap prior notes. */
 double pos=ppq,remaining=block,elapsed=0;
 int budget=EVENTS;
 int in_cycle=e->cycle&&pos>=e->cycle_a-1e-8&&pos<e->cycle_b;
 for(int segments=0;remaining>1e-10&&segments<64;segments++){
  double duration=remaining;
  if(in_cycle)duration=fmin(duration,e->cycle_b-pos);
  if(duration<=1e-10){pos=e->cycle_a;continue;}
  e->mod_span_offset=elapsed*60./bpm;
  if(!span(e,pos,pos+duration,bpm,base+elapsed*60./bpm,in_cycle?e->cycle_b:INFINITY,&budget)){panic_owned(e);clear_playhead(e);visual_snapshot(e,0,1);return;}
  pos+=duration;elapsed+=duration;remaining-=duration;
  if(in_cycle&&pos>=e->cycle_b-1e-10){pos=e->cycle_a;memset(e->mod_latch_valid,0,sizeof e->mod_latch_valid);for(int i=0;i<LANES;i++)if(e->reset_at[i]>=0)e->reset_at[i]=e->cycle_a;}
 }
 if(remaining>1e-10){e->errors++;panic_owned(e);clear_playhead(e);visual_snapshot(e,0,1);return;}
 e->queue_end=base+elapsed*60./bpm;e->expected_ppq=pos;e->last_host_ppq=host_ppq;
 int moved=0;
 for(int i=0;i<LANES;i++){double k=floor((ppq+1e-7)/step_beats[e->lane[i].f[3]]);e->display_tick[i]=k;long h=visit_index(&e->lane[i],k);if(e->head[i]!=h){moved=1;e->head[i]=h;}}
 if(moved||!e->playing)e->revision++;
 int starting=!e->playing;e->playing=1;e->valid=1;
 visual_snapshot(e,frames,starting);
}
static void render(void *inst,int16_t *out,int frames){(void)inst;if(out&&frames>0)memset(out,0,(size_t)frames*4);}
static const mpc_engine_t api={create,destroy,midi,set_param,get_param,render,NULL,song_pos};
const mpc_engine_t *mpc_engine(void){return &api;}

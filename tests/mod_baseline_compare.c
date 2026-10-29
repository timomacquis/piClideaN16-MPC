/* Event-for-event regression against the preserved pre-E01 engine. */
#define snd_seq_event_output_direct captured_output
#ifdef BASELINE
#include "../Validation/E01-baseline/engine.c"
#else
#include "../src/engine.c"
#endif
#undef snd_seq_event_output_direct
int captured_output(snd_seq_t *s,snd_seq_event_t *v){(void)s;printf("%d %d %d %d %u %u\n",v->type,v->data.note.note,v->data.note.velocity,v->tag,v->time.time.tv_sec,v->time.time.tv_nsec);return 0;}
int main(void){Engine *e=calloc(1,sizeof *e);e->mode=1;e->channel=2;e->sample_rate=48000;
#ifndef BASELINE
mod_init(e);
#endif
for(int i=0;i<16;i++){init_lane(&e->lane[i],i);e->active_note[i]=-1;e->reset_at[i]=-1;Lane *l=&e->lane[i];l->f[0]=7+i;l->f[1]=i%7+1;l->f[2]=i%l->f[0];l->f[3]=i%5;l->f[4]=60+i*4;l->f[5]=i;l->f[6]=60+i*2;l->hold=(i%4==0);l->playmode=i%5;l->auto_rotate=i%2;l->shift=1;l->every=2;pattern(l);l->values[2][3]=3;l->values[2][4]=70;l->values[1][2]=80;}
e->seq=(snd_seq_t*)1;e->port=1;e->queue=1;double p=0;for(int block=0;block<4096;block++){double d=.011+(block%3)*.00007;int budget=EVENTS;if(!span(e,p,p+d,123,1+p*60/123,INFINITY,&budget))return 2;p+=d;}fprintf(stderr,"errors=%u\n",e->errors);free(e);return 0;}

/* Bounded musical state, editing and deterministic playback. Included once by engine.c. */
static unsigned mix32(unsigned x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
static int effective_hit(const Lane *l,int step){return step<l->f[0]&&(l->values[step][0]<0?(int)((l->pattern>>step)&1):l->values[step][0]);}
static int effective_value(const Lane *l,int step,int field){static const int base[]={0,4,6,0,0};return l->values[step][field]>=0?l->values[step][field]:field==3?1:field==4?100:l->f[base[field]];}
static int personalized(const Lane *l,int step){for(int i=1;i<5;i++)if(l->values[step][i]>=0)return 1;return 0;}
static int cycle_size(const Lane *l){int n=l->f[0];return n==1?1:l->playmode==2?2*n:l->playmode==3?2*n-2:n;}
static double local_tick(const Lane *l,double tick){return floor(tick-l->origin/step_beats[l->f[3]]+1e-8);}
static int rotation_at(const Lane *l,double tick){if(!l->auto_rotate)return 0;double cycles=floor(local_tick(l,tick)/cycle_size(l));int offset=(int)fmod(floor(cycles/l->every)*l->shift,l->f[0]);return (offset+l->f[0])%l->f[0];}
static int visit_index(const Lane *l,double tick){int n=l->f[0],size=cycle_size(l),p=(int)fmod(local_tick(l,tick),size);if(p<0)p+=size;if(l->playmode==1)return n-1-p;if(l->playmode==2)return p<n?p:2*n-1-p;if(l->playmode==3)return p<n?p:2*n-2-p;if(l->playmode==4){unsigned order[32],seed=mix32(l->seed^(unsigned)fmod(fmod(floor(local_tick(l,tick)/size),4294967295.)+4294967295.,4294967295.));for(int i=0;i<n;i++)order[i]=(unsigned)i;for(int i=n-1;i>0;i--){seed=mix32(seed+0x9e3779b9u);int j=(int)(seed%(unsigned)(i+1));unsigned tmp=order[i];order[i]=order[j];order[j]=tmp;}return (int)order[p];}return p;}
static int source_index(const Lane *l,int position,double tick){return position<l->f[0]?(position-rotation_at(l,tick)+l->f[0])%l->f[0]:position;}
/* UNDO/REDO removed; published parameter keys remain inert below. */
static void init_lane(Lane *l,int i){
 memset(l,0,sizeof *l);static const int old_hits[8]={4,2,11,3,4,5,1,3};
 int d[NF]={16,i<8?old_hits[i]:0,i==1?4:0,1,i==0?112:100,0,100,50,drum_notes[i],drum_notes[i],i,11,0,0};
 memcpy(l->f,d,sizeof d);pattern(l);l->shift=l->every=1;l->seed=0x519b17u+i*271u;
 for(int j=0;j<32;j++)for(int k=0;k<5;k++)l->values[j][k]=-1;
}
static const char *step_fields[]={"trig","vel","prob","ratchet","decay"};
static const char *extra_keys[]={"edit_view","edit_tool","step_velocity","step_prob","step_ratchet","step_decay","play_mode","auto_rotate","auto_shift","auto_every","can_undo","can_redo","selected_step","pattern_base"};
static const char *play_fields[]={"playmode","auto","shift","every"};
static void evolution_key(int id,char *key,int size){if(id>=14+LANES*4+LANES*160){snprintf(key,size,"q%d",id-14-LANES*4-LANES*160);return;}if(id<14)snprintf(key,size,"%s",extra_keys[id]);else if((id-=14)<LANES*4)snprintf(key,size,"l%d_%s",id/4,play_fields[id%4]);else{id-=LANES*4;snprintf(key,size,"l%d_s%d_%s",id/160,(id%160)/5,step_fields[id%5]);}}
static int parse_step_key(const char *key,int *line,int *step,int *field){
 const char *p;if(!lane_prefix(key,line,&p)||*p++!='s')return 0;
 if(*p<'0'||*p>'9')return 0;int n=*p++-'0';
 if(*p>='0'&&*p<='9')n=n*10+*p++-'0';if(*p!='_'||n>=32)return 0;p++;
 for(int i=0;i<5;i++)if(!strcmp(p,step_fields[i])){*step=n;*field=i;return 1;}return 0;
}
static int *play_field(Lane *l,int field){return field==0?&l->playmode:field==1?&l->auto_rotate:field==2?&l->shift:&l->every;}
static int parse_play_key(const char *key,int *line,int *field){
 const char *suffix;if(!lane_prefix(key,line,&suffix))return 0;
 for(int i=0;i<4;i++)if(!strcmp(suffix,play_fields[i])){*field=i;return 1;}return 0;
}
static const char *q_target(Engine *e,int q){static const char *gen[]={"sel_velocity","sel_prob","sel_humanize","sel_gate","sel_steps","sel_hits","sel_rotate","sel_division"},*edit[]={"sel_velocity","sel_prob","sel_humanize","sel_gate","step_velocity","step_prob","step_ratchet","step_decay"};return e->edit_view?edit[q]:gen[q];}
static void q_range(Engine *e,int q,int *lo,int *hi){static const int gen_lo[]={1,0,0,10,1,0,0,0},gen_hi[]={127,100,50,95,32,32,31,4},edit_lo[]={1,0,0,10,1,0,1,0},edit_hi[]={127,100,50,95,127,100,8,150};*lo=e->edit_view?edit_lo[q]:gen_lo[q];*hi=e->edit_view?edit_hi[q]:gen_hi[q];if(!e->edit_view&&q==5)*hi=e->lane[e->selected].f[0];if(!e->edit_view&&q==6)*hi=e->lane[e->selected].f[0]-1;}
/* Q-Link aliases are continuous: preserve fractional moves instead of rounding
   them to a 127-bin value before mapping a seven-value ratchet range. */
static int q_set(Engine *e,const char *key,const char *value){
 int q;if(!parse_suffix(key,"q",8,&q))return 0;
 /* EDIT keeps LINE values visible as references, never as contextual targets.
    Canonical lN_* automation remains writable independently of this UI guard. */
 if(e->dialog==1||(e->edit_view&&q<4))return 1;
 char *end;errno=0;double raw=strtod(value,&end);
 if(errno||end==value||*end||!isfinite(raw))return 1;raw=fmax(0,fmin(127,raw));
 int lo,hi;q_range(e,q,&lo,&hi);if(hi<=lo)return 1;const char *target=q_target(e,q);char buf[24];int current;
 if(get_param(e,target,buf,sizeof buf)<=0||!integer(buf,&current))return 1;
 double requested=lo+raw*(hi-lo)/127.;QMove *last=&e->qmove[q];
 int valid=(e->q_valid&(1u<<q))&&last->actual==current&&last->line==e->selected&&last->step==e->step&&last->view==e->edit_view&&last->lo==lo&&last->hi==hi;
 int next;if(fabs(requested-round(requested))<=.001)next=(int)lround(requested);
 else{double direction=valid&&fabs(requested-last->requested)<.5?requested-last->requested:requested-current;next=fabs(direction)<1e-7?current:direction>0?(int)ceil(requested-.001):(int)floor(requested+.001);}
 next=clampi(next,lo,hi);*last=(QMove){requested,next,e->selected,e->step,e->edit_view,lo,hi};e->q_valid|=1u<<q;
 if(next==current)return 1;snprintf(buf,sizeof buf,"%d",next);set_param(e,target,buf);return 1;
}
static int evolution_set(Engine *e,const char *key,int v){Lane *l=&e->lane[e->selected];int line,step,field;

 const char *suffix;
 if(lane_prefix(key,&line,&suffix)&&!strcmp(suffix,"hold")){
  int next=clampi(v,0,1);if(e->lane[line].hold!=next){e->lane[line].hold=next;e->release_pending|=1u<<line;notify_id(e,HOLD_ID+line);e->revision++;}return 1;
 }
 if(!strncmp(key,"toggle_",7)){if(v<=0)return 1;const char *target=key+7;char buf[24];int current;
  if(strcmp(target,"auto_rotate")&&strcmp(target,"sel_mute")&&strcmp(target,"sel_solo")){int li,fi;if(!parse_lane(target,&li,&fi)||fi<12)return 1;}
  if(get_param(e,target,buf,sizeof buf)>0&&integer(buf,&current)){snprintf(buf,sizeof buf,"%d",!current);set_param(e,target,buf);}return 1;
 }
 if(!strcmp(key,"edit_view")){if(e->edit_view==clampi(v,0,1))return 1;rand_clear(e);e->edit_view=clampi(v,0,1);e->q_valid=0;changed_selected(e);return 1;}
 if(!strcmp(key,"edit_tool")){if(e->edit_tool==clampi(v,0,1))return 1;e->edit_tool=clampi(v,0,1);changed_selected(e);return 1;}
 for(int f=1;f<5;f++)if(!strcmp(key,extra_keys[f+1])){int value=clampi(v,f==1||f==3?1:0,f==1?127:f==2?100:f==3?8:150);if(l->values[e->step][f]==value||(l->values[e->step][f]<0&&effective_value(l,e->step,f)==value))return 1;l->values[e->step][f]=value;changed_step(e,e->selected,e->step,f);return 1;}
 if(parse_step_key(key,&line,&step,&field)){int value=clampi(v,-1,field==0?1:field==1?127:field==2?100:field==3?8:150);if((field==1||field==3)&&value==0)value=1;if(e->lane[line].values[step][field]!=value){e->lane[line].values[step][field]=value;changed_step(e,line,step,field);}return 1;}
 if(parse_play_key(key,&line,&field)){int value=clampi(v,field==2?-8:field==3?1:0,field==0?4:field==1?1:field==2?8:16);if(*play_field(&e->lane[line],field)!=value){*play_field(&e->lane[line],field)=value;e->release_pending|=1u<<line;changed_lane(e,line);}return 1;}
 for(int f=0;f<4;f++)if(!strcmp(key,extra_keys[6+f])){int value=clampi(v,f==2?-8:f==3?1:0,f==0?4:f==1?1:f==2?8:16);if(*play_field(l,f)!=value){*play_field(l,f)=value;e->release_pending|=1u<<e->selected;changed_lane(e,e->selected);}return 1;}
 if(!strcmp(key,"erase_values")){if(v>0&&e->edit_tool){if(effective_hit(l,e->step)){l->values[e->step][0]=0;changed_step(e,e->selected,e->step,0);}return 1;}if(v>0&&personalized(l,e->step)){for(int f=1;f<5;f++){l->values[e->step][f]=-1;changed_step(e,e->selected,e->step,f);}}return 1;}
 if(!strcmp(key,"undo")||!strcmp(key,"redo"))return 1; /* retired */
 if(!strcmp(key,"reset_phase")){if(v>0){double d=step_beats[l->f[3]];if(e->playing)e->reset_at[e->selected]=ceil((e->expected_ppq-1e-8)/d)*d;else{l->origin=0;e->display_tick[e->selected]=0;e->reset_at[e->selected]=-1;}changed_selected(e);}return 1;}
 return 0;
}
static int evolution_get(Engine *e,const char *key,char *b,int size){Lane *l=&e->lane[e->selected];int n=-1,line,step,field;const char *suffix_hold;
 if(lane_prefix(key,&line,&suffix_hold)&&!strcmp(suffix_hold,"hold"))return snprintf(b,size,"%d",e->lane[line].hold);
 if(key[0]=='q'&&key[1]>='0'&&key[1]<='7'){int q=key[1]-'0';const char *suffix=key+2,*target=q_target(e,q);char buf[24];int value=0;if(!*suffix||!strcmp(suffix,"_display")){if(get_param(e,target,buf,sizeof buf)>0)integer(buf,&value);if(!strcmp(suffix,"_display")){if(!e->edit_view&&q==7){static const char *div[]={"1/32","1/16","1/8","1/4","1/2"};return snprintf(b,size,"%s",div[clampi(value,0,4)]);}return snprintf(b,size,"%d",value);}int lo,hi;q_range(e,q,&lo,&hi);return snprintf(b,size,"%.9g",hi>lo?127.*(value-lo)/(hi-lo):0.);}if(!strcmp(suffix,"_name")){static const char *gen[]={"VELOCITY","PROBABILITY","HUMANIZE","GATE","STEPS","HITS","ROTATE","DIVISION"},*edit[]={"VELOCITY","PROBABILITY","HUMANIZE","GATE","VELOCITY","PROBABILITY","RATCHET","DECAY"};return snprintf(b,size,"%s",e->edit_view?edit[q]:gen[q]);}}
 if(!strcmp(key,"rhythm_choice_display")){static const char *names[]={"FOUR ON THE FLOOR","TRESILLO","CINQUILLO","E(12,5)","E(16,7)","E(16,11)"};return snprintf(b,size,"%s",names[e->rhythm]);}
 if(!strcmp(key,"edit_tool_display"))return snprintf(b,size,"%s",e->edit_tool?"DRAW":"SELECT");
 if(!strcmp(key,"play_mode_display")){static const char *names[]={"FWD","REV","PEND1","PEND2","URN"};return snprintf(b,size,"%s",names[l->playmode]);}
 if(!strcmp(key,"edit_view"))n=e->edit_view;else if(!strcmp(key,"edit_tool"))n=e->edit_tool;
 else if(!strcmp(key,"can_undo"))n=0;else if(!strcmp(key,"can_redo"))n=0;
 else if(parse_step_key(key,&line,&step,&field))n=e->lane[line].values[step][field];
 else if(parse_play_key(key,&line,&field))n=*play_field(&e->lane[line],field);
 else if(!strcmp(key,"step_scope"))return snprintf(b,size,"SELECTED STEP / A%02d /",e->selected+1);
 else if(!strcmp(key,"step_position")&&e->visual_ready)n=e->visual_last[VIS_POSITION];
 else if(!strcmp(key,"step_position"))n=e->step<l->f[0]?(e->step+rotation_at(l,e->display_tick[e->selected]))%l->f[0]+1:e->step+1;
 else if(parse_suffix(key,"step_custom",4,&field)&&e->visual_ready)n=e->visual_last[VIS_CUSTOM+field];
 else if(parse_suffix(key,"step_custom",4,&field))n=l->values[e->step][field+1]>=0;
 else if(!strcmp(key,"line_scope"))return snprintf(b,size,"LINE DEFAULTS / A%02d",e->selected+1);
 else if(!strcmp(key,"phase_cycle")&&e->visual_ready)n=e->visual_last[VIS_PHASE];
 else if(!strcmp(key,"phase_cycle"))return snprintf(b,size,"%.0f",fmax(1.,fmin(2147483647.,floor(local_tick(l,e->display_tick[e->selected])/cycle_size(l))+1)));
 else if(!strcmp(key,"phase_offset")&&e->visual_ready)n=e->visual_last[VIS_PHASE+1];
 else if(!strcmp(key,"phase_offset"))n=rotation_at(l,e->display_tick[e->selected]);
 else if(!strcmp(key,"phase_label"))return snprintf(b,size,"CYCLE %.0f / SHIFT %d",floor(local_tick(l,e->display_tick[e->selected])/cycle_size(l))+1,rotation_at(l,e->display_tick[e->selected]));
 else if(parse_suffix(key,"step_inherit",4,&field))return snprintf(b,size,"%s",l->values[e->step][field+1]<0?"LINE DEFAULT":"CUSTOM");
 else if(parse_suffix(key,"editcell",32,&step)&&e->visual_ready)n=e->visual_last[VIS_CELL+step];
 else if(parse_suffix(key,"editcell",32,&step)){int source=source_index(l,step,e->display_tick[e->selected]);n=step>=l->f[0]?16:(effective_hit(l,source)?1:0)+(source==e->step?2:0)+(e->playing&&e->head[e->selected]==step?4:0)+(personalized(l,source)?8:0);if(step>=l->f[0]&&e->step==step)n++;}
 else if(parse_suffix(key,"cell",32,&step)){int source=source_index(l,step,e->display_tick[e->selected]);n=step>=l->f[0]?8:effective_hit(l,source);if(e->step==source)n+=step>=l->f[0]?1:2;if(step<l->f[0]&&e->playing&&e->head[e->selected]==step)n+=4;}
 else if(sscanf(key,"trackcell%d_%d",&line,&step)==2&&line>=0&&line<LANES&&step>=0&&step<32){if(e->visual_ready)return snprintf(b,size,"%d",e->visual_last[VIS_TRACK+line*32+step]);Lane *t=&e->lane[line];int source=source_index(t,step,e->display_tick[line]);n=step>=t->f[0]?8:effective_hit(t,source)+(e->playing&&e->head[line]==step?2:0)+(personalized(t,source)?4:0);}
 else if(sscanf(key,"g%d_%d",&line,&step)==2&&line>=0&&line<LANES&&step>=0&&step<32){Lane *t=&e->lane[line];n=step>=t->f[0]?4:effective_hit(t,source_index(t,step,e->display_tick[line]));if(step<t->f[0]&&e->playing&&e->head[line]==step)n+=2;}
 else {for(int f=1;f<5;f++)if(!strcmp(key,extra_keys[f+1]))return snprintf(b,size,"%d",effective_value(l,e->step,f));for(int f=0;f<4;f++)if(!strcmp(key,extra_keys[f+6]))return snprintf(b,size,"%d",*play_field(l,f));return -1;}
 return snprintf(b,size,"%d",n);
}
/* PICN4 appends sixteen HOLD flags; older PICN1/2/3 load with GATE. */
static int write_evolution(Engine *e,char *b,int size){int off=0;
#define PUT(...) do{int wrote=snprintf(b+off,size-off,__VA_ARGS__);if(wrote<0||wrote>=size-off)return 0;off+=wrote;}while(0)
 PUT("PICN6 %d %d %d %d %d %d %d 1 ",e->selected,e->mode,e->channel,e->bank,e->step,e->rhythm,e->row);
 for(int i=0;i<LANES;i++)for(int j=0;j<NF;j++)PUT("%d ",e->lane[i].f[j]);
 PUT("%d %d ",e->edit_view,e->edit_tool);
 for(int i=0;i<LANES;i++){Lane *l=&e->lane[i];PUT("%d %d %d %d %u %.17g ",l->playmode,l->auto_rotate,l->shift,l->every,l->seed,l->origin);for(int j=0;j<32;j++)for(int k=0;k<5;k++)PUT("%d ",l->values[j][k]);}
 for(int i=0;i<LANES;i++)PUT("%d ",e->lane[i].hold);
 PUT("%d ",e->mod_selected);for(int i=0;i<MOD_SLOTS;i++){PUT("%d %d ",e->mod_pad[i]+1,e->mod[i].mix);for(int j=0;j<2;j++)for(int k=0;k<MOD_FIELDS;k++)PUT("%d ",e->mod[i].osc[j].f[k]);}
 PUT("SCALE %d %d END",e->scale_applied_choice,e->scale_applied_start);return off;
#undef PUT
}
static int read_number(const char **p,double *n){char *end;errno=0;*n=strtod(*p,&end);if(errno||end==*p||!isfinite(*n)||(*end&&*end!=' '))return 0;*p=end;while(**p==' ')(*p)++;return 1;}
static int read_int(const char **p,int low,int high,int *n){double d;if(!read_number(p,&d)||d<low||d>high||d!=floor(d))return 0;*n=(int)d;return 1;}
static int read_evolution(Engine *e,const char *value){const char *p=value+6;int slot_format=!strncmp(value,"PICN6 ",6);if(!strncmp(value,"PICN5 ",6))return 0;int has_mod=slot_format;int has_hold=has_mod||!strncmp(value,"PICN4 ",6);ModLane mods[MOD_SLOTS];int pads[MOD_SLOTS]={-1,-1,-1,-1},selected_mod=0;for(int i=0;i<MOD_SLOTS;i++)mod_defaults(&mods[i]);int count=(has_hold||!strncmp(value,"PICN3 ",6))?LANES:8;int h[8],view,tool;Lane lanes[LANES];for(int i=0;i<LANES;i++)init_lane(&lanes[i],i);int highs[]={count-1,1,15,7,31,5,count-1,1};
 for(int i=0;i<8;i++)if(!read_int(&p,0,highs[i],&h[i]))return 0;if(h[7]!=1)return 0;
 for(int i=0;i<count;i++){for(int j=0;j<NF;j++)if(!read_int(&p,mins[j],maxs[j],&lanes[i].f[j]))return 0;if(lanes[i].f[1]>lanes[i].f[0]||lanes[i].f[2]>=lanes[i].f[0])return 0;pattern(&lanes[i]);}
 if(!read_int(&p,0,1,&view)||!read_int(&p,0,1,&tool))return 0;
 for(int i=0;i<count;i++){Lane *l=&lanes[i];int seed;if(!read_int(&p,0,4,&l->playmode)||!read_int(&p,0,1,&l->auto_rotate)||!read_int(&p,-8,8,&l->shift)||!read_int(&p,1,16,&l->every)||!read_int(&p,1,INT_MAX,&seed)||!read_number(&p,&l->origin)||l->origin<0||l->origin>1e9)return 0;l->seed=(unsigned)seed;
  for(int j=0;j<32;j++)for(int k=0;k<5;k++){if(!read_int(&p,-1,k==0?1:k==1?127:k==2?100:k==3?8:150,&l->values[j][k]))return 0;if((k==1||k==3)&&l->values[j][k]==0)return 0;}}
 if(has_hold)for(int i=0;i<LANES;i++)if(!read_int(&p,0,1,&lanes[i].hold))return 0;
 if(slot_format){if(!read_int(&p,0,MOD_SLOTS-1,&selected_mod))return 0;for(int i=0;i<MOD_SLOTS;i++){int pad;if(!read_int(&p,0,LANES,&pad))return 0;pads[i]=pad-1;for(int n=0;n<i;n++)if(pads[i]>=0&&pads[n]==pads[i])return 0;if(!read_int(&p,-100,100,&mods[i].mix))return 0;for(int j=0;j<2;j++)for(int k=0;k<MOD_FIELDS;k++)if(!read_int(&p,mod_lo[k],mod_hi[k],&mods[i].osc[j].f[k]))return 0;}}
 int saved_scale=0,saved_start=36,scale_metadata=!strncmp(p,"SCALE ",6);if(scale_metadata){p+=6;if(!read_int(&p,0,15,&saved_scale)||!read_int(&p,0,127,&saved_start))return 0;}
 if(strcmp(p,"END"))return 0;
 if(!scale_metadata&&count==LANES){int candidate[LANES];for(int sc=0;sc<16;sc++){int first=sc?lanes[0].f[h[1]?9:8]:36;if(!scale_notes(sc,first,candidate))continue;int match=1;for(int i=0;i<LANES;i++)if(candidate[i]!=lanes[i].f[h[1]?9:8])match=0;if(match){saved_scale=sc;saved_start=first;break;}}}
 for(int i=count;i<LANES;i++)lanes[i].f[9]=drum_notes[h[3]*16+i];e->scale_choice=e->scale_applied_choice=saved_scale;e->scale_start=e->scale_applied_start=saved_start;e->scale_ui_pending=7;e->selected=h[0];e->mode=h[1];e->channel=0;e->bank=h[3];e->step=h[4];e->rhythm=h[5];e->row=h[6];e->edit_view=view;e->edit_tool=tool;memcpy(e->lane,lanes,sizeof lanes);normalize_config(e);mod_init(e);memcpy(e->mod,mods,sizeof mods);memcpy(e->mod_pad,pads,sizeof pads);e->mod_selected=selected_mod;for(int i=0;i<MOD_SLOTS;i++)mod_coefficients(e,i);e->dialog=0;e->q_valid=0;e->panic=1;e->valid=0;e->visual_ready=0;e->playing=0;for(int i=0;i<LANES;i++){e->reset_at[i]=-1;e->head[i]=-1;e->display_tick[i]=0;e->ui_offset[i]=0;}changed_all(e);return 1;
}

/* Two-way modulation without recursion. Both oscillators read the other one's
   previous control sample. This explicit <=1ms delay also resolves feedback.
   Raw source -> its own DEPTH/OFFSET -> the other oscillator's destination;
   MIX is never part of that route. Recorded bases remain immutable. */
static void mod_chain_point(Engine *e,int s,double phase[2],ModWalk walk[2],
                            double out[2][3],double hz[2],double previous[2],int preview){
 double raw[2],source[2];
 int lane=e->mod_pad[s];
 for(int j=0;j<2;j++){
  ModOsc *o=&e->mod[s].osc[j];unsigned seed=preview?0x74391u:(lane>=0?e->lane[lane].seed:0x1933u)^(unsigned)(j*0x29a583u);
  double p=phase[j]+o->f[7]/360.;
  raw[j]=o->f[1]==8?mod_walk_value(&walk[j],o,p,seed):mod_shape(o,p,seed);
  source[j]=o->f[0]?previous[j]:0.;
 }
 for(int j=0;j<2;j++){
  int other=1-j,dest=e->mod[s].osc[other].f[8];double v=source[other];
  hz[j]=e->mod_hz[s][j];out[j][0]=raw[j];out[j][1]=e->mod_depth[s][j];out[j][2]=e->mod_offset[s][j];
  if(e->mod[s].osc[other].f[0]){
   if(dest==MOD_CHAIN_RATE)hz[j]=fmax(.001,fmin(20.,hz[j]*exp2(v*4.)));
   else if(dest==MOD_CHAIN_DEPTH)out[j][1]=mod_clip(out[j][1]+v);
   else if(dest==MOD_CHAIN_OFFSET)out[j][2]=mod_clip(out[j][2]+v);
  }
 }
 for(int j=0;j<2;j++)previous[j]=e->mod[s].osc[j].f[0]?mod_clip(out[j][0]*out[j][1]+out[j][2]):0.;
}
static void mod_chain_fill(Engine *e,int s,double duration){
 double phase[2]={e->mod_phase[s][0],e->mod_phase[s][1]},out[2][3],hz[2];
 ModWalk walk[2]={e->mod_walk[s][0],e->mod_walk[s][1]};
 double previous[2]={e->mod_chain_source[s][0],e->mod_chain_source[s][1]};
 int run[2]={mod_running(e,s,0),mod_running(e,s,1)};
 if(!run[0]&&!run[1]){e->mod_chain_valid[s]=0;return;}
 int n=clampi((int)ceil(duration/.001),1,MOD_CHAIN_POINTS-1);
 e->mod_chain_duration[s]=n*.001;e->mod_chain_count[s]=n;
 for(int k=0;k<=n;k++){
  mod_chain_point(e,s,phase,walk,out,hz,previous,0);
  for(int j=0;j<2;j++){
   for(int f=0;f<3;f++)e->mod_chain_cache[s][j][k][f]=out[j][f];
   if(k<n&&run[j])phase[j]+=.001*hz[j];
  }
 }
 e->mod_chain_valid[s]=1;
}
static void mod_chain_advance(Engine *e,int s,double dt){
 double phase[2]={e->mod_phase[s][0],e->mod_phase[s][1]},out[2][3],hz[2];
 ModWalk walk[2]={e->mod_walk[s][0],e->mod_walk[s][1]};
 double previous[2]={e->mod_chain_source[s][0],e->mod_chain_source[s][1]};
 int run[2]={mod_running(e,s,0),mod_running(e,s,1)};
 if(!run[0]&&!run[1]){e->mod_chain_valid[s]=0;return;}
 int n=clampi((int)ceil(dt/.001),1,MOD_CHAIN_POINTS-1);double step=dt/n;
 for(int k=0;k<n;k++){
  mod_chain_point(e,s,phase,walk,out,hz,previous,0);
  for(int j=0;j<2;j++)if(run[j])phase[j]+=step*hz[j];
 }
 for(int j=0;j<2;j++){e->mod_phase[s][j]=phase[j];e->mod_walk[s][j]=walk[j];e->mod_chain_source[s][j]=previous[j];}
 mod_chain_fill(e,s,dt);
}
static void mod_chain_preview(Engine *e,int s){
 /* Edit-only four-bar trace: one bounded pass over the 128 native segments.
    Deliberately not a sample-accurate simulation: dense motion is an envelope,
    never a falsely aliased slow waveform. No work on playback-only updates. */
 double duration=16.*60./e->mod_bpm,phase[2]={0,0},out[2][3],hz[2];
 ModWalk walk[2]={0};double previous[2]={0,0};int last[2]={16,16};
 for(int j=0;j<2;j++)e->mod_chain_dense[s][j]=0;
 for(int k=0;k<=128;k++){
  mod_chain_point(e,s,phase,walk,out,hz,previous,1);
  for(int j=0;j<2;j++){
   int value=clampi((int)lround((mod_clip(out[j][0]*out[j][1]+out[j][2])+1)*15.5),0,31);
   int wave=e->mod[s].osc[j].f[1];int dense=duration/128.*hz[j]>.25&&(wave<=4||wave==7||wave==10);
   if(dense)e->mod_chain_dense[s][j]=1;
   if(k)e->mod_trace_cache[s][j][k-1]=dense?31:last[j]*32+value;
   last[j]=value;phase[j]+=duration/128.*hz[j];
  }
 }
}

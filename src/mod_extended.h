/* Extended control generators. Own MIT implementation, inspired by documented
   Lunar behaviours; no audio engine or hardware dependency. All loops bounded. */
static double mod_clip(double v){return fmax(-1.,fmin(1.,v));}
static double mod_draw(unsigned seed,uint64_t cycle){return mix32(seed^(unsigned)cycle^mix32((unsigned)(cycle>>32)))/4294967295.*2.-1.;}
static double mod_walk_value(ModWalk *w,const ModOsc *o,double phase,unsigned seed){
 int64_t c=(int64_t)floor(fmax(0.,phase));double p=phase-floor(phase);
 if(!w->ready||w->seed!=seed||c<w->cycle||c-w->cycle>1024){
  w->cycle=c;w->seed=seed;w->ready=1;
  w->previous=mod_draw(seed^0x715a39u,(uint64_t)c);
  w->next=w->previous;
 }
 double step=.01+.99*(o->extra[0]+100)/200.;
 /* Next target is a reflected random step, not a new full-range draw. */
 if(w->next==w->previous){double x=w->previous+step*mod_draw(seed^0x35a791u,(uint64_t)w->cycle);w->next=x>1?2-x:x< -1?-2-x:x;}
 while(w->cycle<c){w->previous=w->next;w->cycle++;double x=w->previous+step*mod_draw(seed^0x35a791u,(uint64_t)w->cycle);w->next=x>1?2-x:x< -1?-2-x:x;}
 return mod_clip(w->previous+(w->next-w->previous)*p*p*(3-2*p));
}
static double mod_loop_value(const ModOsc *o,double phase,unsigned seed){
 int length=clampi(1+(int)floor(31.*(o->extra[0]+100)/200.),1,32);
 uint64_t c=(uint64_t)floor(fmax(0.,phase));unsigned pos=(unsigned)(c%(unsigned)length);
 double evolution=(o->extra[1]+100)/200.;
 /* Each position evolves on its own deterministic schedule. At zero, the
    exact sequence repeats forever. No frame-rate-dependent random draws. */
 uint64_t generation=c/(unsigned)length;
 unsigned salt=mix32(seed^pos^0x5de891u);
 uint64_t epoch=(uint64_t)floor(generation*evolution+salt/4294967296.);
 return mod_draw(seed^mix32(pos+0x79a31u),epoch);
}
static double mod_bounce_value(const ModOsc *o,double p){
 double restitution=.25+.65*(o->extra[1]+100)/200.;
 double gravity=.25+3.75*(o->extra[0]+100)/200.;
 p*=sqrt(gravity);if(p>=1.)return -1.;
 double first=(1.-restitution)/(1.-pow(restitution,8.));
 double start=0.,width=first,height=1.;
 for(int k=0;k<8;k++){
  if(p<start+width||k==7){double x=fmax(0.,fmin(1.,(p-start)/width));return 2*height*fmax(0.,4*x*(1-x))-1.;}
  start+=width;width*=restitution;height*=restitution*restitution;
 }
 return -1.;
}

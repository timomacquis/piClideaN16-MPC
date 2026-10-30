#define main unused_mod_test_main
#include "mod_engine_test.c"
#undef main
int main(int argc,char **argv){
 Engine *e=fresh();put(e,"mod_pad",3);put(e,"mod0_on",1);put(e,"mod1_on",1);
 put(e,"mod0_wave",1);put(e,"mod0_dest",7);put(e,"mod1_wave",argc>1?atoi(argv[1]):9);put(e,"mod1_dest",8);mod_advance(e,48,120);mod_preview(e,0);
 printf("mod_pad 3\nmod0_on 1\nmod1_on 1\nmod0_wave 1\nmod0_dest 7\nmod1_wave %d\nmod1_dest 8\n",e->mod[0].osc[1].f[1]);
 char key[64],value[64];for(int j=0;j<2;j++){
  for(int k=0;k<8;k++){snprintf(key,sizeof key,"mod%d_c%d",j,k);get_param(e,key,value,sizeof value);printf("%s %s\n",key,value);}
  printf("mod_stairs%d %d\n",j,mod_style(e,j));
  for(int k=0;k<128;k++)printf("g%d_%d %d\n",j*4+k/32,k%32,mod_segment(e,0,j,k));
 }
 done(e);return 0;
}

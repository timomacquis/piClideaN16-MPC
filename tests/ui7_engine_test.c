#define main previous_tests_main
#include "mod_engine_test.c"
#undef main
int main(void){Engine *e=fresh();for(int line=0;line<16;line++){char key[32];snprintf(key,sizeof key,"line_select16");put(e,key,line);for(int mode=0;mode<5;mode++){put(e,"open_bank",1);assert(e->dialog==3);char v[16];get_param(e,"map_sound",v,sizeof v);assert(atoi(v)==e->lane[line].playmode);put(e,"map_sound",mode);assert(e->dialog==0&&e->lane[line].playmode==mode);}int before=e->lane[line].playmode;put(e,"open_bank",1);put(e,"map_sound",7);assert(e->dialog==3&&e->lane[line].playmode==before);put(e,"close",1);assert(e->dialog==0&&e->lane[line].playmode==before);}
for(int i=0;i<16;i++){e->lane[i].f[12]=i%2;e->lane[i].f[13]=1;}for(int n=0;n<200;n++){put(e,"open_target",1);put(e,"apply_rhythm",1);for(int i=0;i<16;i++)assert(e->lane[i].f[12]==i%2&&!e->lane[i].f[13]);}
e->edit_view=0;Lane copy=e->lane[e->selected];put(e,"randomize",1);put(e,"sel_velocity",12);put(e,"sel_rotate",2);assert(!memcmp(&copy,&e->lane[e->selected],sizeof copy));put(e,"randomize",1);done(e);puts("PASS all modes on 16 lanes / CANCEL + invalid selection / 200 Rd ALL keep per-lane MUTE / RAND selection cannot edit values");return 0;}

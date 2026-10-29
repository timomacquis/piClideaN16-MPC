/* UI state tests, no ALSA device opened. */
#define main existing_mod_tests_main
#include "mod_engine_test.c"
#undef main
static void drain_panel(Engine *e){char k[64];int n=0;while(mpc_engine_changed(e,k,sizeof k)>0){assert(++n<20000);}}
int main(void){Engine *e=fresh();e->scale_start=e->scale_applied_start=36;changed_all(e);visual_snapshot(e,0,1);drain_panel(e);assert(!scale_pending(e));
Lane before[16];memcpy(before,e->lane,sizeof before);put(e,"choose_bank",1);assert(e->dialog==5&&e->changes<=3&&e->visual_ready);put(e,"choose_bank",0);assert(e->dialog==5);put(e,"close",0);assert(e->dialog==5);put(e,"close",1);assert(e->dialog==0&&!scale_pending(e));drain_panel(e);
put(e,"choose_bank",1);put(e,"choose_sound",6);assert(e->dialog==0&&scale_pending(e)==1);assert(!memcmp(before,e->lane,sizeof before));put(e,"edit_row",40);assert(scale_pending(e)==3);put(e,"choose_sound",0);assert(scale_pending(e)==2);put(e,"edit_row",36);assert(!scale_pending(e));
put(e,"choose_sound",6);put(e,"apply_mapping",1);assert(!scale_pending(e));put(e,"edit_row",37);assert(scale_pending(e)==2);put(e,"edit_row",36);assert(!scale_pending(e));put(e,"edit_row",127);memcpy(before,e->lane,sizeof before);put(e,"apply_mapping",1);assert(scale_pending(e)==2&&!memcmp(before,e->lane,sizeof before));
put(e,"open_mapping",1);assert(!scale_pending(e)&&e->scale_start==36&&e->scale_choice==0);assert(e->visual_ready);visual_snapshot(e,2048,0);drain_panel(e);for(int j=0;j<1000;j++){visual_snapshot(e,2048,0);assert(!e->changes&&!e->scale_ui_pending);}
for(int scale=0;scale<16;scale++){put(e,"choose_bank",1);put(e,"choose_sound",scale);assert(e->dialog==0&&e->scale_choice==scale&&e->visual_ready);visual_snapshot(e,2048,0);drain_panel(e);}
done(e);puts("PASS scale panel open/close/select / separate pending colours / APPLY/RESET / overflow retains pending / no global invalidation / idle emits no notifications");return 0;}

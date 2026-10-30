"""Generate the candidate model. All runtime identifiers and paths are ASCII."""
from pathlib import Path
import json
from project_config import DISPLAY_NAME
B=Path(__file__).resolve().parents[1]
P=[]
def add(key,name,lo=0,hi=1,default=0,**kw):
 d=dict(key=key,name=name,min=lo,max=hi,default=default,display='int');d.update(kw);P.append(d)
def choice(key,name,opts,default=0,**kw):
 add(key,name,default=default,options=opts,**kw)
def trig(key,name,**kw):
 if key in ('half','double','randomize','erase_values','undo','redo'):kw.setdefault('hold_ms',100)
 add(key,name,momentary=True,**kw)
sounds=['KICK','MBRANE','CL HAT','OP HAT','CLAP','RIM','CRASH','RIDE','X1','X2','FM','CUSTOM']
add('line','Line',0,7,0,refresh_display=True,nudge_pct=10)
choice('target_mode','Target',['DRUM INSTRUMENT','DRUM TRACK'],default=1,refresh_display=True)
choice('channel','Channel',[str(i) for i in range(1,17)],refresh_display=True)
choice('bank','Bank',list('ABCDEFGH'),refresh_display=True)
add('selected_step','Selected step',0,31,0)
add('rhythm_choice','Rhythm',0,5,0,dynamic_display=True)
add('dialog','Dialog',0,6,0,poll=False)
add('edit_row','START',0,127,36,dynamic_display=True)
choice('map_sound','Sound',sounds,refresh_display=True)
for k in ['open_rhythm','open_mapping','open_target','open_bank','close','apply_rhythm','apply_mapping','randomize','half','double']:
 trig(k,k.replace('_',' ').upper(),refresh_display=k not in ('randomize','half','double','open_rhythm','apply_rhythm'))
trig('prev_line','Previous line',step_of='line',step_delta=-1)
trig('next_line','Next line',step_of='line',step_delta=1)
fields=[('steps',1,32,16),('hits',0,32,4),('rotate',0,31,0),('division',0,4,1),('velocity',1,127,112),('humanize',0,50,0),('prob',0,100,100),('gate',10,95,50)]
for key,lo,hi,de in fields:
 if key=='division':choice('sel_'+key,key.upper(),['1/32','1/16','1/8','1/4','1/2'],de)
 else:add('sel_'+key,'PROBABILITY' if key=='prob' else key.upper(),lo,hi,de,nudge_pct=10 if key in ('steps','hits','rotate') else 0,unit='%' if key in ('prob','gate') else '')
for k in ['sel_mute','sel_solo']:add(k,k[4:].upper(),0,1,0)
for i in range(8):
 for key,lo,hi,de in fields:
  de = [4,2,11,3,4,5,1,3][i] if key=='hits' else (4 if i==1 else 0) if key=='rotate' else (112 if i==0 else 100) if key=='velocity' else de
  add(f'l{i}_{key}',f'L{i+1} {key}',lo,hi,de)
 for key,lo,hi,de in [('inote',0,127,36+i),('dnote',0,127,36+i),('pad',0,15,i),('sound',0,11,i),('mute',0,1,0),('solo',0,1,0)]:
  if key=='sound':choice(f'l{i}_{key}',f'L{i+1} sound',sounds,de,refresh_display=True)
  elif key=='pad':choice(f'l{i}_{key}',f'L{i+1} pad',[f'{j+1:02}' for j in range(16)],de,refresh_display=True)
  else:add(f'l{i}_{key}',f'L{i+1} {key}',lo,hi,de,refresh_display=key in ('inote','dnote'))
 trig(f'edit_line{i}',f'Line {i+1}',refresh_display=True)
 trig(f'open_sound{i}',f'Choose sound {i+1}',refresh_display=True)
for i in range(32):
 trig(f'step_press{i}',f'Select step {i+1}',refresh_display=False)
 add(f'cell{i}',f'Step {i+1} state',0,9,0,poll=False)
for i in range(8):
 for j in range(32):add(f'g{i}_{j}',f'Retired grid / static MOD segment {i*32+j+1}',0,3071,528,poll=False)
for k,n in [('line_label','Selected sound'),('target_label','MIDI NOTE'),('status','MIDI status'),('step_label','Selection'),('mapping_label','PAD'),('port_label','MODE')]+[(f'name{i}',f'Lane {i+1} sound') for i in range(8)]:
 add(k,n,display='string',poll=k=='status')
for i in range(32):add(f'preview{i}',f'Rhythm preview {i+1}',0,4,0,poll=False)
# Append only: preserve every published candidate.1 index, including retired arrows.
choice('line_select','Selected rhythm lane',[str(i+1) for i in range(8)],refresh_display=True)
add('pattern_base','Pattern base visible',0,1,1,poll=False)
add('setup_base','Setup base visible',0,1,1,poll=False)
for i in range(8):add(f'line_on{i}',f'Lane {i+1} selected',0,1,int(i==0),poll=False)
add('choose_target','Scale pending fields',0,3,0,poll=False)
trig('choose_bank','Open scales',refresh_display=False,hold_ms=100)
choice('choose_sound','SCALES',['DRUM','Major','Natural minor','Harmonic minor','Melodic minor','Major pentatonic','Minor pentatonic','Blues','Chromatic','Ionian mode','Dorian mode','Phrygian mode','Lydian mode','Mixolydian mode','Aeolian mode','Locrian mode'])
# Evolution: append only; legacy indices and ranges stay unchanged.
choice('edit_view','Pattern view',['LINE','EDIT'],default=1,refresh_display=True)
add('edit_tool','Step tool',0,1,0,dynamic_display=True)
for k,n,lo,hi,de in [('step_velocity','VELOCITY',1,127,100),('step_prob','PROBABILITY',0,100,100),('step_ratchet','RATCHET',1,8,1),('step_decay','DECAY',0,150,100)]:
 add(k,n,lo,hi,de,unit='%' if k in ('step_prob','step_decay') else 'x' if k=='step_ratchet' else '')
for key in ['erase_values','undo','redo','reset_phase']:trig(key,key.replace('_',' ').upper(),refresh_display=False)
add('play_mode','Playback mode',0,4,0,dynamic_display=True)
add('auto_rotate','Auto rotate',0,1,0)
add('auto_shift','Shift',-8,8,1)
add('auto_every','Every cycles',1,16,1)
for key in ['can_undo','can_redo']:add(key,key,0,1,0,poll=False)
for k in ['step_scope','line_scope','phase_label']+[f'step_inherit{i}' for i in range(4)]:add(k,k,display='string',poll=False)
# New cell readouts carry purple independently of hit/selection/head. Retain old cell ranges.
for i in range(32):add(f'editcell{i}',f'Edit cell {i+1}',0,17,0,poll=False)
for i in range(8):
 for key,lo,hi,de in [('playmode',0,4,0),('auto',0,1,0),('shift',-8,8,1),('every',1,16,1)]:add(f'l{i}_{key}',f'L{i+1} {key}',lo,hi,de)
 for step in range(32):
  for key,hi in [('trig',1),('vel',127),('prob',100),('ratchet',8),('decay',150)]:
   add(f'l{i}_s{step}_{key}',f'L{i+1} step {step+1} {key}',-1,hi,-1)
for q in range(8):add(f'q{q}',f'Q{q+1}',0,127,0,dynamic_name=True,dynamic_display=True,display='float',poll=False)
add('phase_cycle','CYCLE',1,2147483647,1,poll=False)
add('phase_offset','SHIFT',0,31,0,poll=False)
for i in range(4):add(f'step_custom{i}',f'Step custom {i}',0,1,0,poll=False)
add('step_position','Selected visible step',1,32,1,poll=False)
for i in range(8):
 for j in range(32):add(f'trackcell{i}_{j}',f'Track {i+1} step {j+1}',0,8,0,poll=False)
for k in ['sel_mute','sel_solo','auto_rotate']+[f'l{i}_{kind}' for i in range(8) for kind in ['mute','solo']]:trig('toggle_'+k,'Toggle '+k)
# Sixteen-lane extension: preserve the entire published 2196-key prefix.
old_lanes=[dict(p) for p in P if p['key'].startswith('l0_')]
for i in range(8,16):
 for template in old_lanes:
  d=dict(template);d['key']='l'+str(i)+'_'+template['key'][3:];d['name']=template['name'].replace('L1 ',f'L{i+1} ',1)
  suffix=template['key'][3:]
  if suffix=='hits':d['default']=0
  elif suffix=='rotate':d['default']=0
  elif suffix in ('inote','dnote'):d['default']=36+i
  elif suffix=='pad':d['default']=i
  elif suffix=='sound':d['default']=min(i,11)
  elif suffix=='velocity':d['default']=100
  P.append(d)
 trig(f'edit_line{i}',f'Line {i+1}',refresh_display=True)
 trig(f'open_sound{i}',f'Choose sound {i+1}',refresh_display=True)
 add(f'name{i}',f'Lane {i+1} sound',display='string',poll=False)
 add(f'line_on{i}',f'Lane {i+1} selected',0,1,0,poll=False)
 for j in range(32):add(f'trackcell{i}_{j}',f'Track {i+1} step {j+1}',0,8,0,poll=False)
 for kind in ['mute','solo']:trig(f'toggle_l{i}_{kind}',f'Toggle line {i+1} {kind}')
add('line_select16','Selected lane',0,15,0,refresh_display=True,nudge_pct=10)
# Append-only HOLD flags; all 3925 published definitions remain unchanged.
for i in range(16):add(f'l{i}_hold',f'L{i+1} note hold',0,1,0)
# Playback indicators are read-only visual parameters, never automation targets.
for i in range(16):add(f'lane_flash{i}',f'Lane {i+1} activity',0,1,0,poll=False)
# Transient selective-RAND controls: append only; no automation or project state.
add('rand_mode','RAND selection mode',0,1,0,poll=False)
for i in range(8):add(f'rand_pick{i}',f'RAND selected parameter {i+1}',0,1,0,poll=False)
for i in range(8):trig(f'rand_toggle{i}',f'Choose RAND parameter {i+1}')
# E01: expose contextual controls only. Full sixteen-lane MOD configuration
# lives in PICN5; Force 3.10 skin handles stop at Parameter 4095.
mod_specs=[('on',0,1,0),('wave',0,10,0),('sync',0,1,1),('rate',0,6,2),('hz',0,1000,500),('depth',-100,100,50),('offset',-100,100,0),('phase',0,359,0),('dest',0,10,0),('threshold',-100,100,0)]+[(f'c{k}',-100,100,round(100*__import__('math').sin(k*__import__('math').tau/8))) for k in range(8)]
mod_options={'wave':['SINE','TRIANGLE','RAMP UP','RAMP DOWN','SQUARE','SAMPLE & HOLD','RANDOM SMOOTH','FREE CURVE','RANDOM WALK','RANDOM LOOP','BOUNCE'],'sync':['HZ','SYNC'],'rate':['1/16','1/8','1/4','1/2','1 BAR','2 BARS','4 BARS'],'dest':['NONE','VELOCITY','PROBABILITY','HUMANIZE','GATE','RATCHET','DECAY','NOTE ON','OTHER LFO RATE','OTHER LFO DEPTH','OTHER LFO OFFSET']}
for lane in [16]:
 prefix=f'l{lane}_' if lane<16 else ''
 add(prefix+'mod_mix',prefix+'LFO MIX',-100,100,0)
 for j in range(2):
  for key,lo,hi,de in mod_specs:
   k=prefix+f'mod{j}_'+key
   if key in mod_options:choice(k,k,([*mod_options[key][:8]]+[f'LFO {2-j} RATE',f'LFO {2-j} DEPTH',f'LFO {2-j} OFFSET']) if key=='dest' else mod_options[key],de,refresh_display=key in ('wave','sync','dest'))
   else:add(k,k,lo,hi,de,dynamic_display=key=='hz' or key.startswith('c'),refresh_display=key=='on')
for i in range(16):add(f'mod_indicator{i}',f'Lane {i+1} modulation',0,1,0,poll=False)
# Reuse only unpublished E01 visual indices; stable 1.2.2 indices unchanged.
for j in range(4):trig(f'mod_choose{j}',f'Choose MOD {j+1}',refresh_display=True,hold_ms=100)
choice('mod_pad','PAD',['NONE']+[f'A{i+1:02}' for i in range(16)],refresh_display=True)
for j in range(2):
 for k in range(32):
  if k==30:add(f'mod_stairs{j}',f'LFO {j+1} static render style',0,2,0,poll=False)
  elif j==0 and k==31:add('mod_slot','Selected MOD slot',0,3,0,poll=False)
  elif j==1 and k==31:add('mod_tempo','Preview window',0,1000,120,poll=False,dynamic_display=True)
  else:add(f'mod_trace{j}_{k}',f'LFO {j+1} static segment {k+1}',0,1023,528,poll=False)
for param in P:
 if __import__('re').fullmatch(r'l[0-9]+_dnote',param['key']):param['dynamic_display']=True
 if param['key'] in ('open_mapping','apply_mapping'):param['refresh_display']=False;param['hold_ms']=100
for param in P:
 if param['key'].startswith(('mod0_','mod1_','mod_choose')) or param['key'] in ['mod_pad','open_rhythm','open_target','open_bank','close','map_sound'] or param['key'].startswith('open_sound'):param['refresh_display']=False
 if param['key']=='can_undo':param.update(min=0,max=7,poll=False)
 if param['key']=='map_sound':param.pop('options',None);param.update(min=0,max=10,poll=False)
 if param['key']=='close':param['hold_ms']=100
(B/'params.json').write_text(json.dumps(P,ensure_ascii=True,indent=2)+'\n')
(B/'vst.json').write_text(json.dumps(dict(name=DISPLAY_NAME,vendor='Timo',uid='PiCN',version=10300,so='piclidean_mpc.so',params='params.json',custom_skin=True,cc=False,nrpn=False,defines={'HAS_SONG_POS':1,'HAS_DISPLAY_REV':0,'DISPLAY_REV_NO_UPDATE':1,'PARAM_TEXT_MAX':64,'PICN_HOST_CONTEXT':1,'PICN_NOTIFICATION_BUDGET':8,'MPC_CHUNK_BYTES':32768}),indent=2)+'\n')
ref=json.loads((B/'reference/DRUMTRACK-REFERENCE.json').read_text())
(B/'src/drum_notes.h').write_text('/* Local reference-project mapping, not universal hardware discovery. */\nstatic const unsigned char drum_notes[128]={'+','.join(map(str,ref['notes']))+'};\n')
(B/'build/reports').mkdir(parents=True,exist_ok=True)
(B/'build/reports/PARAMETERS.json').write_text(json.dumps({'count':len(P),'keys':[p['key'] for p in P]},indent=2))
print('Generated',len(P),'parameters')

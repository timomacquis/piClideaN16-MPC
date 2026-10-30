"""Generate five native MPC pages for the sixteen-lane sequencer."""
from pathlib import Path
import json,zipfile,copy,hashlib,math,shutil,sys,re
from functools import lru_cache
from PIL import Image,ImageDraw,ImageFont
from project_config import FOLDER, SKIN_IDENTIFIER, VERSION
from provenance import inputs,tree,timestamp
B=Path(__file__).resolve().parents[1];P=json.loads((B/'params.json').read_text());I={p['key']:i for i,p in enumerate(P)}
assert max(I.values())<4096, 'Force native parameter handles must fit indices 0..4095'
OUT=B/'build/skin'/FOLDER;A=OUT/'Plugin Skins'
if OUT.exists():shutil.rmtree(OUT)
A.mkdir(parents=True)
z=zipfile.ZipFile(B/'reference/skin-base.zip');assert hashlib.sha256((B/'reference/skin-base.zip').read_bytes()).hexdigest()==(B/'reference/skin-base.zip.sha256').read_text().split()[0]
J=json.loads(z.read('Plugin Skins/TUI.json'));D=J['pageData'];D['componentDefinitions']['localComponentDefinitions']=[];defs=D['componentDefinitions']['localComponentDefinitions']
C={'bg':'#171a1e','panel':'#252b31','oled':'#111b18','text':'#e8e5d0','muted':'#b4bcba','red':'#f2474d','amber_bg':'#342e22','amber_text':'#c5ac78','amber_edge':'#8b7854'}
READ_ONLY_TEXT='#6b756f'
CORNER_RADIUS=10  # Native 1280px coordinates; artwork only, hit areas unchanged.
def rounded(d,box,fill=None,outline=None,width=1,radius=CORNER_RADIUS):
 d.rounded_rectangle(box,radius=radius,fill=fill,outline=outline,width=width)
@lru_cache(maxsize=32)
def font(n):return ImageFont.truetype(str(B/'reference/fonts/TitilliumWeb-Regular.ttf'),n)
def text(d,xy,t,n=23,c=None,anchor=None):d.text(xy,t,font=font(n),fill=c or C['text'],anchor=anchor)
def image(w,h,bg=None):im=Image.new('RGB',(w,h),bg or C['bg']);return im,ImageDraw.Draw(im)
def save(name,im):im.save(A/name)
def bounds(x,y,w,h,extra=None):return dict(version=2,acceptsHWFocus='Yes',showWhenDataModelInvalid='Hide',whenVisible='Always',boundsType='Absolute',bounds=f'{x} {y} {w} {h}',additionalInvalidatingHandles=extra or [])
def child(kind,data,x,y,w,h,name='Control'):
 return {'version':2,'componentData':{'version':1,'name':name,'type':kind,'data':data},'handle remapping':{'version':1,'map':[]},'bounds':bounds(x,y,w,h)}
def action(handler):return {'version':2,'onAction':'Mouse Down','handler':handler,'handleName':'Data','additionalData':'','handle remapping':{'version':1,'map':[]}}
def local(name,children,actions=None,focus=False):
 clear={'version':1,'colour':'0','image':''};v=dict(version=4,actions=actions or [],backgroundData=dict(version=1,focussed=clear,unfocussed=clear),ignoreMousePresses=not bool(actions),disableCoarseDataWheel=False,repeats=1,hideQLinkBounds=True,componentsData=children)
 if focus:
  boxes=[list(map(int,c['bounds']['bounds'].split())) for c in children];w=max(x+cw for x,y,cw,ch in boxes);h=max(y+ch for x,y,cw,ch in boxes)
  # Passive transparent focus artwork: retain native focus visibility and actions.
  fn=f'focus_rounded_{w}_{h}.png'
  if not (A/fn).exists():
   im=Image.new('RGBA',(w,h),(0,0,0,0));rounded(ImageDraw.Draw(im),(2,2,w-3,h-3),outline='#ffffff',width=2);save(fn,im)
  c=art(fn,0,0,w,h);c['componentData']['name']='Focus';c['bounds']['whenVisible']='WhenFocussed';children.append(c)
 defs.append({'key':name,'value':v});return name
def placed(kind,key,x,y,w,h,extra=None,maps=None):
 c=child(kind,dict(version=1,handleName='Data'),x,y,w,h,key);c['handle remapping']['map']=[{'key':'Data','value':f'Parameter {I[key]}'}]+(maps or []);c['bounds']['additionalInvalidatingHandles']=extra or [];return c
def gate(key,val,n):return f'IndexedEnabling/{val}/{n}/Parameter {I[key]}'
def art(name,x,y,w,h):
 c=child('Image',dict(version=2,imageType='Regular',colour='0',image=name),x,y,w,h);c['bounds']['acceptsHWFocus']='No';return c
def label(x,y,w,h,n=24,handle='Data',kind='Value',colour='ffe8e5d0'):
 c=child('Label',dict(version=1,textStyle=dict(version=1,font=dict(version=1,name='Titillium Web',style='Regular',height=float(n)),colour=colour,justification='left verticallyCentred',case='Original'),type=kind,handleName=handle),x,y,w,h);c['bounds']['acceptsHWFocus']='No';return c
def readout(key,x,y,w,h,n=24):
 k=local(f'read_{key}_{x}_{y}',[label(0,0,w,h,n)]);c=placed(k,key,x,y,w,h);c['bounds']['acceptsHWFocus']='No';return c
def button(key,title,x,y,w,h,momentary=True,id=1,count=2,extra=None,tag=None,native_touch=False):
 tag=tag or f'btn_{key}_{x}_{y}_{id}'
 for state in [0,1]:
  im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=C['amber_bg'] if state else C['panel'],outline=C['amber_edge'] if state else '#727d81');text(d,(w//2,h//2),title,21,C['amber_text'] if state else C['text'],'mm');save(tag+f'_{state}.png',im)
 if native_touch:
  # A single touch owner, matching the working step-pad triggers. A nested
  # Momentary Button can retain its pressed gesture until focus moves away.
  # The wrapper already sends the trigger's release; artwork follows Data.
  children=[]
  for state in [0,1]:
   c=art(tag+f'_{state}.png',0,0,w,h)
   c['bounds']['additionalInvalidatingHandles']=[gate(key,state,2)]
   children.append(c)
  k=local(tag,children,[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True)
 else:
  k=local(tag,[child('Button',dict(version=2,onImage=tag+'_1.png',offImage=tag+'_0.png',buttonId=id,numButtonsInGroup=count,handleName='Data',gestureBehaviour='Momentary' if momentary else 'Instant'),0,0,w,h)],[action('Q-Link'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True)
 if not momentary:
  for element in defs[-1]['value']['componentsData']:
   if element['componentData']['name']=='Focus':element['bounds']['additionalInvalidatingHandles']=[gate(key,id,count)]
 return placed(k,key,x,y,w,h,extra)
def toggle(key,title,x,y,w,h):
 # One native Toggle Switch owns touch/Enter and binds the actual state.
 # No momentary proxy and no child Button that could toggle a second time.
 children=[]
 for current in [0,1]:
  tag=f'toggle_{key}_{x}_{y}_{current}'
  im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=C['amber_bg'] if current else C['panel'],outline=C['amber_edge'] if current else '#727d81')
  caption=('ON' if current else 'OFF') if re.fullmatch(r'mod[01]_on',key) else ('HOLD' if current else 'GATE') if key.endswith('_hold') else title+(' ON' if current else ' OFF') if key=='auto_rotate' else title
  text(d,(w//2,h//2),caption,21,C['amber_text'] if current else C['text'],'mm');save(tag+'.png',im)
  c=art(tag+'.png',0,0,w,h);c['bounds']['additionalInvalidatingHandles']=[gate(key,current,2)];children.append(c)
 actions=[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')]
 return [placed(local(f'toggle_native_{key}_{x}_{y}',children,actions,focus=True),key,x,y,w,h)]
def field(key,x,y,w,h,n=26):
 tag=f'field_{w}_{h}';fn=tag+'.png'
 if not (A/fn).exists():
  im=Image.new('RGB',(w,h*128),C['bg']);dr=ImageDraw.Draw(im)
  for f in range(128):rounded(dr,(1,f*h+1,w-2,(f+1)*h-2),fill=C['panel'],outline='#727d81')
  save(fn,im)
 parts=[child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=fn,numFrames=127,invert=False,dragOrientation='Vertical',handleName='Data'),0,0,w,h),label(12,0,w-24,h,n)]
 k=local(f'field_{key}',parts,[action('Q-Link')],focus=True);return placed(k,key,x,y,w,h)
def rand_button(x,y,w,h):
 children=[]
 for armed in [0,1]:
  im,d=image(w,h)
  rounded(d,(1,1,w-2,h-2),fill='#1a3324' if armed else C['panel'],outline='#65bc7b' if armed else '#727d81')
  if armed:
   # Shear the glyph mask to italic without requiring an additional font file.
   mask=Image.new('RGBA',(80,30),(0,0,0,0));md=ImageDraw.Draw(mask)
   text(md,(8,0),'RAND',21,'#a0d8ad')
   mask=mask.transform(mask.size,Image.Transform.AFFINE,(1,-.18,4,0,1,0),resample=Image.Resampling.BICUBIC)
   im.paste(mask,((w-80)//2,(h-30)//2),mask)
   d.ellipse((w-17,9,w-13,13),fill='#ffffff')
  else:text(d,(w//2,h//2),'RAND',21,C['text'],'mm')
  fn=f'rand_mode{armed}.png';save(fn,im)
  c=art(fn,0,0,w,h);c['bounds']['additionalInvalidatingHandles']=[gate('rand_mode',armed,2)];children.append(c)
 actions=[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')]
 return placed(local('rand_two_stage',children,actions,focus=True),'randomize',x,y,w,h)
# Shared knob filmstrip, exact image/control geometry.
im=Image.new('RGB',(58,58*128),C['oled']);dr=ImageDraw.Draw(im)
for v in range(128):
 y=v*58;dr.ellipse((4,y+4,54,y+54),outline=C['text'],width=2);a=math.radians(-135+270*v/127);dr.line((29,y+29,29+math.sin(a)*20,y+29-math.cos(a)*20),fill=C['text'],width=3)
save('dial.png',im)
# Read-only reference strip: same frames/geometry, attenuated foreground only.
reference=Image.new('RGB',im.size,C['oled'])
reference=Image.blend(reference,im,.35)
save('dial_read_only.png',reference)
# Native 1280 x 628 content: MPC owns the header and bottom tabs.
im,dr=image(1280,628);save('pattern.png',im)
parts=[art('pattern.png',0,0,1280,628)]
for i in range(16):
 x=20+(i%8)*156;w=148;h=46;tag=f'lane_select_{i}';children=[]
 # Exactly one native selection owner is visible per lane. Explicit Mute
 # overrides activity immediately; Solo exclusions retain the usual artwork.
 for muted,flash in [(0,0),(0,1),(1,0)]:
  for selected in [0,1]:
   bg=('#2b2529' if selected else '#1b2023') if muted else '#203c49' if flash else '#352329' if selected else C['panel']
   ink=('#ad747b' if selected else '#858d91') if muted else '#91c9df' if flash else C['red'] if selected else C['text']
   edge='#ffffff' if selected else '#333b40' if muted else '#5b9cb7' if flash else '#485057'
   im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=bg,outline=edge,width=2 if selected else 1)
   text(d,(w//2,h//2),f'A{i+1:02}',24,ink,'mm');save(tag+f'_m{muted}_f{flash}_{selected}.png',im)
  native=child('Button',dict(version=2,onImage=tag+f'_m{muted}_f{flash}_1.png',offImage=tag+f'_m{muted}_f{flash}_0.png',buttonId=i,numButtonsInGroup=16,handleName='Data',gestureBehaviour='Instant'),0,0,w,h)
  conditions=[gate(f'l{i}_mute',muted,2)]
  if not muted:conditions.append(gate(f'lane_flash{i}',flash,2))
  native['bounds']['acceptsHWFocus']='No';native['bounds']['additionalInvalidatingHandles']=conditions;children.append(native)
 kind=local(tag,children,[action('Q-Link')]);parts.append(placed(kind,'line_select16',x,12+(i//8)*52,w,h))
parts += [button('edit_view','LINE',20,394,148,46,False,0,2),button('edit_view','EDIT',176,394,148,46,False,1,2)]
for view,mutepos,solopos in [(0,332,488),(1,488,644)]:
 for control in toggle('sel_mute','MUTE',mutepos,394,148,46)+toggle('sel_solo','SOLO',solopos,394,148,46):
  control['bounds']['additionalInvalidatingHandles'].append(gate('edit_view',view,2));parts.append(control)
for control in [button('half','1/2',644,394,148,46),button('double','x2',800,394,148,46),rand_button(956,394,148,46),button('open_target','Rd ALL',1112,394,148,46,native_touch=True)]:
 control['bounds']['additionalInvalidatingHandles'].append(gate('edit_view',0,2));parts.append(control)
parts += [button('edit_tool','SELECT',800,394,148,46,False,0,2,extra=[gate('edit_view',1,2)]),button('edit_tool','DRAW',956,394,148,46,False,1,2,extra=[gate('edit_view',1,2)]),button('erase_values','ERASE',1112,394,148,46,extra=[gate('edit_view',1,2)])]
# Opaque display artwork, headers and four controls per scope.
im,dr=image(1240,264);rounded(dr,(1,1,1238,262),fill=C['oled'],outline='#68736a',width=2,radius=12);dr.line((8,131,1230,131),fill='#68736a',width=1);save('scopes.png',im);parts.append(art('scopes.png',20,120,1240,264))
im,d=image(296,94,C['oled']);rounded(d,(1,1,294,92),outline='#68736a');save('parameter_box.png',im)
im,d=image(296,94,C['oled']);rounded(d,(1,1,294,92),outline='#303f36');save('parameter_box_read_only.png',im)
for view,keys in [(0,['sel_'+k for k in ['velocity','prob','humanize','gate','steps','hits','rotate','division']]),(1,['sel_velocity','sel_prob','sel_humanize','sel_gate','step_velocity','step_prob','step_ratchet','step_decay'])]:
 for title,y in [('LINE',120),('STEP' if view else 'STEPS',252)]:
  im,d=image(720,26,C['oled']);text(d,(0,0),title,20,READ_ONLY_TEXT if view and y==120 else C['text']);fn=f'scope_v{view}_{y}.png';save(fn,im);c=art(fn,34,y+4,720,26);c['bounds']['additionalInvalidatingHandles']=[gate('edit_view',view,2)];parts.append(c)
 for n,key in enumerate(keys):
  x=30+(n%4)*308;y=151+(n//4)*132
  read_only=bool(view and n<4)
  colour='ff'+READ_ONLY_TEXT[1:] if read_only else 'ffe8e5d0'
  children=[art('parameter_box_read_only.png' if read_only else 'parameter_box.png',0,0,296,94),child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='dial_read_only.png' if read_only else 'dial.png',numFrames=127,invert=False,dragOrientation='Vertical',handleName='Data'),9,19,58,58),label(82,11,210,27,20,kind='Name',colour=colour),label(82,38,200,40,34,colour=colour)]
  if read_only:
   for component in children:component['bounds']['acceptsHWFocus']='No'
  kind=local(f'knob_v{view}_{n}',children,None if read_only else [action('Q-Link')],focus=not read_only)
  conditions=[gate('edit_view',view,2)]+([gate('rand_mode',0,2)] if not view else [])
  component=placed(kind,key,x,y,296,94,conditions)
  if read_only:component['bounds']['acceptsHWFocus']='No'
  parts.append(component)
  if not view:
   # Mutually exclusive sibling: exactly one touch owner at any given time.
   picked=[]
   for chosen in [0,1]:
    im,d=image(296,94,C['oled'])
    rounded(d,(1,1,294,92),fill='#1a3324' if chosen else C['oled'],outline='#65bc7b' if chosen else '#68736a',width=2 if chosen else 1)
    fn=f'rand_pick_box{chosen}.png';save(fn,im)
    c=art(fn,0,0,296,94);c['bounds']['additionalInvalidatingHandles']=[gate(f'rand_pick{n}',chosen,2)];picked.append(c)
   # Same indexed artwork, dimensions and angle as the normal value knob.
   # Read-only display: parent owns selection; engine rejects sel_* writes in RAND.
   for chosen in [0,1]:
    fn=f'rand_dial_strip{chosen}.png'
    if not (A/fn).exists():
     strip=Image.open(A/'dial.png').convert('RGB')
     if chosen:
      pixels=strip.load()
      for yy in range(strip.height):
       for xx in range(strip.width):
        pixels[xx,yy]=(26,51,36) if pixels[xx,yy]==tuple(int(C['oled'][i:i+2],16) for i in (1,3,5)) else (160,216,173)
     save(fn,strip)
    c=child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=fn,numFrames=127,invert=False,dragOrientation='Vertical',handleName='Value'),9,19,58,58)
    c['bounds']['acceptsHWFocus']='No';c['bounds']['additionalInvalidatingHandles']=[gate(f'rand_pick{n}',chosen,2)];picked.append(c)
   picked += [label(82,11,210,27,20,handle='Value',kind='Name'),label(82,38,200,40,34,handle='Value')]
   actions=[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')]
   k=local(f'rand_target{n}',picked,actions,focus=True)
   parts.append(placed(k,f'rand_toggle{n}',x,y,296,94,[gate('edit_view',0,2),gate('rand_mode',1,2)],maps=[{'key':'Value','value':f'Parameter {I[key]}'}]))
  if view and n>=4:
   im=Image.new('RGBA',(10,10),(0,0,0,0));d=ImageDraw.Draw(im);d.ellipse((2,2,7,7),fill=C['text']);fn='custom_dot.png';save(fn,im);c=art(fn,x+277,y+10,10,10);c['bounds']['additionalInvalidatingHandles']=[gate('edit_view',1,2),gate(f'step_custom{n-4}',1,2)];parts.append(c)
# Each pad has ONE native touch owner containing its passive state artwork.
# A sibling display above a separate Button can intercept physical touch on MPC.
for i in range(32):
 x=20+(i%16)*78;y=470+(i//16)*64;w=70;h=54
 states=[]
 for state in range(18):
  outside=state>=16;active=not outside and state&1;selected=(not outside and state&2) or state==17;head=not outside and state&4;custom=not outside and state&8
  bg='#432449' if custom else '#352329' if active else C['panel'];ink='#d780e0' if custom else C['red'] if active else '#555e62' if outside else '#a5adaa'
  im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=bg,outline='#ffffff' if selected else '#88518e' if custom else '#485057',width=2 if selected else 1)
  if head:rounded(d,(4,4,w-5,h-5),outline=C['red'],width=3,radius=7)
  if custom:d.ellipse((w-17,10,w-13,14),fill=C['text'])
  text(d,(w//2,h//2),f'{i+1:02}',24,ink,'mm');fn=f'step{i}_state{state}.png';save(fn,im)
  c=art(fn,0,0,w,h);c['bounds']['additionalInvalidatingHandles']=[gate(f'editcell{i}',state,18)];
  states.append(c)
 # Native action, as for the working M/S switches: no overlaid sibling and
 # no nested Button that could consume the touch or fire the action twice.
 actions=[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')]
 k=local(f'pad_touch{i}',states,actions)
 c=placed(k,f'step_press{i}',x,y,w,h);parts.append(c)

# TRACKS: one passive 128-frame tile per step, shared across all 512 cells.
# Frames retain exactly the previous static tile pixels and MIDI state indices.
tiny_tiles=[]
tiny_atlas=Image.new('RGB',(12,22*128),C['bg'])
for state in range(9):
 # Miniature PATTERN tiles: preserve all nine existing state values and geometry.
 # Playback outlines the tile without hiding its note/custom colour.
 outside=state==8
 active=not outside and bool(state&1)
 custom=not outside and bool(state&4)
 head=not outside and bool(state&2)
 bg='#1c2025' if outside else '#432449' if custom else '#352329' if active else C['panel']
 edge='#292e34' if outside else '#88518e' if custom else '#a9434c' if active else '#485057'
 im,dr=image(12,22)
 rounded(dr,(1,3,10,18),fill=bg,outline=edge,radius=2)
 if head:rounded(dr,(0,2,11,19),outline=C['text'],radius=3)
 tiny_tiles.append(im)
# Native numFrames is the frame COUNT here, not the last frame index.
# Plateaus also tolerate floor/nearest rounding of normalized MIDI states.
for frame in range(128):tiny_atlas.paste(tiny_tiles[round(frame*8/127)],(0,frame*22))
save('track_states_128.png',tiny_atlas)
tiny=child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='track_states_128.png',numFrames=128,invert=False,dragOrientation='Horizontal',handleName='Data'),0,0,12,22)
tiny['bounds']['acceptsHWFocus']='No'
tiny_kind=local('track_preview_tile',[tiny])
def track_static_field(key,x,y,w,h,n=26):
 # Q-Link owns editing; the identical 128 background frames carried no value.
 fn=f'track_field_{w}_{h}.png'
 if not (A/fn).exists():
  im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=C['panel'],outline='#727d81');save(fn,im)
 kind=local(f'track_field_{key}',[art(fn,0,0,w,h),label(12,0,w-24,h,n)],[action('Q-Link')],focus=True)
 return placed(kind,key,x,y,w,h)
def track_mode_picker():
 fn='track_mode_pick.png';im,d=image(360,48);rounded(d,(1,1,358,46),fill=C['panel'],outline='#727d81');save(fn,im)
 kind=local('track_mode_picker',[art(fn,0,0,360,48),label(12,0,336,48,26,handle='Caption')],[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True)
 return placed(kind,'open_bank',880,116,360,48,maps=[{'key':'Caption','value':f"Parameter {I['play_mode']}"}])
track_pages=[]
for start in [0,8]:
 im,dr=image(1280,628);text(dr,(24,16),f'TRACKS / {start+1:02}–{start+8:02}',28)
 for title,x,y in [('MODE',880,86),('SHIFT / STEPS',880,260),('EVERY / CYCLES',1070,260),('SHIFT',880,340),('CYCLE',1070,340)]:text(dr,(x,y),title,19,C['muted'])
 fn=f'tracks_{start}.png';save(fn,im);tracks=[art(fn,0,0,1280,628)]
 for row in range(8):
  i=start+row;y=88+row*58
  selection=button(f'edit_line{i}',f'A{i+1:02}',20,y,70,42,native_touch=True)
  # Keep the existing selection trigger; amber follows the actual selected lane.
  for state,c in enumerate(defs[-1]['value']['componentsData'][:2]):
   c['bounds']['additionalInvalidatingHandles']=[gate(f'line_on{i}',state,2)]
  tracks.append(selection)
  for j in range(32):
   c=placed(tiny_kind,f'trackcell{i}_{j}',154+j*15,y+10,12,22);c['bounds']['acceptsHWFocus']='No';tracks.append(c)
  tracks += toggle(f'l{i}_mute','M',697,y,62,42)+toggle(f'l{i}_solo','S',775,y,62,42)
 tracks += [track_mode_picker(),*toggle('auto_rotate','AUTO ROTATE',880,190,360,46),track_static_field('auto_shift',880,292,168,48),track_static_field('auto_every',1070,292,170,48),readout('phase_offset',880,362,168,38,23),readout('phase_cycle',1070,362,170,38,23),button('reset_phase','RESET PHASE',880,430,360,48)]
 for key,x,title in [('phase_offset',880,'SHIFT'),('phase_cycle',1070,'CYCLE')]:
  item=next(c for c in tracks if c['componentData']['name']==key)
  width=math.ceil(font(19).getlength(title))
  item['bounds']['bounds']=f'{x} 362 {width} 38'
  definition=next(d['value'] for d in defs if d['key']==item['componentData']['type'])
  lab=definition['componentsData'][0]
  lab['bounds']['bounds']=f'0 0 {width} 38'
  lab['componentData']['data']['textStyle']['justification']='horizontallyCentred verticallyCentred'
 track_pages.append(tracks)
# CONFIG: sixteen fixed pad labels, one unified note field per lane.
im,dr=image(1280,628);save('config.png',im)
config=[art('config.png',0,0,1280,628)]
# Identical native font and label height to PATTERN parameter names.
# Reuse existing read-only label handles; preserve all parameter indices.
for i in range(16):
 y=32+i*36
 im,d=image(100,32);text(d,(0,16),f'A{i+1:02}',22,anchor='lm');fn=f'config_pad{i}.png';save(fn,im);config.append(art(fn,20,y,100,32))
 config+=toggle(f'l{i}_hold','',145,y,290,32)
 config.append(field(f'l{i}_dnote',510,y,208,32,29))
# CONFIG placement follows the user reference download.png (28 October 2026).
# Coordinates exclude MPC's 111px native header.
for title,x,y in [('SCALES',875,86),('START',875,186)]:
 im,d=image(360,28);text(d,(0,0),title,19,C['muted']);fn='config_'+title.lower()+'.png';save(fn,im);config.append(art(fn,x,y,360,28))
# All pending colours follow a read-only 2-bit handle; no animation or polling.
for pending in range(4):
 for key,x,y,w,h in [('choose_bank',875,116,360,46),('edit_row',875,216,174,46),('apply_mapping',1061,216,174,46)]:
  green=bool(pending & (1 if key=='choose_bank' else 2)) if key!='apply_mapping' else bool(pending)
  fill='#1a3324' if green else C['panel'];edge='#65bc7b' if green else '#727d81';ink='#a0d8ad' if green else C['text']
  tag=f'config_pending_{key}_{pending}'
  if key=='edit_row':
   c=field(key,x,y,w,h);definition=copy.deepcopy(defs[-1]['value']);fn=tag+'.png';im=Image.new('RGB',(w,h*128),C['bg']);d=ImageDraw.Draw(im)
   for frame in range(128):rounded(d,(1,frame*h+1,w-2,(frame+1)*h-2),fill=fill,outline=edge)
   save(fn,im);definition['componentsData'][0]['componentData']['data']['filmStrip']=fn
   definition['componentsData'][1]['componentData']['data']['textStyle']['colour']='ff'+ink[1:]
   defs.append({'key':tag,'value':definition});c['componentData']['type']=tag
  else:
   im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=fill,outline=edge)
   if key=='apply_mapping':
    text(d,(w//2,h//2),'APPLY',21,ink,'mm')
    if green:d.ellipse((w-17,9,w-13,13),fill='#ffffff')
   fn=tag+'.png';save(fn,im);children=[art(fn,0,0,w,h)];maps=[]
   if key=='choose_bank':
    children.append(label(12,0,w-24,h,26,handle='Caption',colour='ff'+ink[1:]));maps=[{'key':'Caption','value':f"Parameter {I['choose_sound']}"}]
   kind=local(tag,children,[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True);c=placed(kind,key,x,y,w,h,maps=maps)
  c['bounds']['additionalInvalidatingHandles']=[gate('choose_target',pending,4)];config.append(c)
config.append(button('open_mapping','RESET',875,302,360,46,native_touch=True))
# Only MOD still owns a selection layer.
def overlay(dialog,title):
 im,d=image(1280,628);text(d,(40,25),title,32);filename=f'dialog{dialog}.png';save(filename,im);return [art(filename,0,0,1280,628)]
# E01 MOD panel. All fields retain one native touch/focus owner.
# Activity indicators are passive and cannot intercept a lane press.
mod_parts=[art('pattern.png',0,0,1280,628)]
for container in [parts]:
 for i in range(16):
  for muted,flash in [(0,0),(0,1),(1,0)]:
   for selected in [0,1]:
    ink=('#ad747b' if selected else '#858d91') if muted else '#91c9df' if flash else C['red'] if selected else C['text']
    im=Image.new('RGBA',(8,8),(0,0,0,0));ImageDraw.Draw(im).ellipse((2,2,5,5),fill=ink);fn=f'mod_dot_{muted}_{flash}_{selected}.png';save(fn,im)
    c=art(fn,20+(i%8)*156+132,12+(i//8)*52+8,8,8)
    conditions=[gate(f'mod_indicator{i}',1,2),gate(f'line_on{i}',selected,2),gate(f'l{i}_mute',muted,2)]
    if not muted:conditions.append(gate(f'lane_flash{i}',flash,2))
    c['bounds']['additionalInvalidatingHandles']=conditions;container.append(c)
# Four slot selectors; one native touch owner per button. Selection artwork
# follows mod_slot, not the transient trigger, so repeated taps remain reliable.
for i in range(4):
 x=20+i*244;children=[]
 for selected in [0,1]:
  im,d=image(232,48);rounded(d,(1,1,230,46),fill='#352329' if selected else C['panel'],outline='#ffffff' if selected else '#727d81',width=2 if selected else 1);text(d,(116,24),f'MOD {i+1}',24,C['red'] if selected else C['text'],'mm');fn=f'mod_slot_{i}_{selected}.png';save(fn,im);c=art(fn,0,0,232,48);c['bounds']['additionalInvalidatingHandles']=[gate('mod_slot',i,4)] if selected else [];children.append(c)
 # Use one of four visible-state owners, with explicit exclusion gates.
 children[0]['bounds']['additionalInvalidatingHandles']=[]
 children[0]['bounds']['whenVisible']='Always'
 # Draw normal artwork then selected artwork on top; both passive.
 kind=local(f'mod_choose_native{i}',children,[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True)
 mod_parts.append(placed(kind,f'mod_choose{i}',x,20,232,48))
# The pad assignment is a red selected target, not a generic dropdown.
children=[]
for assigned in range(17):
 im,d=image(260,48);rounded(d,(1,1,258,46),fill='#352329' if assigned else C['panel'],outline='#ffffff' if assigned else '#727d81');text(d,(130,24),f'A{assigned:02}' if assigned else 'NONE',24,C['red'] if assigned else C['text'],'mm');fn=f'mod_pad_{assigned}.png';save(fn,im);c=art(fn,0,0,260,48);c['bounds']['additionalInvalidatingHandles']=[gate('mod_pad',assigned,17)];children.append(c)
kind=local('mod_pad_open',children,[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True);mod_parts.append(placed(kind,'open_rhythm',1000,20,260,48))
im,dr=image(1280,550);dr.line((640,8,640,152),fill='#68736a');dr.line((640,332,640,470),fill='#68736a');save('mod_panel.png',im);mod_parts.append(art('mod_panel.png',0,78,1280,550))
def mod_slider(key,x,y,w=180,h=58):
 fn=f'mod_slider_{w}_{h}.png'
 if not (A/fn).exists():
  im=Image.new('RGB',(w,h*128),C['bg']);d=ImageDraw.Draw(im)
  for v in range(128):
   yy=v*h;d.rounded_rectangle((4,yy+11,w-5,yy+17),radius=3,fill='#464b4d');xx=7+int(v*(w-15)/127);d.line((7,yy+14,xx,yy+14),fill=C['amber_text'],width=5);d.ellipse((xx-5,yy+8,xx+5,yy+20),fill=C['amber_text'])
  save(fn,im)
 children=[child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=fn,numFrames=127,invert=False,dragOrientation='Horizontal',handleName='Data'),0,0,w,h),label(8,28,w-16,h-28,18 if h<50 else 21)]
 name=f'slider_{key}_{w}_{h}'
 if not any(row['key']==name for row in defs):local(name,children,[action('Q-Link')],focus=True)
 return placed(name,key,x,y,w,h)
def mod_picker(key,pick,x,y,w,h,n=23):
 fn=f'mod_pick_{pick}.png';im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill=C['panel'],outline='#727d81');save(fn,im)
 children=[art(fn,0,0,w,h)]
 if pick%4==3:
  active_fn=f'mod_pick_chain_{pick}.png';im,d=image(w,h);rounded(d,(1,1,w-2,h-2),fill='#302733',outline='#8d649a');save(active_fn,im)
  for dest in [8,9,10]:
   c=art(active_fn,0,0,w,h);c['bounds']['additionalInvalidatingHandles']=[gate(key,dest,11)];children.append(c)
 children.append(label(14,0,w-28,h,n,handle='Caption'))
 kind=local(f'mod_pick_touch_{pick}',children,[action('Toggle Switch'),dict(action('Toggle Switch'),onAction='Enter Pressed')],focus=True)
 return placed(kind,f'open_sound{pick}',x,y,w,h,maps=[{'key':'Caption','value':f'Parameter {I[key]}'}])
for j,x in [(0,20),(1,700)]:
 im,d=image(560,46);text(d,(8,23),f'LFO {j+1}',24,anchor='lm');save(f'mod_header{j}.png',im);mod_parts.append(art(f'mod_header{j}.png',x,75,560,46));mod_parts+=toggle(f'mod{j}_on','ON',x+90,75,96,46)
 mod_parts.append(mod_picker(f'mod{j}_wave',j*4,x+200,75,360,46))
 im,d=image(560,96,C['oled']);
 for xx in [140,280,420]:d.line((xx,22,xx,95),fill='#2c4036')
 save(f'mod_plot{j}.png',im);mod_parts.append(art(f'mod_plot{j}.png',x,190,560,96));mod_parts.append(readout('mod_tempo',x+325,190,235,22,16))
 # Encoded endpoint pairs: a static segment can connect different levels,
 # unlike the old disconnected horizontal point markers. Shared filmstrips.
 fn='mod_segment_atlas.png'
 if not (A/fn).exists():
  im=Image.new('RGB',(4,68*3072),C['oled']);d=ImageDraw.Draw(im)
  for style in range(3):
   for pair in range(1024):
    a,b=divmod(pair,32);yy=(style*1024+pair)*68;ya=yy+34-round((a/31*2-1)*29);yb=yy+34-round((b/31*2-1)*29);d.line((0,yy+34,3,yy+34),fill='#2c4036')
    if style==2:d.rectangle((0,yb,3,ya) if yb<ya else (0,ya,3,yb),fill='#507981')
    elif style==1:d.line((0,ya,3,ya,3,yb),fill='#91c9df',width=2)
    else:d.line((0,ya,3,yb),fill='#91c9df',width=2)
  save(fn,im)
 for k in range(128):
  c=child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=fn,numFrames=3071,invert=False,dragOrientation='Horizontal',handleName='Data'),0,0,4,68);c['bounds']['acceptsHWFocus']='No'
  kind=local(f'mod_static_read{j}_{k}',[c]);c=placed(kind,f'g{j*4+k//32}_{k%32}',x+24+k*4,212,4,68);c['bounds']['acceptsHWFocus']='No';mod_parts.append(c)
 # Bar guides sit above opaque segment tiles and do not receive touches.
 if not (A/'mod_bar_guide.png').exists():
  im=Image.new('RGBA',(1,68),(0,0,0,0));d=ImageDraw.Draw(im)
  for gy in range(0,68,8):d.line((0,gy,0,min(gy+3,67)),fill='#384b42')
  save('mod_bar_guide.png',im)
 for gx in [128,256,384]:mod_parts.append(art('mod_bar_guide.png',x+24+gx,212,1,68))
 # Threshold is one passive dotted line, split into reusable narrow tiles.
 if not (A/'mod_threshold_line.png').exists():
  im=Image.new('RGBA',(17,68*201),(0,0,0,0));d=ImageDraw.Draw(im)
  for v in range(201):
   yy=v*68+34-round(((v-100)/100)*29);d.line((0,yy,6,yy),fill=C['amber_text'],width=2);d.line((11,yy,16,yy),fill=C['amber_text'],width=2)
  save('mod_threshold_line.png',im)
 children=[child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='mod_threshold_line.png',numFrames=200,invert=False,dragOrientation='Horizontal',handleName='Data'),k*17,0,17,68) for k in range(31)]
 for c in children:c['bounds']['acceptsHWFocus']='No'
 kind=local(f'threshold_overlay{j}',children);c=placed(kind,f'mod{j}_threshold',x+16,212,527,68,[gate(f'mod{j}_dest',7,11)]);c['bounds']['acceptsHWFocus']='No';mod_parts.append(c)
 mod_parts.append(mod_picker(f'mod{j}_sync',j*4+1,x,342,180,44));rate=mod_picker(f'mod{j}_rate',j*4+2,x+196,342,364,44);rate['bounds']['additionalInvalidatingHandles']=[gate(f'mod{j}_sync',1,2)];mod_parts.append(rate)
 hz=mod_slider(f'mod{j}_hz',x+196,342,364,44);hz['bounds']['additionalInvalidatingHandles']=[gate(f'mod{j}_sync',0,2)];mod_parts.append(hz)
 for k,(key,title) in enumerate([('depth','DEPTH'),('offset','OFFSET'),('phase','PHASE')]):
  xx=x+k*190;im,d=image(180,22);text(d,(0,0),title,18,C['muted']);fn=f'mod_label_{j}_{key}.png';save(fn,im);mod_parts += [art(fn,xx,401,180,22),mod_slider(f'mod{j}_{key}',xx,426)]
 im,d=image(560,22);text(d,(0,0),'DESTINATION',18,C['muted']);save(f'mod_destination{j}.png',im);mod_parts += [art(f'mod_destination{j}.png',x,490,560,22),mod_picker(f'mod{j}_dest',j*4+3,x,516,560,44)]
 threshold=mod_slider(f'mod{j}_threshold',x+196,572,180,50);threshold['bounds']['additionalInvalidatingHandles']=[gate(f'mod{j}_dest',7,11)];mod_parts.append(threshold)
 im,d=image(180,30);text(d,(0,4),'THRESHOLD',18,C['muted']);save(f'mod_threshold{j}.png',im);c=art(f'mod_threshold{j}.png',x,576,180,30);c['bounds']['additionalInvalidatingHandles']=[gate(f'mod{j}_dest',7,11)];mod_parts.append(c)
 # Eight vertical faders replace the unintelligible numeric point buttons.
 fn='mod_curve_fader.png'
 if not (A/fn).exists():
  im=Image.new('RGB',(66,40*128),C['oled']);d=ImageDraw.Draw(im)
  for v in range(128):
   yy=v*40;d.line((18,yy+3,18,yy+35),fill='#5d6461',width=3);y=yy+35-round(v*32/127);d.rounded_rectangle((9,y-2,27,y+2),radius=2,fill=C['amber_text'])
  save(fn,im)
 for k in range(8):
  children=[child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=fn,numFrames=127,invert=False,dragOrientation='Vertical',handleName='Data'),0,0,66,40),label(30,7,35,25,17)]
  kind=local(f'free_point_slider{j}_{k}',children,[action('Q-Link')],focus=True);c=placed(kind,f'mod{j}_c{k}',x+k*70,282,66,40,[gate(f'mod{j}_wave',7,11)]);mod_parts.append(c)
# Small MIX dial, passive value only; no second slider or animated meter.
# Shape-specific controls reuse contextual curve handles; FREE CURVE points
# remain separately stored. No new parameter indices or animated readouts.
for j,x in [(0,20),(1,700)]:
 for wave,names in [(4,['WIDTH']),(8,['STEP SIZE']),(9,['LENGTH','EVOLVE']),(10,['GRAVITY','BOUNCE'])]:
  for k,title in enumerate(names):
   xx=x+k*280;im,d=image(260,22);text(d,(0,0),title,16,C['muted']);fn=f'mod_extra_{j}_{wave}_{k}.png';save(fn,im)
   for c in [art(fn,xx,282,260,22),mod_slider(f'mod{j}_c{k}',xx,306,260,44)]:
    c['bounds']['additionalInvalidatingHandles']=[gate(f'mod{j}_wave',wave,11)];mod_parts.append(c)
# Move rate fields below contextual controls, retaining the static graph and
# THRESHOLD. Standard forms use the same layout to avoid moving touch targets.
for c in mod_parts:
 rect=list(map(int,c['bounds']['bounds'].split()))
 if rect[1]==342:rect[1]=352;c['bounds']['bounds']=' '.join(map(str,rect))
# Passive small round marker on a parameter receiving the other LFO.
for j,x in [(0,20),(1,700)]:
 for dest,xx,yy in [(8,x+540,358),(9,x+166,405),(10,x+356,405)]:
  im=Image.new('RGBA',(8,8),(0,0,0,0));ImageDraw.Draw(im).ellipse((2,2,5,5),fill=C['text']);fn=f'mod_chain_dot_{j}_{dest}.png';save(fn,im)
  c=art(fn,xx,yy,8,8);c['bounds']['additionalInvalidatingHandles']=[gate(f'mod{1-j}_on',1,2),gate(f'mod{1-j}_dest',dest,11)];mod_parts.append(c)
im,d=image(72,28);text(d,(36,14),'MIX',20,anchor='mm');save('mod_mix_title.png',im);mod_parts.append(art('mod_mix_title.png',604,298,72,28))
kind=local('mod_mix_dial',[child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='dial.png',numFrames=127,invert=False,dragOrientation='Vertical',handleName='Data'),0,0,58,58)],[action('Q-Link')],focus=True)
mod_parts += [placed(kind,'mod_mix',611,330,58,58),readout('mod_mix',619,396,48,28,22)]
# One header row saves 50 native pixels on both sides. Move every lower
# control together, preserving spacing, hit regions, centre line and MIX axis.
for c in mod_parts:
 rect=list(map(int,c['bounds']['bounds'].split()))
 if rect[1]>=130:rect[1]-=50;c['bounds']['bounds']=' '.join(map(str,rect))
for c in mod_parts:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',0,7))
cs=overlay(6,'SELECT PAD')
for i in range(16):
 cs.append(button('mod_pad',f'A{i+1:02}',280+(i%8)*90,220+(i//8)*75,78,56,False,i+1,17,tag=f'mod_target_pad{i}'))
cs += [button('apply_rhythm','NONE',380,420,220,52,native_touch=True),button('close','CANCEL',680,420,220,52,native_touch=True)]
for c in cs:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',6,7));mod_parts.append(c)
# Same opaque selection-layer mechanism as MOD pad assignment.
for c in config:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',0,7))
scale_layer=overlay(5,'SELECT SCALE')
scale_names=next(p['options'] for p in P if p['key']=='choose_sound')
for i,title in enumerate(scale_names):
 scale_layer.append(button('choose_sound',title,80+(i%4)*282,130+(i//4)*80,268,62,False,i,16,tag=f'scale_choice_{i}'))
scale_layer.append(button('close','CLOSE',530,500,220,52,native_touch=True))
for c in scale_layer:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',5,7));config.append(c)
# Explicit confirmation for a bounded global randomization.
for c in parts:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',0,7))
confirmation=overlay(4,'This will randomize all the patterns')
confirmation += [button('apply_rhythm','APPLY',370,320,240,56,native_touch=True),button('close','CANCEL',670,320,240,56,native_touch=True)]
for c in confirmation:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',4,7));parts.append(c)
# Eight contextual panels share one read-only context handle and one selection.
mod_lists=[[next(p['options'] for p in P if p['key']==f'mod{j}_'+name) for name in ['wave','sync','rate','dest']] for j in range(2)]
for pick in range(8):
 title=['WAVEFORM','CLOCK MODE','RATE','DESTINATION'][pick%4]
 panel=overlay(20+pick,title)
 options=mod_lists[pick//4][pick%4]
 for option,name in enumerate(options):panel.append(button('map_sound',name,80+(option%4)*282,190+(option//4)*90,268,64,False,option,11,tag=f'mod_choice_{pick}_{option}'))
 panel.append(button('close','CANCEL',530,510,220,52,native_touch=True))
 for c in panel:c['bounds']['additionalInvalidatingHandles'] += [gate('dialog',2,7),gate('can_undo',pick,8)];mod_parts.append(c)
mode_panel=overlay(3,'PLAYBACK MODE')
for option,name in enumerate(['FWD','REV','PEND1','PEND2','URN']):mode_panel.append(button('map_sound',name,110+option*214,240,196,64,False,option,11,tag=f'play_mode_choice{option}'))
mode_panel.append(button('close','CANCEL',530,420,220,52,native_touch=True))
for page in track_pages:
 for c in page:c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',0,7))
 for original in mode_panel:
  c=copy.deepcopy(original);c['bounds']['additionalInvalidatingHandles'].append(gate('dialog',3,7));page.append(c)
pages=[('PATTERN' ,parts),('MOD',mod_parts),('01 - 08',track_pages[0]),('09 - 16',track_pages[1]),('CONFIG',config)]
for name,children in pages:
 for control in children:
  x,y,w,h=map(int,control['bounds']['bounds'].split())
  if x<0 or y<0 or w<=0 or h<=0 or x+w>1280 or y+h>628:raise ValueError('Control outside native content: '+control['componentData']['name'])
 local(name,children)
D['tabs']=[dict(version=3,tabName=name,fnKeyIndex=i,fnKeySubIndex=0,qlinkBoundsData=['0 0 0 0'],componentName=name,initialSize='0 0 1280 628',scale=1.) for i,(name,_) in enumerate(pages)]
# Identical images share one filename and therefore one host image-cache key.
image_by_hash={};aliases={};saved_bytes=0
for path in sorted(A.glob('*.png')):
 data=path.read_bytes();digest=hashlib.sha256(data).hexdigest()
 if digest in image_by_hash:
  aliases[path.name]=image_by_hash[digest];saved_bytes+=len(data)
 else:image_by_hash[digest]=path.name
def share_images(value):
 if isinstance(value,dict):
  for key,item in value.items():
   if key in ('image','onImage','offImage','filmStrip') and isinstance(item,str) and item in aliases:value[key]=aliases[item]
   else:share_images(item)
 elif isinstance(value,list):
  for item in value:share_images(item)
share_images(J)
for alias in aliases:(A/alias).unlink()
print('Image cache deduplication:',len(aliases),'duplicate files;',saved_bytes,'compressed bytes removed')
(A/'TUI.json').write_text(json.dumps(J,indent=2)+'\n')
for filename in ['Q-Links.json','Q-Links - 8by1.json']:
 q=json.loads(z.read('Plugin Skins/'+filename));template=q['Screen Mode Q-Links']['map'][0];rows=[]
 for tab in range(1,6):
  row=copy.deepcopy(template);row['Tab']=tab;row['SubTab']=1;row['Q-Links']={f'Q-Link {i}':-1 for i in range(1,17)}
  keys=[f'q{i}' for i in range(8)] if tab==1 else ['mod0_depth','mod0_offset','mod0_phase','mod0_threshold','mod1_depth','mod1_offset','mod1_phase','mod1_threshold'] if tab==2 else ['line_select16','play_mode','auto_rotate','auto_shift','auto_every'] if tab in (3,4) else []
  for hw,k in zip([13,9,5,1,14,10,6,2],keys):row['Q-Links'][f'Q-Link {hw}']=I[k]
  rows.append(row)
 q['Screen Mode Q-Links']['map']=rows;(A/filename).write_text(json.dumps(q,indent=2)+'\n')
(OUT/'version.xml').write_text(f'<?xml version="1.0"?>\n<plugincontent version="1.0"><identifier>{SKIN_IDENTIFIER}</identifier><version>{VERSION.split("-",1)[0]}.0</version></plugincontent>\n',encoding='utf-8')
print('Native skin:',len(defs),'definitions;',len(list(A.glob('*.png'))),'PNG assets; five MPC pages')

receipt={"version":VERSION,"inputs":inputs(),"outputs":tree(OUT),"timestamp_utc":timestamp()}
(B/"build/skin-receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")

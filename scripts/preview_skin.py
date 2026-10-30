"""Offline layout view of generated native controls. Not an MPC screenshot or engine test."""
from pathlib import Path
import json, re, math
from project_config import FOLDER, DISPLAY_NAME
from PIL import Image, ImageDraw, ImageFont
B=Path(__file__).resolve().parents[1]
A=B/'build/skin'/FOLDER/'Plugin Skins'
J=json.loads((A/'TUI.json').read_text())['pageData']
D={row['key']:row['value'] for row in J['componentDefinitions']['localComponentDefinitions']}
P=json.loads((B/'params.json').read_text()); V=[p['default'] for p in P]; K={p['key']:i for i,p in enumerate(P)}
def pattern(n,k,r,j):
    t=(j-r+n)%n
    return int(k>0 and (t*k)%n<k)
for i in range(16):
    n=V[K[f'l{i}_steps']]; k=V[K[f'l{i}_hits']]; r=V[K[f'l{i}_rotate']]
    for j in range(32):
        if i<8:V[K[f'g{i}_{j}']]=pattern(n,k,r,j) if j<n else 4
        V[K[f'trackcell{i}_{j}']]=pattern(n,k,r,j) if j<n else 8
    if i==0:
        for j in range(32):
            V[K[f'cell{j}']]=(pattern(n,k,r,j) if j<n else 8)+(2 if j==0 else 0)
            V[K[f'editcell{j}']]=(pattern(n,k,r,j) if j<n else 16)+(2 if j==0 else 0)
for k in ['steps','hits','rotate','division','velocity','humanize','prob','gate']: V[K['sel_'+k]]=V[K['l0_'+k]]
V[K['step_velocity']]=V[K['l0_velocity']]
V[K['step_prob']]=V[K['l0_prob']]
texts={'line_label':'01 - A01','target_label':'DRUM TRACK / CH 1 / NOTE 36','status':'STOP / MIDI TO QUALIFY','step_label':'STEP 01 / HIT','mapping_label':'INCIPIENCE NOTES','port_label':'piClideaN MPC Out','step_scope':'SELECTED STEP / A01 /','line_scope':'LINE DEFAULTS / A01','phase_label':'CYCLE 1 / SHIFT 0','play_mode':'FWD','edit_tool':'SELECT','rhythm_choice':'FOUR ON THE FLOOR'}
for i in range(16):texts[f'name{i}']=f'A{i+1:02}'
def value(handle):
    m=re.fullmatch(r'Parameter (\d+)',handle)
    return int(m[1]) if m else None
def draw_control(canvas,c,ox=0,oy=0,handle=None):
    handles=dict(handle or {})
    b=c['bounds']; x,y,w,h=map(int,b['bounds'].split()); x+=ox; y+=oy
    if b.get('whenVisible')=='WhenFocussed':return
    for g in b.get('additionalInvalidatingHandles',[]):
        m=re.fullmatch(r'IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)',g)
        if m and V[int(m[3])]!=int(m[1]):return
    for row in c.get('handle remapping',{}).get('map',[]):
        handles[row['key']]=row['value']
    spec=c['componentData']; kind=spec['type']; d=spec['data']; idx=value(handles.get(d.get('handleName','Data'),''))
    if kind in D:
        for sub in D[kind]['componentsData']:draw_control(canvas,sub,x,y,handles)
    elif kind=='Image':
        canvas.paste(Image.open(A/d['image']).convert('RGB'),(x,y))
    elif kind=='Button':
        on=idx is not None and V[idx]==d['buttonId']
        canvas.paste(Image.open(A/d['onImage' if on else 'offImage']).convert('RGB'),(x,y))
    elif kind=='Knob':
        p=P[idx] if idx is not None else {'min':0,'max':1};lo=p.get('min',0);hi=len(p['options'])-1 if 'options'in p else p.get('max',1)
        last=d['numFrames']-1 if d['filmStrip']=='track_states_128.png' else d['numFrames']
        frame=round(last*((V[idx] if idx is not None else 0)-lo)/(hi-lo)) if hi!=lo else 0
        strip=Image.open(A/d['filmStrip']).convert('RGB');canvas.paste(strip.crop((0,frame*h,w,(frame+1)*h)),(x,y))
    elif kind=='Label' and idx is not None:
        p=P[idx];v=V[idx];style=d['textStyle'];fs=round(style['font']['height']);font=ImageFont.truetype(str(B/'reference/fonts/TitilliumWeb-Regular.ttf'),fs)
        if d['type']=='Name':t=p['name']
        elif p['key'] in texts:t=texts[p['key']]
        elif 'options'in p:t=p['options'][int(v)]
        else:t=str(v)
        colour='#'+style['colour'][-6:];layer=Image.new('RGBA',(w,h));dr=ImageDraw.Draw(layer);dr.text((0,h/2),t,font=font,fill=colour,anchor='lm');canvas.paste(layer,(x,y),layer)
pages=[dict(p) for p in J['tabs']];pages.append(dict(pages[0],preview_name='PATTERN-GENERATE'));pages.append(dict(pages[0],preview_name='PATTERN-ACTIVITY'))
(B/'docs/images').mkdir(parents=True,exist_ok=True)
for page in pages:
    for i in range(16):V[K[f'lane_flash{i}']]=int(page.get('preview_name')=='PATTERN-ACTIVITY' and i in (0,2,4))
    V[K['edit_view']]=0 if page.get('preview_name')=='PATTERN-GENERATE' else 1
    name=page['tabName'];native=Image.new('RGB',(1280,628),'#171a1e')
    for c in D[name]['componentsData']:draw_control(native,c)
    out=Image.new('RGB',(800,480),'#15171a');out.paste(native.resize((800,393),Image.Resampling.LANCZOS),(0,47));dr=ImageDraw.Draw(out);ft=ImageFont.truetype(str(B/'reference/fonts/TitilliumWeb-Regular.ttf'),17)
    dr.text((14,12),DISPLAY_NAME+' / OFFLINE SKIN LAYOUT',font=ft,fill='#e8e5d0')
    for i,n in enumerate([t['tabName'] for t in J['tabs']]):dr.text((i*160+12,451),n,font=ft,fill='#fff' if n==name else '#92999e')
    dr.line((page['fnKeyIndex']*160,440,page['fnKeyIndex']*160+159,440),fill='#f2474d',width=2)
    filename={'PATTERN':'pattern-edit.png','PATTERN-GENERATE':'pattern-line.png','PATTERN-ACTIVITY':'activity.png','TR 01-08':'tracks.png','TR 09-16':'tracks-09-16.png','SET 01-08':'setup.png','SET 09-16':'setup-09-16.png'}[page.get('preview_name',name)]
    out.save(B/'docs/images'/filename)
print('Five offline native-layout previews generated')

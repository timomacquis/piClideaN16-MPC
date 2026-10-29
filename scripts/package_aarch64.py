"""Assemble the portable aarch64 (Gen2) installation archive from the same verified build outputs as the engine."""
from pathlib import Path
import hashlib,json,tempfile,os,zipfile,xml.etree.ElementTree as ET
from project_config import VERSION,DISPLAY_NAME,FOLDER
from provenance import inputs,tree
B=Path(__file__).resolve().parents[1]
def digest(data):return hashlib.sha256(data).hexdigest()
def main():
    build=json.loads((B/'build/reports/BUILD-aarch64.json').read_text())
    if build['version']!=VERSION or build['input_hashes']!=inputs():
        raise SystemExit('Build inputs differ; run scripts/build_aarch64.py first')
    skin=B/'build/skin'/FOLDER
    if build['skin_output_hashes']!=tree(skin):raise SystemExit('Skin outputs differ; rebuild first')
    engine=(B/'build/aarch64/piclidean_mpc.so').read_bytes()
    if engine[:5]!=b'\x7fELF\x02' or int.from_bytes(engine[18:20],'little')!=183 or digest(engine)!=build['engine_sha256']:
        raise SystemExit('Invalid aarch64 engine or build checksum')
    name=f'piClideaN-MPC-{VERSION}-aarch64.zip'
    guide=(B/'docs/INSTALLATION-GEN2.md').read_text(encoding='utf-8')
    if name not in guide or 'armv7.zip' in guide.replace('piClideaN-MPC-'+VERSION+'-armv7.zip','') or 'aarch64' not in guide:
        raise SystemExit('Gen2 installation guide must name '+name+' and no other package')
    manifest=dict(schema=1,id='piclidean-mpc',name=DISPLAY_NAME,version=VERSION,distribution_revision='release',uid='5069434e',manufacturer='Timo',so='piclidean_mpc.so',skin=FOLDER,folder='portable/'+FOLDER,arch='aarch64',kind='instrument',layout='portable',extras=[],user_data=[],requires='')
    metadata=ET.Element('PLUGIN',name=DISPLAY_NAME,descriptiveName=DISPLAY_NAME,format='VST',category='Synth',manufacturer='Timo',version=VERSION,file='%payload-path%/'+FOLDER+'/piclidean_mpc.so',uid='5069434e',isInstrument='1',fileTime='0',infoUpdateTime='0',numInputs='0',numOutputs='2',isShell='0')
    port='portable/'+FOLDER+'/'
    payload={}
    for p in skin.rglob('*'):
        if p.is_symlink():raise RuntimeError('Symbolic link in generated skin')
        if p.is_file():payload[port+p.relative_to(skin).as_posix()]=p.read_bytes()
    payload[port+'piclidean_mpc.so']=engine
    mj=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode()
    payload['mpc-plugin.json']=mj;payload[port+'mpc-plugin.json']=mj
    payload[port+'plugin-meta.xml']=(ET.tostring(metadata,encoding='unicode')+'\n').encode()
    payload['MODES']=b'x\tpiclidean_mpc.so\n'
    for n in ['README.md','LICENSE','THIRD_PARTY.md','Cover.JPG']:payload[n]=(B/n).read_bytes()
    for folder in ['docs','licenses']:
        for p in (B/folder).rglob('*'):
            if p.is_file() and p.name not in ('TOOLCHAIN.json','piclidean-banner.png','INSTALLATION-GEN2.md'):payload[p.relative_to(B).as_posix()]=p.read_bytes()
    payload['docs/INSTALLATION.md']=guide.encode('utf-8')
    payload['licenses/MPC-VST-LICENSE']=(B/'third_party/mpc-vst-wrapper/LICENSE').read_bytes()
    payload['licenses/FONT-OFL.txt']=(B/'reference/fonts/OFL.txt').read_bytes()
    payload['SHA256SUMS']=''.join(digest(d)+'  '+n+'\n' for n,d in sorted(payload.items())).encode()
    prefix='piClideaN-MPC-'+VERSION
    dest=B/'Installation'/name;dest.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=dest.parent,suffix='.partial',delete=False) as f:temp=Path(f.name)
    try:
        with zipfile.ZipFile(temp,'w',zipfile.ZIP_DEFLATED) as z:
            for n,d in sorted(payload.items()):
                if '..' in Path(n).parts or n.startswith('/') or '\\' in n:raise RuntimeError('Invalid package path')
                r=zipfile.ZipInfo(prefix+'/'+n,(2026,10,23,0,0,0));r.create_system=3
                r.external_attr=(0o100755 if n.endswith('/piclidean_mpc.so') else 0o100644)<<16;r.compress_type=zipfile.ZIP_DEFLATED
                z.writestr(r,d)
        with zipfile.ZipFile(temp) as z:
            if z.testzip():raise RuntimeError('ZIP CRC mismatch')
            for n,d in payload.items():
                if z.read(prefix+'/'+n)!=d:raise RuntimeError('ZIP content mismatch')
        os.replace(temp,dest)
    finally:
        if temp.exists():temp.unlink()
    dest.with_suffix(dest.suffix+'.sha256').write_text(digest(dest.read_bytes())+'  '+dest.name+'\n')
    print(dest)
if __name__=='__main__':main()

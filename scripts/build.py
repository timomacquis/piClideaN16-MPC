"""Local ARMv7 build. Never accesses or restarts a device."""
from pathlib import Path
import os,subprocess,sys,json,re,hashlib,struct
from project_config import VERSION,FOLDER
from provenance import inputs,tree,timestamp,build_date
B=Path(__file__).resolve().parents[1];tools=B/'third_party/mpc-vst-wrapper/tools'
if os.environ.get('GEN_VST_RELOCK'):raise SystemExit('Forced parameter relock is forbidden in this build')
subprocess.run([sys.executable,str(B/'scripts/generate.py')],check=True)
subprocess.run([sys.executable,str(B/'scripts/prepare_wrapper.py')],check=True)
sys.path.insert(0,str(tools));import gen_vst
params=json.loads((B/'params.json').read_text());keys=[p['key'] for p in params];lock=B/'params.lock'
if lock.exists():
 old=lock.read_text().split();assert len(keys)>=len(old) and keys[:len(old)]==old,'Published parameter indices changed'
gen_vst.check_lock(str(B),params)
(B/'build').mkdir(exist_ok=True);cfg=gen_vst.load(str(B/'vst.json'));gen_vst.gen_params(cfg,params,str(B/'build/params.h'))
auto=[int(bool(re.fullmatch(r'l(?:[0-9]|1[0-5])_(?:[a-z]+|s[0-9]+_[a-z]+|mod_mix|mod[01]_(?:[a-z]+|c[0-7]))',p['key'])) or p['key'] in ('channel','bank','target_mode')) for p in params]
with (B/'build/params.h').open('a') as f:
 f.write('\nstatic const unsigned char PICN_AUTOMATABLE[NPARAMS]={'+','.join(map(str,auto))+'};\n')
 f.write('static const unsigned short PICN_PARAM_LOOKUP[NPARAMS]={'+','.join(map(str,sorted(range(len(keys)),key=lambda i:keys[i])))+'};\n')
root=Path(os.environ.get('PICN_SYSROOT',str(B/'toolchain/root'))).resolve();prefix=root/'usr';cc=prefix/'bin/arm-linux-gnueabihf-gcc-12';alsa=next(iter(sorted(root.glob('usr/lib/arm-linux-gnueabihf/libasound.so.2*'))),None)
if not cc.exists() or alsa is None:raise SystemExit('Missing local compiler/ALSA runtime. See docs/BUILD.md')
env=dict(os.environ,PATH=str(prefix/'bin')+':'+os.environ.get('PATH',''),LD_LIBRARY_PATH=str(prefix/'lib/x86_64-linux-gnu')+':'+os.environ.get('LD_LIBRARY_PATH',''))
cmd=[str(cc),'--sysroot='+str(root),'-std=c11','-D_POSIX_C_SOURCE=200809L','-O2','-fPIC','-fvisibility=hidden','-fno-common','-Wall','-Wextra','-Wno-unused-parameter','-Wno-misleading-indentation','-march=armv7-a','-mfpu=neon','-mfloat-abi=hard','-Ibuild','-Isrc','-Ithird_party/mpc-vst-wrapper/wrapper','-Ithird_party/alsa-dev/usr/include','-shared','src/engine.c','third_party/mpc-vst-wrapper/wrapper/vst2_wrap.c',str(alsa),'-static-libgcc','-lpthread','-lm','-ldl','-Wl,--no-undefined','-Wl,-z,relro,-z,now,-z,noexecstack','-o','build/piclidean_mpc.so']
source_inputs=inputs()
result=subprocess.run(cmd,cwd=B,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(B/'build/compile.log').write_text(result.stdout);print(result.stdout)
if result.returncode:raise SystemExit(result.returncode)
subprocess.run([str(prefix/'bin/arm-linux-gnueabihf-strip'),str(B/'build/piclidean_mpc.so')],env=env,check=True)
python=os.environ.get('PICN_SKIN_PYTHON',sys.executable)
script=str(B/'scripts/build_skin.py')
if python.lower().endswith('.exe') and script.startswith('/mnt/c/'):script='C:/'+script[len('/mnt/c/'):]

subprocess.run([python,script],check=True,cwd=B)
readelf=prefix/'bin/arm-linux-gnueabihf-readelf';elf=B/'build/piclidean_mpc.so'
report=subprocess.check_output([str(readelf),'-h','-d','--version-info','-Ws',str(elf)],env=env,text=True);(B/'build/reports/ELF.txt').write_text(report)
raw=elf.read_bytes();assert raw[:5]==b'\x7fELF\x01' and int.from_bytes(raw[18:20],'little')==40
info={'version':VERSION,'date':build_date(),'timestamp_utc':timestamp(),'architecture':'ELF32 ARM hard-float','parameters':len(params),'automatable_canonical_parameters':sum(auto),'engine_sha256':hashlib.sha256(raw).hexdigest(),'params_lock_sha256':hashlib.sha256(lock.read_bytes()).hexdigest(),'glibc_versions':sorted(set(re.findall(r'GLIBC_[0-9.]+',report))),'compiler_command':cmd,'hardware_access':False}
skin_receipt=json.loads((B/'build/skin-receipt.json').read_text())
assert source_inputs==inputs(), 'Build inputs changed during compilation; rebuild required'
assert skin_receipt['version']==VERSION and skin_receipt['inputs']==inputs(), 'Skin inputs changed during build'
assert skin_receipt['outputs']==tree(B/'build/skin'/FOLDER), 'Skin output changed during build'
info['input_hashes']=inputs();info['skin_output_hashes']=skin_receipt['outputs']
(B/'build/reports/BUILD.json').write_text(json.dumps(info,indent=2)+'\n');print('Built',elf,info['engine_sha256'])

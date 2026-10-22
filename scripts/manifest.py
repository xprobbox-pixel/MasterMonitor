import hashlib,json,os,pathlib,subprocess,sys
from identity import identity,ROOT
root=pathlib.Path(sys.argv[1]).resolve()
def output(args):
    p=subprocess.run(args,capture_output=True,text=True)
    return {'exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}
files=[]
for path in sorted(root.rglob('*')):
    if not path.is_file() or path.name=='manifest.json': continue
    entry={'path':str(path.relative_to(root)),'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    if sys.platform=='darwin' and (path.suffix=='.pkg' or 'Contents/MacOS/' in str(path)):
        if path.suffix=='.pkg': entry['signature']=output(['pkgutil','--check-signature',str(path)])
        else:
            entry['architectures']=output(['lipo','-archs',str(path)])
            entry['signature']=output(['codesign','-dv','--verbose=2',str(path)])
    if sys.platform=='win32' and path.suffix=='.exe': entry['signature']=output(['powershell','-NoProfile','-Command',f'(Get-AuthenticodeSignature -LiteralPath \'{str(path).replace(chr(39),chr(39)*2)}\').Status'])
    files.append(entry)
notary=root/'validation/notarisation.json'
commit=output(['git','-C',str(ROOT),'rev-parse','HEAD'])
validation=root/'validation/results.json'
manifest={'source_commit':commit['stdout'] if commit['exit']==0 else None,'identity':identity(),'validation':json.loads(validation.read_text()) if validation.exists() else 'See native CI logs; summary not supplied','product':'MASTER MONITOR','version':identity()['MM_VERSION'],'build_host':sys.platform,'notarisation':json.loads(notary.read_text()) if notary.exists() else 'not performed','artifacts':files}
(root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

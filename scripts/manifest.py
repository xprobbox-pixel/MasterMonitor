"""Hash release artifacts and inspect native headers without executing binaries."""
import hashlib,json,pathlib,struct,subprocess,sys
from identity import identity,ROOT

def output(args):
    p=subprocess.run(args,capture_output=True,text=True)
    return {'exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}

def pe_metadata(data):
    if len(data)<64 or data[:2]!=b'MZ': return None
    offset=struct.unpack_from('<I',data,0x3c)[0]
    if offset+24>len(data) or data[offset:offset+4]!=b'PE\0\0': return None
    machine=struct.unpack_from('<H',data,offset+4)[0]
    magic=struct.unpack_from('<H',data,offset+24)[0]
    if magic not in (0x10b,0x20b): return None
    directory=offset+24+(112 if magic==0x20b else 96)
    if directory+40>len(data): return None
    cert_offset,cert_size=struct.unpack_from('<II',data,directory+32)
    unsigned=cert_offset==0 and cert_size==0
    return {'architecture':{0x8664:'x86_64',0x14c:'x86',0xaa64:'arm64'}.get(machine,hex(machine)),
            'machine':hex(machine),'certificate_table':{'offset':cert_offset,'size':cert_size},
            'signature':{'status':'unsigned' if unsigned else 'certificate present; cryptographic verification required',
                         'method':'PE certificate-table inspection'}}

def main(root):
    files=[]
    for path in sorted(root.rglob('*')):
        if not path.is_file() or path.name=='manifest.json': continue
        data=path.read_bytes()
        entry={'path':path.relative_to(root).as_posix(),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        pe=pe_metadata(data)
        if pe is not None: entry.update(pe)
        elif sys.platform=='darwin' and (path.suffix=='.pkg' or 'Contents/MacOS/' in str(path)):
            if path.suffix=='.pkg': entry['signature']=output(['pkgutil','--check-signature',str(path)])
            else:
                entry['architectures']=output(['lipo','-archs',str(path)])
                entry['signature']=output(['codesign','-dv','--verbose=2',str(path)])
        files.append(entry)
    notary=root/'validation/notarisation.json'
    commit=output(['git','-C',str(ROOT),'rev-parse','HEAD'])
    validation=root/'validation/results.json'
    manifest={'source_commit':commit['stdout'] if commit['exit']==0 else None,'identity':identity(),
              'validation':json.loads(validation.read_text()) if validation.exists() else 'See native CI logs; summary not supplied',
              'product':'MASTER MONITOR','version':identity()['MM_VERSION'],'manifest_host':sys.platform,
              'notarisation':json.loads(notary.read_text()) if notary.exists() else 'not performed','artifacts':files}
    (root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if __name__=='__main__': main(pathlib.Path(sys.argv[1]).resolve())

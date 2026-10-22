"""Independent FFmpeg ebur128 filter reference; requires ffmpeg on PATH.
Generated fixtures, no downloaded or user audio. Tolerances: 0.2 LU / 0.6 dBTP.
"""
import array,json,math,pathlib,re,subprocess,sys,tempfile,os
exe=str(pathlib.Path(sys.argv[1]).resolve())
results=[]
with tempfile.TemporaryDirectory() as temp:
 for rate in [44100,48000,96000]:
  for name in ['sine','gated','intersample']:
   frames=rate*(12 if name=='gated' else 4);samples=array.array('f')
   for i in range(frames):
    amplitude=10**(-23/20) if name=='sine' else (0.2 if i<rate*4 else 0.00001) if name=='gated' else .98
    frequency=1000 if name!='intersample' else rate/4
    value=amplitude*math.sin(2*math.pi*frequency*i/rate+(math.pi/4 if name=='intersample' else 0))
    samples.extend((value,value))
   path=pathlib.Path(temp)/'signal.raw';path.write_bytes(samples.tobytes())
   actual=json.loads(subprocess.check_output([exe,'--analyze',str(path),str(rate)],text=True,env={**os.environ,'MM_NO_USER_SETTINGS':'1'}))
   proc=subprocess.run(['ffmpeg','-hide_banner','-nostdin','-f','f32le','-ar',str(rate),'-ac','2','-i',str(path),'-af','ebur128=peak=true','-f','null','-'],capture_output=True,text=True,check=True)
   summary=proc.stderr.split('Summary:')[-1]
   loud=float(re.search(r'I:\s*([-\d.]+) LUFS',summary)[1]);peak=float(re.search(r'Peak:\s*([-\d.]+) dBFS',summary)[1])
   passed=abs(actual['integrated']-loud)<=.2 and abs(actual['true_peak']-peak)<=.6
   results.append(dict(rate=rate,fixture=name,actual=actual,ffmpeg={'integrated':loud,'true_peak':peak},passed=passed))
print(json.dumps(results,indent=2))
sys.exit(0 if all(x['passed'] for x in results) else 1)

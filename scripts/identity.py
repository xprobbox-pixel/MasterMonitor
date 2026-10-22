"""Read the single identity source used by CMake and all packaging scripts."""
import pathlib, re, json
ROOT=pathlib.Path(__file__).resolve().parents[1]
def identity():
    return dict(re.findall(r'set\((MM_\w+)\s+"?([^"\s\)]+(?: [^"\)]+)?)"?\)', (ROOT/'cmake/Identity.cmake').read_text()))
if __name__=='__main__': print(json.dumps(identity(),indent=2))

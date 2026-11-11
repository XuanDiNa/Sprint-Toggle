"""Run artifact-verifier mutations against actual release bytes and ZIPs."""
import hashlib,io,json,struct,tempfile,zipfile
import jsonschema
from pathlib import Path
from verify_artifacts import ROOT,FILES,inspect,verify_zip

def archive(payload):
    out=io.BytesIO()
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
        for name,data in payload.items():z.writestr(name,data)
    return io.BytesIO(out.getvalue())

def expect_rejected(label,call):
    try:call()
    except (AssertionError,ValueError,KeyError,struct.error,jsonschema.ValidationError):
        print('REFUSED '+label);return
    raise AssertionError('Mutation accepted: '+label)

def main():
    dll=(ROOT/'build/windows/x64/release/ToggleSprint.dll').read_bytes()
    inspect(dll)
    evidence=json.loads((ROOT/'profile/native-evidence.json').read_text())
    guard=bytes.fromhex(evidence['guards'][-1]['bytes'])
    assert dll.count(guard)==1
    bad=dll.replace(guard,bytes([guard[0]^1])+guard[1:],1)
    expect_rejected('altered embedded native guard',lambda:inspect(bad))
    payload={'ToggleSprint/'+n:(dll if n=='ToggleSprint.dll' else (ROOT/n).read_bytes()) for n in FILES}
    def checksum(p):
        files=[n for n in FILES if 'ToggleSprint/'+n in p]
        p['ToggleSprint/SHA256SUMS.txt']=''.join(hashlib.sha256(p['ToggleSprint/'+n]).hexdigest().upper()+'  '+n+'\n' for n in sorted(files)).encode()
    checksum(payload);verify_zip(archive(payload))
    variants=[]
    for name in ('extra.dll','../outside.txt','Minecraft.exe','private.i64','private.log','private.pdb'):
        bad=payload.copy();bad['ToggleSprint/'+name]=b'forbidden';variants.append((name,bad))
    bad=payload.copy();bad['ToggleSprint/README.md']+=b'changed';variants.append(('checksum mismatch',bad))
    for name,field,value in [('manifest.json','version','future'),('config.json','toggleKey','F25')]:
        bad=payload.copy();doc=json.loads(bad['ToggleSprint/'+name]);doc[field]=value
        bad['ToggleSprint/'+name]=json.dumps(doc).encode();checksum(bad);variants.append(('metadata '+name,bad))
    for label,p in variants:expect_rejected(label,lambda:verify_zip(archive(p)))
    update=payload.copy();del update['ToggleSprint/config.json'];checksum(update)
    verify_zip(archive(update),update=True)
    expect_rejected('config included in update ZIP',lambda:verify_zip(archive(payload),update=True))
    missing=update.copy();del missing['ToggleSprint/modmenu.settings.json'];checksum(missing)
    expect_rejected('update missing matching sidecar',lambda:verify_zip(archive(missing),update=True))
    # Extract the real update over a customized install and prove byte preservation.
    with tempfile.TemporaryDirectory(prefix='ToggleSprintUpdate-') as tmp:
        config=Path(tmp)/'ToggleSprint/config.json';config.parent.mkdir()
        existing=b'{ "mode": 2, "toggleKey": "SHIFT+F6" }\r\n'
        config.write_bytes(existing)
        with zipfile.ZipFile(archive(update)) as z:z.extractall(tmp)
        assert config.read_bytes()==existing,'Update replaced the existing configuration'
    print('Artifact mutations passed (1 binary guard, 11 ZIP variants); update preserves custom config')
if __name__=='__main__':main()

"""Inspect the actual DLL and reopened ZIP; refuse any non-allowlisted payload."""
import argparse,copy,hashlib,io,json,re,struct,tempfile,zipfile
from pathlib import Path,PurePosixPath
from verify_metadata import verify as metadata,VERSION
ROOT=Path(__file__).resolve().parents[1]
FILES={'ToggleSprint.dll','manifest.json','mod.json','config.json','config.schema.json','modmenu.settings.json','README.md','TESTING.md','THIRD_PARTY_NOTICES.txt'}
def inspect(data):
    assert data[:2]==b'MZ','PE signature'
    pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0'
    machine,count=struct.unpack_from('<HH',data,pe+4)
    assert machine==0x8664 and struct.unpack_from('<H',data,pe+22)[0]&0x2000,'AMD64 DLL required'
    assert struct.unpack_from('<H',data,pe+24)[0]==0x20b
    sections=[]
    table=pe+24+struct.unpack_from('<H',data,pe+20)[0]
    for i in range(count):
        virtual,rva,size,raw=struct.unpack_from('<IIII',data,table+40*i+8)
        assert raw+size<=len(data)
        sections.append((rva,size,raw))
    def offset(rva):
        for start,size,raw in sections:
            if start<=rva<start+size:return raw+rva-start
        raise AssertionError('Unmapped RVA')
    def string(rva):
        at=offset(rva);end=data.index(0,at)
        assert end-at<1024
        return data[at:end].decode('ascii')
    exports=[];rva=struct.unpack_from('<I',data,pe+136)[0]
    assert rva
    export=offset(rva);n=struct.unpack_from('<I',data,export+24)[0];names=offset(struct.unpack_from('<I',data,export+32)[0])
    assert n<32
    for i in range(n):exports.append(string(struct.unpack_from('<I',data,names+4*i)[0]))
    assert set(exports)=={'TS_GetStatus','TS_GetVersion','TS_Shutdown'},'Unexpected export'
    imports={};rva=struct.unpack_from('<I',data,pe+144)[0];assert rva
    at=offset(rva)
    while any(data[at:at+20]):
        thunk,_,_,name,iat=struct.unpack_from('<5I',data,at)
        dll=string(name).lower()
        assert dll in {'kernel32.dll','user32.dll','bcrypt.dll'},'Unexpected or dynamic-runtime dependency: '+dll
        symbols=[];cursor=offset(thunk or iat)
        while True:
            pointer=struct.unpack_from('<Q',data,cursor)[0];cursor+=8
            if not pointer:break
            assert not pointer>>63,'Unexpected ordinal import'
            symbols.append(string(pointer+2))
        imports[dll]=symbols;at+=20
    forbidden={'SendInput','keybd_event','mouse_event','socket','connect','send','recv','WSAStartup','WinHttpOpen','InternetOpenA','InternetOpenW'}
    assert not forbidden.intersection(s for names in imports.values() for s in names),'Forbidden imported API'
    for marker in [b'SendInput',b'keybd_event',b'WSAStartup',b'Minecraft.exe',b'inner-analysis-only']:
        assert marker not in data,'Private or forbidden binary marker'
    assert not re.search(rb'[A-Za-z]:[\\/][\x20-\x7e]{4,}',data),'Absolute binary path'
    assert VERSION.encode('ascii') in data
    # Confirm complete identity allowlist and selected hook RVAs are embedded.
    for h in [b'4A92BFA3CE2428B40EE517B7C1125D9F6F79274382DE03846351992663B2E2E4',b'0B66101DC873434E853A898329CB85CEE0DE52D0AE91AC796EFA0FE05B61522C']:
        assert h in data,'Missing full image identity'
    evidence=json.loads((ROOT/'profile/native-evidence.json').read_text())
    for row in evidence['targets']+evidence['guards']:
        assert bytes.fromhex(row['bytes']) in data,'Missing/altered embedded native guard: '+row['name']
    return {'architecture':'AMD64','bytes':len(data),'sha256':hashlib.sha256(data).hexdigest().upper(),'exports':exports,'imports':imports}

def verify_zip(path,update=False):
    with zipfile.ZipFile(path) as z:
        files=FILES-{'config.json'} if update else FILES
        names=z.namelist();expected={'ToggleSprint/'+n for n in files|{'SHA256SUMS.txt'}}
        assert len(names)==len(expected) and set(names)==expected,'ZIP allowlist mismatch'
        for info in z.infolist():
            p=PurePosixPath(info.filename)
            assert not p.is_absolute() and '..' not in p.parts and '\\' not in info.filename and ':' not in info.filename
            assert info.file_size<8*1024*1024 and (info.external_attr>>16)&0o170000!=0o120000,'Unsafe ZIP member'
        sums={}
        for line in z.read('ToggleSprint/SHA256SUMS.txt').decode('ascii').splitlines():
            h,n=line.split('  ',1);assert re.fullmatch('[A-F0-9]{64}',h) and n in files and n not in sums
            sums[n]=h
        assert set(sums)==files
        for n,h in sums.items():
            payload=z.read('ToggleSprint/'+n)
            assert hashlib.sha256(payload).hexdigest().upper()==h,'Checksum mismatch: '+n
            if n!='ToggleSprint.dll':
                assert b'D:\\' not in payload and b'C:\\Users' not in payload,'Private payload path'
        report=inspect(z.read('ToggleSprint/ToggleSprint.dll'))
        with tempfile.TemporaryDirectory(prefix='ToggleSprintZip-') as tmp:
            root=Path(tmp)
            for n in files-{'ToggleSprint.dll'}:(root/n).write_bytes(z.read('ToggleSprint/'+n))
            metadata(root,update=update)
        assert z.testzip() is None
        return report

def negative_tests(dll):
    data=dll.read_bytes()
    variants=[]
    bad=bytearray(data);pe=struct.unpack_from('<I',bad,60)[0];struct.pack_into('<H',bad,pe+4,0x14c);variants.append(bytes(bad))
    variants.extend([data+b'SendInput',data+b'Z:\\private-build\\private.pdb',data.replace(b'TS_GetVersion',b'XX_GetVersion')])
    for candidate in variants:
        try:inspect(candidate)
        except (AssertionError,ValueError):continue
        raise AssertionError('Bad binary accepted')
    print('4 binary negative mutations refused')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--dll',type=Path,default=ROOT/'build/windows/x64/release/ToggleSprint.dll');parser.add_argument('--zip',type=Path);parser.add_argument('--update-zip',type=Path);args=parser.parse_args()
    print(json.dumps(inspect(args.dll.read_bytes()),indent=2));negative_tests(args.dll)
    if args.zip:print('Reopened ZIP verified:',json.dumps(verify_zip(args.zip),indent=2))
    if args.update_zip:print('Reopened config-preserving update ZIP verified:',json.dumps(verify_zip(args.update_zip,update=True),indent=2))

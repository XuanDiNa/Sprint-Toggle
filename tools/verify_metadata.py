"""Check the actual packaged ModMenu contract and deliberate bad variants."""
import copy,json,re,sys
import jsonschema
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
VERSION='0.1.0-test7-leftalt'
def verify(folder,update=False):
    names=['manifest.json','mod.json','config.schema.json','modmenu.settings.json']
    if not update:names.append('config.json')
    data={n:json.loads((folder/n).read_text(encoding='utf-8')) for n in names}
    m=data['manifest.json'];s=data['modmenu.settings.json']
    assert m=={'name':'ToggleSprint','entry':'ToggleSprint.dll','version':VERSION,'type':'preload-native'}
    assert data['mod.json']['version']==VERSION and data['mod.json']['dll']=='ToggleSprint.dll'
    if not update:assert data['config.json']=={'mode':1,'toggleKey':'LCTRL'}
    assert s['schemaVersion']==5 and s['plugin']=={'id':'ToggleSprint','entry':'ToggleSprint.dll','versions':[VERSION]}
    assert s['config']=={'file':'config.json','reload':'hot'}
    assert len(s['fields'])==2
    mode,key=s['fields']
    assert (mode['path'],mode['type'],mode['default'])==('/mode','choice',1)
    assert [o['en_US'] for o in mode['options']]==['Off','Toggle Sprint','Force Sprint']
    assert (key['path'],key['type'],key['default'])==('/toggleKey','binding','LCTRL')
    assert 'keys' not in key
    for field in s['fields']:
        for name in ('label','description'):
            assert set(field[name])=={'en_US','zh_TW','zh_CN'} and all(field[name].values())
    jsonschema.Draft202012Validator.check_schema(data['config.schema.json'])
    if not update:jsonschema.validate(data['config.json'],data['config.schema.json'])
    schema=json.loads((ROOT/'vendor/modmenu/modmenu.settings.v5.schema.json').read_text())
    jsonschema.validate(s,schema)
    return data
if __name__=='__main__':
    import tempfile
    try:
        data=verify(ROOT)
        mutations=[('manifest.json',lambda d:d.update(version='future')),
           ('modmenu.settings.json',lambda d:d['fields'][0]['options'].reverse()),
           ('modmenu.settings.json',lambda d:d.update(schemaVersion=1)),
           ('modmenu.settings.json',lambda d:d['fields'][1].update(path='/other')),
           ('config.json',lambda d:d.update(toggleKey='F25'))]
        for name,mutate in mutations:
            with tempfile.TemporaryDirectory(prefix='ToggleSprintMetadata-') as tmp:
                candidate=copy.deepcopy(data);mutate(candidate[name])
                for n,obj in candidate.items():(Path(tmp)/n).write_text(json.dumps(obj),encoding='utf-8')
                try:verify(Path(tmp))
                except (AssertionError,ValueError,jsonschema.ValidationError):continue
                raise AssertionError('Bad metadata accepted')
        print('Metadata, JSON Schemas and 5 negative mutations passed')
    except Exception as e:print('Metadata rejected:',str(e));sys.exit(1)

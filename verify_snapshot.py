"""Verify copied snapshot files against the published SHA-256 manifest."""
from pathlib import Path
import hashlib,json,os
root=Path(__file__).resolve().parent
entries=json.loads((root/'upload-manifest.json').read_text())['files']
failures=[]
for entry in entries:
    path=root/entry['path']
    try:
        content=os.readlink(path).encode() if entry['kind']=='symlink' else path.read_bytes()
        if hashlib.sha256(content).hexdigest()!=entry['sha256']:
            failures.append(entry['path'])
        if entry['kind']=='symlink' and not path.exists():
            failures.append(entry['path']+' (broken link)')
    except OSError:
        failures.append(entry['path'])
if failures:
    raise SystemExit('Verification failed: '+', '.join(failures))
print(f'Verified {len(entries)} copied files and links.')

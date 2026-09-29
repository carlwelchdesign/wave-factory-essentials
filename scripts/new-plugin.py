#!/usr/bin/env python3
"""Create a new Essentials effect atomically, using only the Python standard library."""
import argparse
import json
from pathlib import Path
import plistlib
import re
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
STARTER_VERSION = '1.0.0'
FIELDS = {'internal_name', 'display_name', 'unique_id', 'bundle_identifier', 'version'}


def validate(data, root):
    if not isinstance(data, dict) or set(data) != FIELDS:
        raise ValueError('Manifest must contain exactly: ' + ', '.join(sorted(FIELDS)))
    if not all(isinstance(v, str) for v in data.values()):
        raise ValueError('All manifest values must be strings')
    name = data['internal_name']
    if not re.fullmatch(r'[A-Z][A-Za-z0-9]{2,47}', name):
        raise ValueError('internal_name must be 3-48 ASCII letters/digits, starting uppercase')
    if name.upper() in {'CON', 'PRN', 'AUX', 'NUL', *(f'COM{i}' for i in range(1, 10)), *(f'LPT{i}' for i in range(1, 10))}:
        raise ValueError('internal_name is reserved on Windows')
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 -]{0,63}', data['display_name']):
        raise ValueError('display_name must be 1-64 ASCII letters, digits, spaces or hyphens')
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9]{3}', data['unique_id']):
        raise ValueError('unique_id must be four ASCII alphanumeric characters, starting with a letter')
    if data['bundle_identifier'] != 'com.wavefactoryessentials.' + name:
        raise ValueError('bundle_identifier must be com.wavefactoryessentials.' + name)
    if not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', data['version']):
        raise ValueError('version must be major.minor.patch')
    if any(int(n) > 255 for n in data['version'].split('.')):
        raise ValueError('Version components must fit in 8 bits')
    for config in (root / 'plugins').rglob('config.h'):
        text = config.read_text()
        if config.parent.name.casefold() == name.casefold():
            raise ValueError('Plugin name already exists: ' + name)
        for key, value in [('PLUG_UNIQUE_ID', data['unique_id']), ('PLUG_NAME', data['display_name']), ('BUNDLE_NAME', name)]:
            match = re.search(r'#define\s+' + key + r'''\s+["']([^"']+)["']''', text)
            if match and match[1].casefold() == value.casefold():
                raise ValueError('Plugin identity collision: ' + key + ' ' + value)
    for manifest in (root / 'plugins').rglob('starter-manifest.json'):
        existing = json.loads(manifest.read_text())
        if existing['bundle_identifier'].casefold() == data['bundle_identifier'].casefold():
            raise ValueError('Bundle identifier already exists')
    destination = root / 'plugins' / 'generated' / name
    if destination.exists():
        raise ValueError('Destination already exists: ' + str(destination))
    return destination


def render(data):
    major, minor, patch = map(int, data['version'].split('.'))
    version = (major << 16) | (minor << 8) | patch
    values = dict(data, version_hex=f'0x{version:08X}', starter_version=STARTER_VERSION)
    files = {}
    for template in sorted((ROOT / 'starter' / 'templates').rglob('*.in')):
        relative = str(template.relative_to(ROOT / 'starter' / 'templates'))[:-3].replace('Plugin', data['internal_name'])
        body = template.read_text()
        for key, value in values.items():
            body = body.replace('@' + key + '@', value)
        files[relative] = body.encode()
    name = data['internal_name']
    for fmt, category in [('AU', 'audiounit'), ('VST3', 'vst3'), ('CLAP', 'clap')]:
        info = dict(CFBundleDevelopmentRegion='English', CFBundleExecutable=name,
                    CFBundleGetInfoString=data['display_name'] + ' ' + data['version'],
                    CFBundleIdentifier=f'com.wavefactoryessentials.{category}.{name}',
                    CFBundleInfoDictionaryVersion='6.0', CFBundleName=data['display_name'],
                    CFBundlePackageType='BNDL', CFBundleShortVersionString=data['version'],
                    CFBundleSignature=data['unique_id'], CFBundleVersion=data['version'],
                    CSResourcesFileMapped=True, LSMinimumSystemVersion='11.0')
        if fmt == 'AU':
            info.update({'AudioComponents': [dict(description=data['display_name'], factoryFunction=name + '_Factory',
                         manufacturer='WvFy', name='Carl Welch: ' + data['display_name'], sandboxSafe=True,
                         subtype=data['unique_id'], type='aufx', version=version)],
                         'AudioUnit Version': values['version_hex'], 'NSPrincipalClass': name + '_View'})
        files[f'resources/{name}-{fmt}-Info.plist'] = plistlib.dumps(info)
    files['starter-manifest.json'] = (json.dumps(dict(data, starter_version=STARTER_VERSION), indent=2) + '\n').encode()
    return files


def generate(manifest, root=ROOT, dry_run=False):
    data = json.loads(Path(manifest).read_text())
    root = Path(root).resolve()
    destination = validate(data, root)
    files = render(data)
    if not dry_run:
        destination.parent.mkdir(parents=True, exist_ok=True)
        lock = destination.parent / '.starter-generation.lock'
        lock.mkdir()  # Exclusive reservation across all names/IDs; never steal a stale lock.
        temporary = None
        try:
            validate(data, root)  # Recheck collisions under the lock.
            temporary = Path(tempfile.mkdtemp(prefix='.starter-', dir=destination.parent))
            for relative, body in files.items():
                target = temporary / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(body)
            temporary.rename(destination)  # Same filesystem: expose only the complete product.
        finally:
            if temporary is not None and temporary.exists():
                shutil.rmtree(temporary)
            lock.rmdir()
    return {'dry_run': dry_run, 'destination': str(destination), 'files': sorted(files), 'starter_version': STARTER_VERSION}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    try:
        print(json.dumps(generate(args.manifest, dry_run=args.dry_run), indent=2))
    except (ValueError, OSError) as error:
        parser.exit(2, f'new-plugin: {error}\n')

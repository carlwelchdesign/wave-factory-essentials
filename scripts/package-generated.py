#!/usr/bin/env python3
"""Package every manifest-registered generated plugin; never install or publish."""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import shutil
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def inputs_for(root, build, platform):
    result = []
    for manifest in sorted((root / 'plugins' / 'generated').glob('*/starter-manifest.json')):
        data = json.loads(manifest.read_text())
        name = data['internal_name']
        if name != manifest.parent.name or not re.fullmatch(r'[A-Z][A-Za-z0-9]{2,47}', name):
            raise ValueError('Invalid manifest identity: ' + str(manifest))
        if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', data['version']):
            raise ValueError('Invalid manifest version: ' + str(manifest))
        paths = []
        for extension in (['component', 'vst3', 'clap'] if platform == 'macOS' else ['vst3', 'clap']):
            # iPlug2 uses out/<config> on macOS Xcode and out on Windows.
            candidates = [p for p in (build / 'out').rglob(name + '.' + extension)
                          if (p.is_file() if platform == 'Windows' and extension == 'clap' else p.is_dir())]
            if len(candidates) != 1:
                raise ValueError(f'Expected exactly one {name}.{extension} under {build}/out, found {len(candidates)}')
            bundle = candidates[0]
            if platform == 'macOS':
                info = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
                category = {'component': 'audiounit', 'vst3': 'vst3', 'clap': 'clap'}[extension]
                if info['CFBundleIdentifier'] != f'com.wavefactoryessentials.{category}.{name}' or info['CFBundleShortVersionString'] != data['version']:
                    raise ValueError('Bundle metadata does not match manifest: ' + str(bundle))
                binary = bundle / 'Contents/MacOS' / name
            elif extension == 'vst3':
                binary = bundle / 'Contents/x86_64-win' / (name + '.vst3')
            else:
                binary = bundle
            if not binary.is_file() or binary.stat().st_size == 0:
                raise ValueError('Missing or empty binary: ' + str(binary))
            for asset in (manifest.parent / 'resources').rglob('*'):
                if asset.suffix not in ('.png', '.ttf'):
                    continue
                if platform == 'macOS' and not (bundle / 'Contents/Resources' / asset.name).is_file():
                    raise ValueError('Missing packaged resource: ' + asset.name)
            paths.append(bundle)
        result.append((manifest, data, paths))
    if not result:
        raise ValueError('No generated plugin manifests found')
    return result


def package(root, build, output, platform):
    products = inputs_for(root, build, platform)  # Validate every product before writing any archive.
    notices = [root / name for name in ('LICENSE', 'LICENSE-NOTICE.md') if (root / name).is_file()]
    if not notices:
        raise ValueError('No repository license or licensing notice found')
    output.mkdir(parents=True, exist_ok=True)
    written = []
    with tempfile.TemporaryDirectory(prefix='wfe-packages-') as temp:
        temp = Path(temp)
        for manifest, data, bundles in products:
            filename = f"{data['internal_name']}-{data['version']}-{platform}-tester.zip"
            archive = temp / filename
            with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
                for bundle in bundles:
                    if bundle.is_file():
                        z.write(bundle, bundle.name)
                    else:
                        for entry in sorted(bundle.rglob('*')):
                            if entry.is_symlink():
                                raise ValueError('Unexpected bundle symlink: ' + str(entry))
                            if entry.is_file():
                                z.write(entry, str(Path(bundle.name) / entry.relative_to(bundle)))
                z.write(manifest, 'starter-manifest.json')
                z.write(manifest.parent / 'README.md', 'README.md')
                for notice in notices:
                    z.write(notice, notice.name)
                z.writestr('INSTALL.txt', 'Tester artifact. Close all DAWs before installation.\n'
                           'macOS: copy .component to ~/Library/Audio/Plug-Ins/Components, .vst3 to VST3, .clap to CLAP under the same Plug-Ins directory.\n'
                           'Windows: use the host-supported VST3/CLAP folder and rescan.\n'
                           'Production signing/notarization and real host acceptance are separate gates.\n')
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            (temp / (filename + '.sha256')).write_text(digest + '  ' + filename + '\n')
            written.append(filename)
        for entry in temp.iterdir():
            shutil.copy2(entry, output / entry.name)
    return written


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', required=True, type=Path)
    parser.add_argument('--output-dir', default=ROOT / 'dist/generated', type=Path)
    parser.add_argument('--platform', choices=['macOS', 'Windows'], required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(package(ROOT, args.build_dir, args.output_dir, args.platform), indent=2))
    except (OSError, ValueError, KeyError) as error:
        parser.exit(2, f'package-generated: {error}\n')

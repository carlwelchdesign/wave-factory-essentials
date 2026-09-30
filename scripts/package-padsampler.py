#!/usr/bin/env python3
"""Package the standalone/AU/VST3 instrument, preserving bundles with Apple's ditto."""
import argparse
import hashlib
import plistlib
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def package(build, output, framework):
    bundles = []
    for extension in ('app', 'component', 'vst3'):
        candidates = list((build / 'out').glob(f'**/PadSampler.{extension}'))
        if len(candidates) != 1:
            raise ValueError(f'Expected one PadSampler.{extension}; found {len(candidates)}')
        bundle = candidates[0]
        info = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
        category = {'app': 'app', 'component': 'audiounit', 'vst3': 'vst3'}[extension]
        if info.get('CFBundleIdentifier') != f'com.wavefactoryessentials.{category}.PadSampler':
            raise ValueError('Wrong bundle identity')
        if extension == 'component' and info.get('AudioComponents', [{}])[0].get('type') != 'aumu':
            raise ValueError('AU must be an instrument')
        if info.get('CFBundleShortVersionString') != '0.1.0':
            raise ValueError('Wrong bundle version')
        binary = bundle / 'Contents/MacOS/PadSampler'
        architectures = subprocess.check_output(['lipo', '-archs', str(binary)], text=True).split()
        if set(architectures) != {'arm64', 'x86_64'}:
            raise ValueError('Expected universal arm64/x86_64 executable')
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(bundle)], check=True)
        bundles.append(bundle)
    output.mkdir(parents=True, exist_ok=True)
    archive = output / 'PadSampler-0.1.0-macOS-tester.zip'
    with tempfile.TemporaryDirectory(prefix='padsampler-package-') as temp:
        staging = Path(temp) / 'PadSampler'; staging.mkdir()
        for bundle in bundles:
            subprocess.run(['ditto', str(bundle), str(staging / bundle.name)], check=True)
        shutil.copy2(ROOT / 'plugins/PadSampler/README.md', staging / 'README.md')
        for name in ('LICENSE', 'LICENSE-NOTICE.md'):
            if (ROOT / name).exists(): shutil.copy2(ROOT / name, staging / name)
        shutil.copy2(framework / 'LICENSE.txt', staging / 'iPlug2-LICENSE.txt')
        subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(staging), str(archive)], check=True)
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None: raise ValueError('ZIP CRC failure')
    archive.with_suffix('.zip.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest() + '  ' + archive.name + '\n')
    print(archive)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'dist/padsampler')
    parser.add_argument('--framework-dir', type=Path, default=ROOT / 'third_party/iPlug2')
    args = parser.parse_args()
    package(args.build_dir.resolve(), args.output_dir.resolve(), args.framework_dir.resolve())

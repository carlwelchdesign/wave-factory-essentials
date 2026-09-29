"""Generator and packaging contracts; fixtures are created outside tracked source."""
import importlib.util
import json
from pathlib import Path
import plistlib
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def load(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

generator = load('generator', 'scripts/new-plugin.py')
packager = load('packager', 'scripts/package-generated.py')

class StarterTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='wfe starter tests ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = self.root / 'input.json'
        self.data = json.loads((ROOT / 'starter/manifests/StarterGain.json').read_text())
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps(self.data))

    def create(self):
        return Path(generator.generate(self.manifest, self.root)['destination'])

    def test_dry_run_has_no_side_effects(self):
        before = sorted(self.root.rglob('*'))
        result = generator.generate(self.manifest, self.root, True)
        self.assertEqual(before, sorted(self.root.rglob('*')))
        self.assertEqual(len(result['files']), 12)

    def test_two_distinct_products_metadata_and_no_placeholders(self):
        for name, code in [('StarterGain', 'WsG1'), ('StarterTrim', 'WsT1')]:
            self.data.update(internal_name=name, display_name=name, unique_id=code,
                             bundle_identifier='com.wavefactoryessentials.' + name)
            self.save()
            destination = self.create()
            for path in destination.rglob('*'):
                if path.is_file():
                    self.assertNotIn('@internal_name@', path.read_text())
            au = plistlib.loads((destination / f'resources/{name}-AU-Info.plist').read_bytes())
            self.assertEqual(au['AudioComponents'][0]['subtype'], code)
            self.assertEqual(au['AudioComponents'][0]['version'], 256)
            self.assertEqual(au['CFBundleIdentifier'], f'com.wavefactoryessentials.audiounit.{name}')

    def test_repeat_refuses_to_overwrite(self):
        destination = self.create()
        file = destination / 'GainProcessor.h'
        file.write_text('owned changes')
        with self.assertRaises(ValueError): self.create()
        self.assertEqual(file.read_text(), 'owned changes')

    def test_invalid_inputs_leave_no_output(self):
        original = self.data.copy()
        for key, bad in [('internal_name', '../Escape'), ('internal_name', 'x;cmd'), ('display_name', '"injection'),
                         ('unique_id', 'a'), ('version', '1.0'), ('version', '256.0.0'),
                         ('bundle_identifier', 'com.other.Plugin'), ('display_name', None)]:
            self.data = dict(original, **{key: bad}); self.save()
            with self.subTest(key=key, bad=bad), self.assertRaises(ValueError): self.create()
        self.assertFalse((self.root / 'plugins').exists())

    def test_collisions_with_legacy_plugin(self):
        legacy = self.root / 'plugins/Legacy/config.h'
        legacy.parent.mkdir(parents=True)
        legacy.write_text("#define PLUG_UNIQUE_ID 'WsG1'\n")
        with self.assertRaisesRegex(ValueError, 'identity collision'): self.create()

    def test_existing_nonplugin_destination_preserved(self):
        destination = self.root / 'plugins/generated/StarterGain'
        destination.mkdir(parents=True)
        (destination / 'private.txt').write_text('keep')
        with self.assertRaises(ValueError): self.create()
        self.assertEqual((destination / 'private.txt').read_text(), 'keep')

    def test_write_failure_cleans_generated_files(self):
        with patch.object(Path, 'rename', side_effect=OSError('disk unavailable')):
            with self.assertRaises(OSError): self.create()
        self.assertFalse((self.root / 'plugins/generated/StarterGain').exists())
        self.assertEqual(list((self.root / 'plugins/generated').iterdir()), [])

    def test_active_lock_is_preserved(self):
        lock = self.root / 'plugins/generated/.starter-generation.lock'
        lock.mkdir(parents=True)
        with self.assertRaises(FileExistsError): self.create()
        self.assertTrue(lock.is_dir())
        self.assertFalse((lock.parent / 'StarterGain').exists())

    def test_windows_reserved_name_is_rejected(self):
        self.data.update(internal_name='CON', bundle_identifier='com.wavefactoryessentials.CON')
        self.save()
        with self.assertRaisesRegex(ValueError, 'reserved'): self.create()

    def fake_bundles(self):
        product = self.create()
        (self.root / 'LICENSE').write_text('test license')
        for extension, fmt in [('component', 'AU'), ('vst3', 'VST3'), ('clap', 'CLAP')]:
            bundle = self.root / f'build/out/Release/StarterGain.{extension}/Contents'
            (bundle / 'MacOS').mkdir(parents=True)
            binary = bundle / 'MacOS/StarterGain'
            binary.write_bytes(b'fixture binary - not a real executable')
            binary.chmod(0o755)
            (bundle / 'Info.plist').write_bytes((product / f'resources/StarterGain-{fmt}-Info.plist').read_bytes())

    def test_package_fail_closed_before_writing(self):
        self.fake_bundles()
        (self.root / 'build/out/Release/StarterGain.clap/Contents/MacOS/StarterGain').unlink()
        with self.assertRaises(ValueError):
            packager.package(self.root, self.root / 'build', self.root / 'dist', 'macOS')
        self.assertFalse((self.root / 'dist').exists())

    def test_package_preserves_metadata_and_executable_permissions(self):
        self.fake_bundles()
        names = packager.package(self.root, self.root / 'build', self.root / 'dist', 'macOS')
        with zipfile.ZipFile(self.root / 'dist' / names[0]) as z:
            self.assertIn('starter-manifest.json', z.namelist())
            entry = z.getinfo('StarterGain.component/Contents/MacOS/StarterGain')
            self.assertEqual((entry.external_attr >> 16) & 0o777, 0o755)
        self.assertTrue((self.root / 'dist' / (names[0] + '.sha256')).exists())

    def test_notice_only_baseline_packages_without_inventing_license(self):
        self.fake_bundles()
        (self.root / 'LICENSE').rename(self.root / 'LICENSE-NOTICE.md')
        names = packager.package(self.root, self.root / 'build', self.root / 'dist', 'macOS')
        with zipfile.ZipFile(self.root / 'dist' / names[0]) as z:
            self.assertIn('LICENSE-NOTICE.md', z.namelist())
            self.assertNotIn('LICENSE', z.namelist())

    def test_missing_asset_and_wrong_version_rejected(self):
        self.fake_bundles()
        assets = self.root / 'plugins/generated/StarterGain/resources/img'
        assets.mkdir()
        (assets / 'scene.png').write_bytes(b'fixture')
        with self.assertRaisesRegex(ValueError, 'resource'):
            packager.inputs_for(self.root, self.root / 'build', 'macOS')
        (assets / 'scene.png').unlink()
        info_path = self.root / 'build/out/Release/StarterGain.component/Contents/Info.plist'
        info = plistlib.loads(info_path.read_bytes()); info['CFBundleShortVersionString'] = '9.9.9'
        info_path.write_bytes(plistlib.dumps(info))
        with self.assertRaisesRegex(ValueError, 'metadata'):
            packager.inputs_for(self.root, self.root / 'build', 'macOS')

    def test_windows_layout_and_missing_binary(self):
        self.create()
        (self.root / 'LICENSE').write_text('fixture')
        vst = self.root / 'build/out/StarterGain.vst3/Contents/x86_64-win/StarterGain.vst3'
        vst.parent.mkdir(parents=True); vst.write_bytes(b'fixture')
        clap = self.root / 'build/out/StarterGain.clap'; clap.write_bytes(b'fixture')
        names = packager.package(self.root, self.root / 'build', self.root / 'dist', 'Windows')
        with zipfile.ZipFile(self.root / 'dist' / names[0]) as z:
            self.assertIn('StarterGain.clap', z.namelist())
            self.assertIn('StarterGain.vst3/Contents/x86_64-win/StarterGain.vst3', z.namelist())
        clap.unlink()
        with self.assertRaises(ValueError):
            packager.inputs_for(self.root, self.root / 'build', 'Windows')

if __name__ == '__main__': unittest.main()

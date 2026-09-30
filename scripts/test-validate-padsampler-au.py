#!/usr/bin/env python3
"""Registration delay is retryable; AU validation errors must fail immediately."""
import importlib.util
import io
from pathlib import Path
import subprocess
import unittest

spec = importlib.util.spec_from_file_location('validator', Path(__file__).with_name('validate-padsampler-au.py'))
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

class RegistrationTests(unittest.TestCase):
    def check_results(self, results, expected, attempts=12):
        pending = iter(results)
        calls, pauses, log = [], [], io.StringIO()
        def run(command, **kwargs):
            calls.append(command)
            return next(pending)
        self.assertEqual(validator.validate(log, run, pauses.append, attempts), expected)
        self.assertEqual(len(calls), len(results))
        self.assertEqual(len(pauses), max(0, len(results)-1))
        return log.getvalue()

    def test_delayed_registration(self):
        output = self.check_results([
            subprocess.CompletedProcess([], 2, validator.NOT_REGISTERED),
            subprocess.CompletedProcess([], 0, 'AU VALIDATION SUCCEEDED')], 0)
        self.assertIn(validator.NOT_REGISTERED, output)
        self.assertIn('AU VALIDATION SUCCEEDED', output)

    def test_real_validation_failure_is_not_retried(self):
        self.check_results([subprocess.CompletedProcess([], 1, 'FAIL: render validation')], 1)

    def test_missing_component_still_fails(self):
        self.check_results([subprocess.CompletedProcess([], 2, validator.NOT_REGISTERED)]*3, 2, 3)

if __name__ == '__main__':
    unittest.main()

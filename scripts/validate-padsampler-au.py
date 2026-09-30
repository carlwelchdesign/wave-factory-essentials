#!/usr/bin/env python3
"""Validate PadSampler; retry only macOS component-registration misses, never test failures."""
import argparse
from pathlib import Path
import subprocess
import time

NOT_REGISTERED = "FATAL ERROR: didn't find the component"

def validate(log, run=subprocess.run, pause=time.sleep, attempts=12):
    for attempt in range(attempts):
        log.write(f"Registration/validation attempt {attempt + 1}\n")
        try:
            result = run(['auval', '-v', 'aumu', 'WfP6', 'WvFy'],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True, timeout=120)
        except subprocess.TimeoutExpired:
            log.write('auval timed out; validation failed\n')
            return 124
        log.write(result.stdout)
        log.flush()
        if result.returncode == 0 or NOT_REGISTERED not in result.stdout:
            return result.returncode
        if attempt + 1 < attempts:
            pause(5)
    return result.returncode

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', required=True, type=Path)
    args = parser.parse_args()
    args.log.parent.mkdir(parents=True, exist_ok=True)
    with args.log.open('w') as log:
        code = validate(log)
    print(args.log.read_text())
    raise SystemExit(code)

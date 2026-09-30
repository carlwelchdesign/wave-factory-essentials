#!/usr/bin/env python3
"""Opt-in macOS native UI regression. Uses a dedicated, freshly opened app instance.
Requires Accessibility permission and an idle desktop. Never target a working session.
Compile padsampler-ui-events.swift first; pass the dedicated app PID explicitly.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import subprocess
import time
import wave

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--pid', required=True, type=int)
p.add_argument('--events', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output = a.output.resolve()
a.output.mkdir(parents=True, exist_ok=False)
import os
env = dict(os.environ, PADSAMPLER_UI_PID=str(a.pid))

def event(*args):
    apple('')
    subprocess.run([str(a.events.resolve()), *map(str, args)], env=env, check=True, capture_output=True)
    time.sleep(.15)

def apple(body, activate=True):
    script = f'tell application "System Events"\n tell (first process whose unix id is {a.pid})\n set frontmost to true\n{body}\n end tell\nend tell'
    if not activate: script=script.replace(" set frontmost to true\n", "")
    return subprocess.check_output(['osascript', '-e', script], text=True)

def choose(path):
    time.sleep(.8)
    apple('keystroke "g" using {command down, shift down}\ndelay 0.5\nkeystroke '+json.dumps(str(path))+'\ndelay 0.3\nkey code 36\ndelay 0.8\nclick button "Open" of splitter group 1 of sheet 1 of window 1')
    time.sleep(.7)

def wait_sheet():
    for _ in range(30):
        if apple('return exists sheet 1 of window 1').strip()=='true': return
        time.sleep(.1)
    raise AssertionError('Native file dialog did not appear')

def openkit(path):
    event('click',580,37)
    wait_sheet()
    choose(path)

def save(name):
    path = a.output / (name+'.padkit')
    event('click',693,37)
    wait_sheet()
    time.sleep(.3)
    apple('set value of text field "Save As:" of splitter group 1 of sheet 1 of window 1 to '+json.dumps(path.name)+'\nkeystroke "g" using {command down, shift down}\ndelay 0.5\nkeystroke '+json.dumps(str(a.output))+'\ndelay 0.3\nkey code 36\ndelay 0.8\nclick button "Save" of splitter group 1 of sheet 1 of window 1')
    for _ in range(100):
        manifest = path / 'kit.json'
        if manifest.exists():
            for _ in range(50):
                if apple('return exists sheet 1 of window 1').strip()=='false':
                    time.sleep(.4)
                    return json.loads(manifest.read_text())
                time.sleep(.1)
            raise AssertionError('Save completed but its native sheet did not close')
        time.sleep(.05)
    raise AssertionError('Kit was not saved: '+name)

def capture(name):
    data=subprocess.check_output([str(a.events.resolve()),'window'],env=env,text=True)
    window=data.splitlines()[0]
    subprocess.run(['screencapture','-x','-l',window,str(a.output/(name+'.png'))],check=True)

params=[]
for _ in range(6): params += [-12,0,1,1500,18000,1,0,36+_,0]
params += [100,-6]
fixture={'version':2,'parameters':params,'slots':[{'name':n,'path':''} for n in ['Pad 1','Pad 2','Pad 3','Pad 4','External Tip','External Ring']],'curves':[{'legacy':False,'points':[[0,0],[1,1]]} for _ in range(6)]}
# A quiet synthetic fixture, not listening acceptance.
with wave.open(str(a.output/'qa.wav'),'wb') as f:
    f.setnchannels(1);f.setsampwidth(2);f.setframerate(48000)
    f.writeframes(b''.join(struct.pack('<h',int(1500*math.sin(2*math.pi*180*i/48000)*math.exp(-i/9000))) for i in range(24000)))
fixture['slots'][0]['path']='qa.wav'
(a.output/'initial.json').write_text(json.dumps(fixture))
apple('')
event('key',53) # Dismiss any leftover native popup before the fresh fixture.
time.sleep(1)
openkit(a.output/'initial.json')
event('click',120,160)
capture('01-ready')
# A single double-click inserts one point; dragging is a separate undoable edit.
event('double',780,520)
added=save('added')
assert len(added['curves'][0]['points'])==3, added['curves'][0]
event('drag',780,520,808,498)
edited=save('edited')
assert edited['curves'][0]!=added['curves'][0]
event('click',740,443)
assert save('undo')['curves'][0]==added['curves'][0]
event('click',800,443)
assert save('redo')['curves'][0]==edited['curves'][0]
# Delete the selected interior point, then undo the deletion.
event('click',808,498);event('key',51)
assert len(save('deleted')['curves'][0]['points'])==2
event('click',740,443)
assert save('undeleted')['curves'][0]==edited['curves'][0]
# Bypass is a retained parameter value; editing and shape remain intact.
event('click',898,360)
bypassed=save('bypassed');assert bypassed['parameters'][6]==1 and bypassed['curves']==edited['curves']
capture('02-bypassed')
event('click',898,360)
# Selection updates the inspector without affecting another slot's curve.
event('click',400,155)
assert save('other-slot')['curves'][1]==fixture['curves'][1]
event('click',120,155)
# Native Save cancellation must not leave a pressed button or mutate state.
event('click',693,37)
apple('click button "Cancel" of splitter group 1 of sheet 1 of window 1')
capture('03-dialog-cancelled')
# Restore an actual collected kit, including its sample and custom points.
openkit(a.output/'edited.padkit/kit.json')
recalled=save('recalled')
assert recalled['curves']==edited['curves'] and recalled['parameters']==edited['parameters']
# Name entry must survive periodic idle updates.
event('click',745,97)
apple('keystroke "a" using command down\nkeystroke "Precision Snare"\ndelay 1\nkey code 36')
assert save('renamed')['slots'][0]['name']=='Precision Snare'
# Learn state cancels; Help opens and Escape dismisses with pointer outside it.
event('click',897,135);capture('04-learn-waiting');event('click',897,135)
event('click',915,37);capture('05-help');event('move',10,740);event('key',53);capture('06-help-closed')
# All native menu choices must remain alive until asynchronous selection completes.
xs=[0,.125,.25,.5,.75,.875,1]
for i, name in enumerate(['linear','early','late','s-curve']):
    event('click',670,443)
    # Native type-ahead selects by label; do not reactivate the app while its menu tracks.
    menu_name=["Linear","Early Open","Late Open","S-Curve"][i]
    time.sleep(.6)
    apple('keystroke '+json.dumps(menu_name)+'\ndelay 0.2\nkey code 36', activate=False)
    time.sleep(.2)
    chosen=save('preset-'+name)
    points=chosen['curves'][0]['points']
    expected=[[x, x if i==0 else math.sqrt(x) if i==1 else x*x if i==2 else 3*x*x-2*x*x*x] for x in xs]
    assert points==expected, (name,points)
    assert chosen['curves'][1:]==recalled['curves'][1:]
capture('07-presets')
# Malformed restored curves must leave the complete current curve bank untouched.
malformed=json.loads(json.dumps(fixture));malformed['curves'][0]['points']=[[0,0],[.8,.8],[.4,.9],[1,1]]
(a.output/'invalid.json').write_text(json.dumps(malformed))
openkit(a.output/'invalid.json')
capture('08-invalid-kit')
assert save('after-invalid')['curves']==chosen['curves']
# Keyboard and numeric entry operate on the selected interior point.
event('click',780,519)
event('key',124)
apple('key code 126 using shift down', activate=False)
time.sleep(.2)
keyboard=save('keyboard')
assert abs(keyboard['curves'][0]['points'][3][0]-.51)<1e-12
assert abs(keyboard['curves'][0]['points'][3][1]-.501)<1e-12
for x,value in [(645,'70'),(775,'55')]:
    event('click',x,599)
    apple('keystroke "a" using command down', activate=False)
    for char in value:
        apple('keystroke '+json.dumps(char), activate=False)
        time.sleep(.15)
    apple('key code 36', activate=False)
    time.sleep(.2)
numeric=save('numeric')
assert numeric['curves'][0]['points'][3]==[70/127,.55]
capture('09-numeric')
# Clip editing uses the same inspector region but an independent history.
event('click',900,443)
capture('09a-clip-full')
event('drag',600,515,668,515)
start_drag=save('clip-start')
assert .09 < start_drag['slots'][0]['clip']['start'] < .11
event('drag',942,515,874,515)
end_drag=save('clip-end')
assert .38 < end_drag['slots'][0]['clip']['end'] < .42
event('click',740,443)
assert save('clip-undo')['slots'][0]['clip']==start_drag['slots'][0]['clip']
event('click',800,443)
assert save('clip-redo')['slots'][0]['clip']==end_drag['slots'][0]['clip']
event('click',668,515);event('key',124)
keyboard_clip=save('clip-keyboard')
assert keyboard_clip['slots'][0]['clip']['start']>end_drag['slots'][0]['clip']['start']
event('click',640,599)
apple('keystroke "a" using command down\nkeystroke "0.150"\nkey code 36',activate=False)
time.sleep(.2)
numeric_clip=save('clip-numeric')
assert abs(numeric_clip['slots'][0]['clip']['start']-.15)<1/48000
capture('09b-clip-trimmed')
event('click',640,443)
assert save('clip-reset')['slots'][0]['clip']=={'start':0.0,'end':None}
event('click',740,443)
assert save('clip-reset-undo')['slots'][0]['clip']==numeric_clip['slots'][0]['clip']
event('click',400,155)
assert save('clip-other-pad')['slots'][1]['clip']=={'start':0.0,'end':None}
event('click',120,155)
event('click',900,443)
# Momentary Stop All pressed and released states are captured for renderer review.
event('down',290,678);capture('10-pressed')
event('up',290,678);capture('11-released')
event('down',290,678);event('move',360,635);event('up',360,635);capture('12-pointer-exit')
# Missing-file and failed-replacement states must remain readable on the affected pad.
missing=json.loads(json.dumps(numeric));missing['slots'][0]['path']='missing.wav'
(a.output/'missing.json').write_text(json.dumps(missing))
openkit(a.output/'missing.json');capture('13-missing')
event('click',640,135);wait_sheet();choose(a.output/'qa.wav')
time.sleep(.5);capture('14-relinked')
(a.output/'invalid.wav').write_text('not a WAV file')
event('click',640,135);wait_sheet();choose(a.output/'invalid.wav')
time.sleep(.5);capture('15-import-error')
recovered=save('failed-replacement')
assert (a.output/'failed-replacement.padkit'/recovered['slots'][0]['path']).is_file()
(a.output/'result.txt').write_text('PASS: native tone and clip drag, keyboard, numeric, Reset, separate undo/redo, slot isolation, kit recall, text editing, Help, presets, malformed kit, and button states. Screenshots captured for visual review.\n')
print(a.output)

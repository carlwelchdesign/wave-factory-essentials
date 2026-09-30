// Native interaction regression helper. Coordinates are editor-local, not screen-dependent.
import Cocoa
import CoreGraphics
let args = CommandLine.arguments
let targetPID = ProcessInfo.processInfo.environment["PADSAMPLER_UI_PID"].flatMap { Int32($0) }
if let pid=targetPID {NSRunningApplication(processIdentifier:pid)?.activate(options:[.activateAllWindows]);usleep(600000)}
let windows = CGWindowListCopyWindowInfo(targetPID == nil ? [.optionOnScreenOnly, .excludeDesktopElements] : .optionAll, kCGNullWindowID) as? [[String:Any]] ?? []
guard let w = windows.first(where: { ($0[kCGWindowOwnerName as String] as? String) == "PadSampler" && (targetPID == nil || ($0[kCGWindowOwnerPID as String] as? Int32) == targetPID) && (($0[kCGWindowBounds as String] as? [String:Double])?["Width"] ?? 0) > 900 && (($0[kCGWindowBounds as String] as? [String:Double])?["Height"] ?? 0) > 700 }),
      let b = w[kCGWindowBounds as String] as? [String:Double] else { fatalError("Open the PadSampler standalone editor first") }
let origin = CGPoint(x: b["X"]!, y: b["Y"]! + b["Height"]! - 760)
func point(_ i:Int)->CGPoint { CGPoint(x: origin.x + Double(args[i])!, y: origin.y + Double(args[i+1])!) }
func mouse(_ type: CGEventType, _ p: CGPoint, _ count: Int64 = 1) { CGWarpMouseCursorPosition(p); let e=CGEvent(mouseEventSource:nil,mouseType:type,mouseCursorPosition:p,mouseButton:.left)!;e.setIntegerValueField(.mouseEventClickState,value:count);e.post(tap:.cghidEventTap) }
switch args[1] {
case "window": print(w[kCGWindowNumber as String]!); print(b)
case "click": mouse(.leftMouseDown,point(2));usleep(60000);mouse(.leftMouseUp,point(2))
case "double": for n:Int64 in 1...2 {mouse(.leftMouseDown,point(2),n);mouse(.leftMouseUp,point(2),n);usleep(65000)}
case "down": mouse(.leftMouseDown,point(2))
case "up": mouse(.leftMouseUp,point(2))
case "move": mouse(.mouseMoved,point(2))
case "drag":
 let a=point(2),z=point(4);mouse(.leftMouseDown,a);usleep(60000)
 for i in 1...20 {let t=Double(i)/20;mouse(.leftMouseDragged,CGPoint(x:a.x+(z.x-a.x)*t,y:a.y+(z.y-a.y)*t));usleep(16000)}
 mouse(.leftMouseUp,z)
case "key":
 let code=CGKeyCode(args[2])!;let down=CGEvent(keyboardEventSource:nil,virtualKey:code,keyDown:true)!,up=CGEvent(keyboardEventSource:nil,virtualKey:code,keyDown:false)!
 if args.count>3 && args[3]=="shift" {down.flags = .maskShift;up.flags = .maskShift}
 down.post(tap:.cghidEventTap);up.post(tap:.cghidEventTap)
default: fatalError("window | click/double/down/up/move x y | drag x y x y | key code [shift]")
}

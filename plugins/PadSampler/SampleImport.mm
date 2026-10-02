#include "SampleImport.h"
#import <AppKit/AppKit.h>

namespace padsampler {
namespace {
ImportedSample CopySelectedURL(NSURL* url) {
  ImportedSample result;
  if (!url || !url.isFileURL) { result.error = "Choose a local WAV or AIFF file"; return result; }
  NSString* extension = url.pathExtension.lowercaseString;
  if (![@[@"wav", @"aif", @"aiff"] containsObject:extension]) {
    result.error = "Choose an uncompressed WAV or AIFF file"; return result;
  }
  NSNumber* regular = nil;
  [url getResourceValue:&regular forKey:NSURLIsRegularFileKey error:nil];
  if (regular && !regular.boolValue) { result.error = "Choose an audio file, not a folder"; return result; }
  NSNumber* size = nil;
  [url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
  if (size && size.unsignedLongLongValue > 256ull * 1024ull * 1024ull) {
    result.error = "Source file exceeds the 256 MiB import limit"; return result;
  }
  const BOOL scoped = [url startAccessingSecurityScopedResource];
  NSFileManager* files = NSFileManager.defaultManager;
  NSError* error = nil;
  NSURL* support = [files URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&error];
  NSURL* folder = [[support URLByAppendingPathComponent:@"Ninth Chamber" isDirectory:YES] URLByAppendingPathComponent:@"PadSampler/Imported Samples" isDirectory:YES];
  if (support && [files createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:&error]) {
    NSString* unique = NSUUID.UUID.UUIDString;
    NSURL* target = [folder URLByAppendingPathComponent:[unique stringByAppendingFormat:@".%@", extension]];
    if ([files copyItemAtURL:url toURL:target error:&error]) {
      result.path = target.path.UTF8String;
      result.name = url.lastPathComponent.UTF8String;
    }
  }
  if (scoped) [url stopAccessingSecurityScopedResource];
  if (result.path.empty()) result.error = error ? [NSString stringWithFormat:@"Cannot import sample: %@", error.localizedDescription].UTF8String : "Cannot create PadSampler sample storage";
  return result;
}
}

ImportedSample ChooseImportedSample() {
  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedFileTypes = @[@"wav", @"aif", @"aiff"];
    if ([panel runModal] != NSModalResponseOK) { ImportedSample result; result.cancelled = true; return result; }
    return CopySelectedURL(panel.URL);
  }
}

ImportedSample ImportDroppedSample(const std::string& droppedPath) {
  @autoreleasepool {
    // Reading file URLs, rather than the legacy filename pasteboard property,
    // asks AppKit to transfer the drag's sandbox extension to the AU host.
    NSPasteboard* board = [NSPasteboard pasteboardWithName:NSDragPboard];
    NSArray<NSURL*>* urls = [board readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
    NSString* expected = [NSString stringWithUTF8String:droppedPath.c_str()];
    if (urls.count != 1 || !urls.firstObject.isFileURL || ![urls.firstObject.path.stringByStandardizingPath isEqualToString:expected.stringByStandardizingPath]) {
      ImportedSample result; result.error = "Cannot access dropped file in Logic. Use Load / Relink to choose it."; return result;
    }
    return CopySelectedURL(urls.firstObject);
  }
}
}

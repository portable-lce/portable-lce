#import <AppKit/AppKit.h>
#import <objc/runtime.h>

// Fixes a bug with SDL2 on macOS versions newer than 14.
// Slightly tweaked patch from this issue.
// resetCursorRects may in the future need patched.
// https://github.com/libsdl-org/SDL/issues/9745
// This has been fixed in SDL3 I believe.
void PatchSDLInvisibleCursor(void) {
    Class cls = objc_getMetaClass("NSCursor");
    if (!cls) return;

    SEL sel = sel_registerName("invisibleCursor");

    IMP replacement = imp_implementationWithBlock(^NSCursor*(id self) {
      NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(1, 1)];
      return [[NSCursor alloc] initWithImage:img hotSpot:NSZeroPoint];
    });

    Method m = class_getClassMethod(objc_getClass("NSCursor"), sel);
    if (m) method_setImplementation(m, replacement);
}

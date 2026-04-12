#import <AppKit/AppKit.h>
#import <objc/runtime.h>

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

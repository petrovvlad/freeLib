#include "myapplicationclass.h"
#include <QDebug>
#include <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <QApplication>

MyPrivate * MyPrivate::p = NULL;

void dockClickHandler(id self, SEL _cmd)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    MyPrivate::instance()->emitClick();
}

MyPrivate::MyPrivate() : QObject(NULL)
{
    /*
    objc_object* cls = [[[NSApplication sharedApplication] delegate] class];
    if (!class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
        NSLog(@"MyPrivate::MyPrivate() : class_addMethod failed!");
   */
    Class nsApplicationClass = objc_getClass("NSApplication");
    SEL sharedApplicationSelector = sel_registerName("sharedApplication");
    id nsApp = objc_msgSend(nsApplicationClass, sharedApplicationSelector);
    id delegate = objc_msgSend(nsApp, sel_registerName("delegate"));
    id delegateClass = objc_msgSend(delegate, sel_registerName("class"));
    SEL applicationShouldHandleReopenSelector = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
    Method applicationShouldHandleReopen = class_getInstanceMethod(delegateClass, applicationShouldHandleReopenSelector);
    bool didAddMethod = class_addMethod(delegateClass,
                                        applicationShouldHandleReopenSelector,
                                        (IMP)dockClickHandler,
                                        method_getTypeEncoding(applicationShouldHandleReopen));

    if (!didAddMethod)
    {
        method_setImplementation(applicationShouldHandleReopen, (IMP)dockClickHandler);
    }
}

MyPrivate * MyPrivate::instance()
{
    if (!p)
        p = new MyPrivate;
    return p;
}

void MyPrivate::emitClick()
{
    emit dockClicked();
}


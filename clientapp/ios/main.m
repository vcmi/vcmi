/*
 * main.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import "startSDL.h"

#import <UIKit/UIKit.h>

#import <objc/runtime.h>

#include <stdlib.h>

// iOS 27 requires apps built with the iOS 27 SDK to adopt the UIScene life cycle.
// Qt 5.15 creates its window without a window scene and usually does not touch the
// hidden flag, so the window is never attached to the scene. Attach every window
// that is created or shown while a scene exists and remember the rest for the
// scene connection.

static NSMutableArray<UIWindow *> * windowsWithoutScene;

static UIWindowScene * activeWindowScene(void)
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
		if ([scene isKindOfClass:UIWindowScene.class])
			return (UIWindowScene *)scene;

	return nil;
}

static void rememberWindow(UIWindow * window)
{
	if (windowsWithoutScene == nil)
		windowsWithoutScene = [NSMutableArray array];

	if (![windowsWithoutScene containsObject:window])
		[windowsWithoutScene addObject:window];
}

static UIWindow * qtWindow(void)
{
	Class qtViewControllerClass = NSClassFromString(@"QIOSViewController");

	for (UIWindow *window in UIApplication.sharedApplication.windows)
		if (qtViewControllerClass != Nil && [window.rootViewController isKindOfClass:qtViewControllerClass])
			return window;

	for (UIWindow *window in UIApplication.sharedApplication.windows)
		if ([NSStringFromClass(window.class) isEqualToString:@"QUIWindow"])
			return window;

	return nil;
}

static void attachWindowToScene(UIWindow * window, UIWindowScene * windowScene)
{
	[windowsWithoutScene removeObject:window];

	if (window.windowScene != windowScene)
		window.windowScene = windowScene;
}

static void attachOrRememberWindow(UIWindow * window)
{
	if (window.windowScene != nil)
		return;

	UIWindowScene *scene = activeWindowScene();
	if (scene != nil)
		attachWindowToScene(window, scene);
	else
		rememberWindow(window);
}

// Qt lays out against UIScreen.bounds, but on iPhone Duo the app's scene can sit on
// the inner display while Qt picked the outer display as its main screen. Report the
// scene bounds so Qt's geometry matches what is actually visible.
static CGRect (*originalScreenBounds)(UIScreen *, SEL);

static CGRect swizzledScreenBounds(UIScreen * self, SEL _cmd)
{
	UIWindowScene *scene = activeWindowScene();
	if (scene != nil && !CGRectIsEmpty(scene.coordinateSpace.bounds))
		return scene.coordinateSpace.bounds;

	return originalScreenBounds(self, _cmd);
}

static void qtUpdateScreens(void)
{
	// Qt's screen tracker asserts (and dereferences null in release) for screens it
	// does not know about yet, so only poke it once Qt created its windows
	if (windowsWithoutScene.count == 0 && qtWindow() == nil)
		return;

	for (UIScreen *screen in UIScreen.screens)
		[NSNotificationCenter.defaultCenter postNotificationName:UIScreenModeDidChangeNotification object:screen];
}

static void resizeWindowsToScene(UIWindowScene * windowScene)
{
	CGRect sceneBounds = windowScene.coordinateSpace.bounds;

	for (UIWindow *window in UIApplication.sharedApplication.windows)
	{
		if (window.windowScene != windowScene)
			continue;

		if (!CGRectEqualToRect(window.frame, sceneBounds))
			window.frame = sceneBounds;
	}
}

@interface VCMISceneDelegate : UIResponder <UIWindowSceneDelegate>
@end

@implementation VCMISceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
	if (![scene isKindOfClass:UIWindowScene.class])
		return;

	UIWindowScene *windowScene = (UIWindowScene *)scene;

	for (UIWindow *window in [windowsWithoutScene copy])
		attachWindowToScene(window, windowScene);

	for (UIWindow *window in UIApplication.sharedApplication.windows)
		if (window.windowScene == nil)
			attachWindowToScene(window, windowScene);

	resizeWindowsToScene(windowScene);
	qtUpdateScreens();
}

- (void)sceneDidBecomeActive:(UIScene *)scene
{
	if (![scene isKindOfClass:UIWindowScene.class])
		return;

	UIWindowScene *windowScene = (UIWindowScene *)scene;

	for (UIWindow *window in UIApplication.sharedApplication.windows)
		if (window.windowScene == nil)
			attachWindowToScene(window, windowScene);

	resizeWindowsToScene(windowScene);
	qtUpdateScreens();

	UIWindow *mainWindow = qtWindow();
	if (mainWindow != nil)
		[mainWindow makeKeyAndVisible];
}

- (void)windowScene:(UIWindowScene *)windowScene didUpdateEffectiveGeometry:(UIWindowSceneGeometry *)previousGeometry API_AVAILABLE(ios(26.0))
{
	resizeWindowsToScene(windowScene);
	qtUpdateScreens();
}

@end

static id (*originalInitWithFrame)(UIWindow *, SEL, CGRect);

static id swizzledInitWithFrame(UIWindow * self, SEL _cmd, CGRect frame)
{
	self = originalInitWithFrame(self, _cmd, frame);

	// Qt creates QUIWindow after the scene has connected; attach it right away
	attachOrRememberWindow(self);

	return self;
}

static void (*originalSetHidden)(UIView *, SEL, BOOL);

static void swizzledSetHidden(UIView * self, SEL _cmd, BOOL hidden)
{
	if (!hidden && [self isKindOfClass:UIWindow.class])
	{
		UIWindow *window = (UIWindow *)self;
		if (window.windowScene == nil)
			attachOrRememberWindow(window);
	}

	originalSetHidden(self, _cmd, hidden);
}

static void (*originalMakeKeyAndVisible)(UIWindow *, SEL);

static void swizzledMakeKeyAndVisible(UIWindow * self, SEL _cmd)
{
	attachOrRememberWindow(self);
	originalMakeKeyAndVisible(self, _cmd);
}

static void installSceneLifecycleSupport(void)
{
	Class windowClass = UIWindow.class;

	Method initMethod = class_getInstanceMethod(windowClass, @selector(initWithFrame:));
	originalInitWithFrame = (id (*)(UIWindow *, SEL, CGRect))method_getImplementation(initMethod);
	if (!class_addMethod(windowClass, @selector(initWithFrame:), (IMP)swizzledInitWithFrame, method_getTypeEncoding(initMethod)))
		method_setImplementation(initMethod, (IMP)swizzledInitWithFrame);

	Method hiddenMethod = class_getInstanceMethod(windowClass, @selector(setHidden:));
	originalSetHidden = (void (*)(UIView *, SEL, BOOL))method_getImplementation(hiddenMethod);
	if (!class_addMethod(windowClass, @selector(setHidden:), (IMP)swizzledSetHidden, method_getTypeEncoding(hiddenMethod)))
		method_setImplementation(hiddenMethod, (IMP)swizzledSetHidden);

	Method keyMethod = class_getInstanceMethod(windowClass, @selector(makeKeyAndVisible));
	originalMakeKeyAndVisible = (void (*)(UIWindow *, SEL))method_getImplementation(keyMethod);
	if (!class_addMethod(windowClass, @selector(makeKeyAndVisible), (IMP)swizzledMakeKeyAndVisible, method_getTypeEncoding(keyMethod)))
		method_setImplementation(keyMethod, (IMP)swizzledMakeKeyAndVisible);

	Class screenClass = UIScreen.class;
	Method screenBoundsMethod = class_getInstanceMethod(screenClass, @selector(bounds));
	originalScreenBounds = (CGRect (*)(UIScreen *, SEL))method_getImplementation(screenBoundsMethod);
	if (!class_addMethod(screenClass, @selector(bounds), (IMP)swizzledScreenBounds, method_getTypeEncoding(screenBoundsMethod)))
		method_setImplementation(screenBoundsMethod, (IMP)swizzledScreenBounds);
}

@interface QIOSApplicationDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation QIOSApplicationDelegate (VCMISceneLifecycle)

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options
{
	UISceneConfiguration *configuration = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
	configuration.delegateClass = VCMISceneDelegate.class;
	return configuration;
}

@end

static void startSDLManually(int argc, char * argv[])
{
	id<UIApplicationDelegate> appDelegate;
	@autoreleasepool {
		__auto_type sdlAppDelegateClass = NSClassFromString(@"SDLUIKitDelegate");
		NSCAssert(sdlAppDelegateClass != nil, @"SDL AppDelegate class doesn't exist");
		NSCAssert(class_conformsToProtocol(sdlAppDelegateClass, @protocol(UIApplicationDelegate)), @"SDL AppDelegate doesn't conform to UIApplicationDelegate");
		appDelegate = [sdlAppDelegateClass new];
	}
	UIApplication.sharedApplication.delegate = appDelegate;

	int result = startSDL(argc, argv, YES);
	exit(result);
}

int qt_main_wrapper(int argc, char * argv[]);

int client_main(int argc, char * argv[])
{
	installSceneLifecycleSupport();

	NSInteger launchType;
	__auto_type envVar = getenv("VCMI_LAUNCH_TYPE");
	if (envVar)
		launchType = envVar[0] == '0' ? 0 : 1;
	else {
		@autoreleasepool {
			launchType = [NSUserDefaults.standardUserDefaults integerForKey:@"LaunchType"];
		}
	}
	if (launchType == 1)
		return startSDL(argc, argv, NO);

	@autoreleasepool {
		id __block startGameObserver = [NSNotificationCenter.defaultCenter addObserverForName:@"StartGame" object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
			[NSNotificationCenter.defaultCenter removeObserver:startGameObserver];
			startGameObserver = nil;

			NSArray<NSString *> * args = note.userInfo[@"args"];
			const char * newArgv[args.count];
			NSUInteger i = 0;
			for (NSString * obj in args)
				newArgv[i++] = obj.UTF8String;
			startSDLManually(args.count, (char **)(newArgv));
		}];
		return qt_main_wrapper(argc, argv);
	}
}

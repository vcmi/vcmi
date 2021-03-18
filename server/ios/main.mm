/*
 * main_ios.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import <UIKit/UIKit.h>

#include "../Global.h"
#include "CVCMIServer.h"

#define SHARED_DATA_DIR @"GameData"

@interface ViewController : UIViewController
@property (nonatomic, copy) NSURL *sharedPathURL;
@property (nonatomic, copy) NSArray<NSURL *> *dataDirsInDocuments;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    auto startServerButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [startServerButton setTitle:@"Start Server" forState:UIControlStateNormal];
    [startServerButton addTarget:self action:@selector(startServer:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:startServerButton];
    startServerButton.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [startServerButton.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [startServerButton.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    ]];

    auto bundleID = NSBundle.mainBundle.bundleIdentifier;
    auto lastDotPos = [bundleID rangeOfString:@"." options:NSBackwardsSearch].location;
    auto groupID = [NSString stringWithFormat:@"group.%@.vcmi", [bundleID substringToIndex:lastDotPos]];
    auto fm = NSFileManager.defaultManager;
    self.sharedPathURL = [fm containerURLForSecurityApplicationGroupIdentifier:groupID];
    if (!self.sharedPathURL)
    {
        NSLog(@"shared path for group '%@' not available", groupID);
        return;
    }

    auto dirEnumerator = [fm enumeratorAtURL:self.sharedPathURL includingPropertiesForKeys:@[NSURLNameKey] options:NSDirectoryEnumerationSkipsSubdirectoryDescendants errorHandler:nil];
    for (NSURL *fileURL in dirEnumerator)
    {
        NSString *filename;
        if ([fileURL getResourceValue:&filename forKey:NSURLNameKey error:nullptr] && [filename caseInsensitiveCompare:SHARED_DATA_DIR] == NSOrderedSame) {
            NSLog(SHARED_DATA_DIR @" dir already exists in the shared path");
            return;
        }
    }

    auto documentsURL = [fm URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nullptr];
    dirEnumerator = [fm enumeratorAtURL:documentsURL includingPropertiesForKeys:@[NSURLNameKey] options:NSDirectoryEnumerationSkipsSubdirectoryDescendants errorHandler:nil];
    auto dataDirs = [NSMutableArray<NSURL *> arrayWithCapacity:3];
    for (NSURL *fileURL in dirEnumerator)
    {
        NSString *filename;
        if (![fileURL getResourceValue:&filename forKey:NSURLNameKey error:nullptr])
            continue;
        if ([filename caseInsensitiveCompare:@"data"] == NSOrderedSame || [filename caseInsensitiveCompare:@"maps"] == NSOrderedSame || [filename caseInsensitiveCompare:@"mp3"] == NSOrderedSame)
            [dataDirs addObject:fileURL];
    }
    if (dataDirs.count < 3)
    {
        NSLog(@"not all required dirs are present, found only: %@", dataDirs);
        return;
    }
    self.dataDirsInDocuments = dataDirs;

    auto moveDataButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [moveDataButton setTitle:@"Move data to shared dir" forState:UIControlStateNormal];
    [moveDataButton addTarget:self action:@selector(moveDataToSharedDir:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:moveDataButton];

    moveDataButton.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [moveDataButton.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [moveDataButton.topAnchor constraintEqualToAnchor:startServerButton.bottomAnchor constant:10],
    ]];
}

- (void)startServer:(UIButton *)button
{
    button.enabled = NO;
    [NSThread detachNewThreadWithBlock:^{
        NSThread.currentThread.name = @"CVCMIServer";
        CVCMIServer::create();
        dispatch_sync(dispatch_get_main_queue(), ^{
            button.enabled = YES;
        });
    }];
}

- (void)moveDataToSharedDir:(UIButton *)button
{
    button.enabled = NO;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        auto fm = NSFileManager.defaultManager;
        auto destinationURL = [self.sharedPathURL URLByAppendingPathComponent:SHARED_DATA_DIR];
        [fm createDirectoryAtURL:destinationURL withIntermediateDirectories:YES attributes:nil error:nullptr];
        for (NSURL *dirURL in self.dataDirsInDocuments)
            [fm moveItemAtURL:dirURL toURL:[destinationURL URLByAppendingPathComponent:dirURL.lastPathComponent] error:nullptr];

        dispatch_sync(dispatch_get_main_queue(), ^{
            [button removeFromSuperview];
        });
    });
}

@end


@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow *window;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
	self.window.rootViewController = [ViewController new];
	[self.window makeKeyAndVisible];

	return YES;
}

@end


int main(int argc, char * argv[])
{
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}

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
#import <AVFoundation/AVFoundation.h>

//#include "StdInc.h"
#include "../Global.h"
#include "CVCMIServer.h"

@interface ViewController : UIViewController
@end

@implementation ViewController
@end


@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow *window;
@property (nonatomic, strong) AVPlayerLooper *looper;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
	self.window.rootViewController = [ViewController new];
	[self.window makeKeyAndVisible];

    [AVAudioSession.sharedInstance setCategory:AVAudioSessionCategoryPlayback mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionMixWithOthers error:nullptr];

    auto item = [AVPlayerItem playerItemWithURL:[NSBundle.mainBundle URLForResource:@"silence" withExtension:@"wav"]];
    auto player = [AVQueuePlayer new];
    player.allowsExternalPlayback = NO;
    [player play];
    self.looper = [AVPlayerLooper playerLooperWithPlayer:player templateItem:item];

    [NSThread detachNewThreadWithBlock:^
    {
        NSThread.currentThread.name = @"CVCMIServer";
        NSLog(@"starting server from thread %@", NSThread.currentThread);
        CVCMIServer::create();
    }];
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

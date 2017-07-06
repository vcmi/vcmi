#ifdef SPARKLE
#import <Cocoa/Cocoa.h>
#import "Sparkle.h"
#endif

void OSX_checkForUpdates() {
#ifdef SPARKLE
    SUUpdater* updater = [[SUUpdater alloc] init];
    [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
#endif
}

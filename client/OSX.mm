#import <Cocoa/Cocoa.h>
#import "Sparkle.h"

void OSX_checkForUpdates() {
    SUUpdater* updater = [[SUUpdater alloc] init];
    [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
}

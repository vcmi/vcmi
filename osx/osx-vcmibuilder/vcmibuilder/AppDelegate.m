#import "AppDelegate.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
    installationCompleted = NO;
    outputDir = [[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/../../Data"];
    tempDir = NSTemporaryDirectory();
    
    // Output to Application Support
    NSArray* appSupportDirs = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
    outputDir = [[appSupportDirs[0] path] stringByAppendingString:@"/vcmi"];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return YES;
}

- (void)download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse*)response
{
    self->bytesRecieved = 0;
    self->bytesExpected = [response expectedContentLength];
}

- (void)download:(NSURLDownload*)download didReceiveDataOfLength:(NSUInteger)length
{
    self->bytesRecieved += length;
    [self showProgressText:[NSString stringWithFormat:@"Downloading %@ archive: %3.1f Mb / %3.1f Mb", self->currentArchiveName,
                            self->bytesRecieved / 1024.0f / 1024.0f, self->bytesExpected / 1024.0f / 1024.0f]];
}

- (void)download:(NSURLDownload*)download decideDestinationWithSuggestedFilename:(NSString*)filename
{
    [download setDestination:[tempDir stringByAppendingString:currentArchiveFilename] allowOverwrite:YES];
}

- (void)downloadDidFinish:(NSURLDownload*)download
{
    [self showProgressText:[NSString stringWithFormat:@"Downloading %@ archive: completed", self->currentArchiveName]];
    [self nextAction];
}

- (void)download:(NSURLDownload*)download didFailWithError:(NSError*)error
{
    [self showProgressText:[NSString stringWithFormat:@"Downloading %@ archive: failed", self->currentArchiveName]];
    [self showErrorText:[error localizedDescription]];
}

- (void)nextAction
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        if ([actions count] > 0) {
            SEL sel = NSSelectorFromString(actions[0]);
            [actions removeObjectAtIndex:0];
            @try {
                [self performSelector:sel];
            }
            @catch (NSException* e) {
                [self showErrorText:[e name]];
            }
        }
    });
}

- (int)runTask:(NSString*)executable withArgs:(NSArray*)args withWorkingDir:(NSString*)workingDir withPipe:(NSPipe*)pipe
{
    if (![executable hasPrefix:@"/usr/"]) {
        executable = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:executable];
    }
    
    NSTask* task = [[NSTask alloc] init];
    [task setLaunchPath:executable];
    if (workingDir != nil) {
        [task setCurrentDirectoryPath:workingDir];
    }
    if (pipe != nil) {
        [task setStandardOutput:pipe];
    }
    [task setArguments:args];
    
    [task launch];
    [task waitUntilExit];
    
    return [task terminationStatus];
}

- (void)validateAction
{
    // Before starting anything run validations
    if (![[NSFileManager defaultManager] fileExistsAtPath:[self.cd1TextField stringValue]]) {
        return [self showErrorText:@"Please select existing file"];
    }
    
    // Show progress controls
    [self.progressIndicator setHidden:NO];
    [self.progressIndicator startAnimation:self];
    [self showProgressText:@"Installing VCMI..."];
    
    [self nextAction];
}

- (void)downloadWogArchive
{
    // First of all we need to download WoG archive
    // Downloading should be done on main thread because of callbacks
    dispatch_async(dispatch_get_main_queue(), ^{
        self->currentArchiveName = @"WoG";
        self->currentArchiveFilename = @"/wog.zip";
        NSURL* url = [NSURL URLWithString:@"http://download.vcmi.eu/WoG/wog.zip"];
        self.download = [[NSURLDownload alloc] initWithRequest:[NSURLRequest requestWithURL:url] delegate:self];
    });
}

- (void)unzipWogArchive
{
    // Then we unzip downloaded WoG archive
    [self showProgressText:@"Unzipping WoG archive"];
    if ([self runTask:@"/usr/bin/unzip" withArgs:@[@"-qo", [tempDir stringByAppendingString:currentArchiveFilename], @"-d", outputDir] withWorkingDir:nil withPipe:nil] != 0) {
        return [self showErrorText:@"Failed to unzip WoG archive"];
    }
    
    [self nextAction];
}

- (void)downloadVcmiArchive
{
    // Than we need to download VCMI archive
    // Downloading should be done on main thread because of callbacks
    dispatch_async(dispatch_get_main_queue(), ^{
        self->currentArchiveName = @"VCMI";
        self->currentArchiveFilename = @"/core.zip";
        NSURL* url = [NSURL URLWithString:@"http://download.vcmi.eu/core.zip"];
        self.download = [[NSURLDownload alloc] initWithRequest:[NSURLRequest requestWithURL:url] delegate:self];
    });
}

- (void)unzipVcmiArchive
{
    // Then we unzip downloaded VCMI archive
    [self showProgressText:@"Unzipping VCMI archive"];
    if ([self runTask:@"/usr/bin/unzip" withArgs:@[@"-qo", [tempDir stringByAppendingString:currentArchiveFilename], @"-d", outputDir, @"-x", @"*.json", @"*.txt", @"*.PAL"] withWorkingDir:nil withPipe:nil] != 0) {
        return [self showErrorText:@"Failed to unzip VCMI archive"];
    }
    
    [self nextAction];
}

- (void)extractGameData
{
    // Then we extract game data from provided iso files using unshield or from innosetup exe
    if ([[self.cd1TextField stringValue] hasSuffix:@".exe"]) {
        [self innoexctract];
    } else {
        [self unshield];
    }
    
    [self nextAction];
}

- (void)innoexctract
{
    // Extraction via innoextact is pretty straightforward
    [self showProgressText:@"Extracting game data using innoextract..."];
    if ([self runTask:@"/innoextract" withArgs:@[[self.cd1TextField stringValue]] withWorkingDir:tempDir withPipe:nil] != 0) {
        [self showErrorText:@"Failed to exctract game data using innoextract"];
    }
    
    dataDir = [tempDir stringByAppendingString:@"/app"];
}

- (NSString*)attachDiskImage:(NSString*)path
{
    [self showProgressText:[NSString stringWithFormat:@"Mounting image \"%@\"", path]];
    
    // Run hdiutil to mount specified disk image
    NSPipe* pipe = [NSPipe pipe];
    if ([self runTask:@"/usr/bin/hdiutil" withArgs:@[@"attach", path] withWorkingDir:nil withPipe:pipe] != 0) {
        [NSException raise:[NSString stringWithFormat:@"Failed to mount \"%@\"", path] format:nil];
    }
    
    // Capture hdiutil output to get mounted disk image filesystem path
    NSFileHandle* file = [pipe fileHandleForReading];
    NSString* output = [[NSString alloc] initWithData:[file readDataToEndOfFile] encoding:NSUTF8StringEncoding];
    
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:@"(/Volumes/.*)$" options:0 error:nil];
    NSTextCheckingResult* match = [regex firstMatchInString:output options:0 range:NSMakeRange(0, [output length])];
    
    return [output substringWithRange:[match range]];
}

- (void)detachDiskImage:(NSString*)mountedPath
{
    if ([self runTask:@"/usr/bin/hdiutil" withArgs:@[@"detach", mountedPath] withWorkingDir:nil withPipe:nil] != 0) {
        [NSException raise:[NSString stringWithFormat:@"Failed to unmount \"%@\"", mountedPath] format:nil];
    }
}

- (void)unshield
{
    // In case of iso files we should mount them first
    // If CD2 is not specified use the same path as for CD1
    NSString* cd1 = [self attachDiskImage:[self.cd1TextField stringValue]];
    NSString* cd2 = [[self.cd2TextField stringValue] isEqualToString:@""] ? cd1 : [self attachDiskImage:[self.cd2TextField stringValue]];
    
    // Extract
    [self showProgressText:@"Extracting game data using unshield..."];

    NSArray* knownLocations = @[
        @"/_setup/data1.cab",
        @"/Autorun/Setup/data1.cab"
    ];

    bool success = false;
    for (NSString* location in knownLocations) {
        NSString* cabLocation = [cd1 stringByAppendingString:location];
        if ([[NSFileManager defaultManager] fileExistsAtPath:cabLocation]) {
            int result = [self runTask:@"/unshield" withArgs:@[@"-d", tempDir, @"x", cabLocation] withWorkingDir:tempDir  withPipe:nil];
        
            if (result == 0) {
                success = true;
                break;
            }
        }
    }
    
    if (!success) {
        return [self showErrorText:@"Failed to extract game data using unshield"];
    }
    
    NSArray* knownDataDirs = @[
        @"/Heroes3",
        @"/Program_Files",
        @"/Data",
    ];
    
    success = false;
    for (NSString* knownDir in knownDataDirs) {
        dataDir = [tempDir stringByAppendingString:knownDir];
        if ([[NSFileManager defaultManager] fileExistsAtPath:dataDir]) {
            success = true;
            break;
        }
    }
    
    if (!success) {
        return [self showErrorText:@"Failed to extract game data using unshield"];
    }
    
    // Unmount CD1. Unmount CD2 if needed
    [self detachDiskImage:cd1];
    if (![cd1 isEqualToString:cd2]) {
        [self detachDiskImage:cd2];
    }
}

- (void)extractionCompleted
{
    // After game data is extracted we should move it to destination place
    [self showProgressText:@"Moving items into place"];
    
    NSFileManager* fileManager = [NSFileManager defaultManager];
    
    [fileManager moveItemAtPath:[dataDir stringByAppendingString:@"/Data"] toPath:[outputDir stringByAppendingString:@"/Data"] error:nil];
    [fileManager moveItemAtPath:[dataDir stringByAppendingString:@"/Maps"] toPath:[outputDir stringByAppendingString:@"/Maps"] error:nil];
    
    if ([fileManager fileExistsAtPath:[dataDir stringByAppendingString:@"/MP3"] isDirectory:nil]) {
        [fileManager moveItemAtPath:[dataDir stringByAppendingString:@"/MP3"] toPath:[outputDir stringByAppendingString:@"/Mp3"] error:nil];
    } else {
        [fileManager moveItemAtPath:[dataDir stringByAppendingString:@"/Mp3"] toPath:[outputDir stringByAppendingString:@"/Mp3"] error:nil];
    }
    
    // After everythin is complete we create marker file. VCMI will look for this file to exists on startup and
    // will run this setup otherwise
    system([[NSString stringWithFormat:@"touch \"%@/game_data_prepared\"", outputDir] UTF8String]);
    
    [self showProgressText:@"Installation complete"];
    [self.installButton setTitle:@"Run VCMI"];
    [self.progressIndicator stopAnimation:self];
    
    // Notify user that installation completed
    [self showNotification:@"Installation completed"];
    
    // Hide all progress related controls
    [self.progressIndicator setHidden:YES];
    [self.progressIndicator stopAnimation:self];
    [self.progressLabel setHidden:YES];
    
    [self.installButton setEnabled:YES];
    installationCompleted = YES;
}

- (void)selectFile:(NSArray*)fileTypes withTextField:(NSTextField*)textField
{
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:YES];
    [openPanel setAllowedFileTypes:fileTypes];
    [openPanel setAllowsMultipleSelection:NO];
    
    if ([openPanel runModal] == NSOKButton) {
        NSString* path = [[openPanel URL] path];
        [textField setStringValue:path];
    }
}

- (IBAction)selectCD1:(id)sender
{
    [self selectFile:@[@"iso", @"exe"] withTextField:self.cd1TextField];
}

- (IBAction)selectCD2:(id)sender
{
    [self selectFile:@[@"iso"] withTextField:self.cd2TextField];
}

- (IBAction)install:(id)sender
{
    if (installationCompleted) {
        // Run vcmi
        system([[NSString stringWithFormat:@"open %@/../../..", [[NSBundle mainBundle] bundlePath]] UTF8String]);
        [NSApp terminate: nil];
    } else {
        // Run installation
        [self.cd1Button setEnabled:NO];
        [self.cd2Button setEnabled:NO];
        [self.installButton setEnabled:NO];
        
        actions = [NSMutableArray arrayWithObjects:
            @"validateAction",
            @"downloadWogArchive",
            @"unzipWogArchive",
            @"downloadVcmiArchive",
            @"unzipVcmiArchive",
            @"extractGameData",
            @"extractionCompleted",
            nil
        ];
        
        [self nextAction];
    }
}

- (void)showNotification:(NSString*)text
{
    // Notification Center is supported only on OS X 10.8 and newer
    NSUserNotification* notification = [[NSUserNotification alloc] init];
    if (notification != nil) {
        notification.title = @"VCMI";
        notification.informativeText = text;
        notification.deliveryDate = [NSDate dateWithTimeInterval:0 sinceDate:[NSDate date]];
        notification.soundName = NSUserNotificationDefaultSoundName;
        
        [[NSUserNotificationCenter defaultUserNotificationCenter] scheduleNotification:notification];
    } else {
        // On older OS X version force dock icon to jump
        [NSApp requestUserAttention:NSCriticalRequest];
    }
}

- (void)showProgressText:(NSString*)text
{
    // All GUI updates should be done on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.progressLabel setHidden:NO];
        [self.progressLabel setStringValue:text];
    });
}

- (void)showErrorText:(NSString*)text
{
    // All GUI updates should be done on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        [self showNotification:@"Installation failed"];
        
        // Show error alert
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Error"];
        [alert setInformativeText:text];
        [alert beginSheetModalForWindow:self.window modalDelegate:nil didEndSelector:nil contextInfo:nil];

        // Enable select file buttons again
        [self.cd1Button setEnabled:YES];
        [self.cd2Button setEnabled:YES];
        [self.installButton setEnabled:YES];
        
        // Hide all progress related controls
        [self.progressIndicator setHidden:YES];
        [self.progressIndicator stopAnimation:self];
        
        [self.progressLabel setHidden:YES];
    });
}

@end

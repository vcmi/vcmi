#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSURLDownloadDelegate>
{
    NSString* outputDir;
    NSString* tempDir;
    NSString* dataDir;
    NSString* currentArchiveName;
    NSString* currentArchiveFilename;
    
    NSMutableArray* actions;
    
    int64_t bytesRecieved;
    int64_t bytesExpected;
    
    BOOL installationCompleted;
}

@property (strong) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSButton *cd1Button;
@property (weak) IBOutlet NSTextField *cd1TextField;
@property (weak) IBOutlet NSButton *cd2Button;
@property (weak) IBOutlet NSTextField *cd2TextField;
@property (weak) IBOutlet NSProgressIndicator *progressIndicator;
@property (weak) IBOutlet NSTextField *progressLabel;
@property (weak) IBOutlet NSButton *installButton;
@property (weak) IBOutlet NSTextField *errorLabel;

@property (strong) NSURLDownload* download;

- (IBAction)selectCD1:(id)sender;
- (IBAction)selectCD2:(id)sender;
- (IBAction)install:(id)sender;

- (void)selectFile:(NSArray*)fileTypes withTextField:(NSTextField*)textField;
- (void)showProgressText:(NSString*)text;
- (void)showErrorText:(NSString*)text;
- (void)showNotification:(NSString*)text;
- (void)nextAction;
- (int)runTask:(NSString*)executable withArgs:(NSArray*)args withWorkingDir:(NSString*)workingDir withPipe:(NSPipe*)pipe;

- (void)downloadWogArchive;
- (void)unzipWogArchive;
- (void)downloadVcmiArchive;
- (void)unzipVcmiArchive;
- (void)extractGameData;
- (void)innoexctract;
- (NSString*)attachDiskImage:(NSString*)path;
- (void)unshield;
- (void)detachDiskImage:(NSString*)mountedPath;
- (void)extractionCompleted;

@end

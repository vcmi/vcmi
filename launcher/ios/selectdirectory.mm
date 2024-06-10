/*
 * selectdirectory.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "selectdirectory.h"

#include <QEventLoop>

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <MobileCoreServices/MobileCoreServices.h>


@interface ObjcDocumentPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, assign, readonly) QEventLoop & eventLoop;
@property (nonatomic, copy, nullable) NSURL * selectedDirectoryURL;
@end

@implementation ObjcDocumentPickerDelegate
{
	QEventLoop _eventLoop;
}

- (QEventLoop &)eventLoop { return _eventLoop; }

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	self.selectedDirectoryURL = urls.firstObject;
	_eventLoop.exit();
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
	_eventLoop.exit();
}

@end

static ObjcDocumentPickerDelegate * documentPickerDelegate;


SelectDirectory::~SelectDirectory()
{
	[documentPickerDelegate.selectedDirectoryURL stopAccessingSecurityScopedResource];
	documentPickerDelegate = nil;
}

QString SelectDirectory::getExistingDirectory()
{
	documentPickerDelegate = [ObjcDocumentPickerDelegate new];

	UIDocumentPickerViewController * documentPickerVc;
	if(@available(iOS 14.0, *))
		documentPickerVc = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeFolder]];
	else
		documentPickerVc = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[(__bridge NSString *)kUTTypeFolder] inMode:UIDocumentPickerModeOpen];
	documentPickerVc.allowsMultipleSelection = NO;
	documentPickerVc.delegate = documentPickerDelegate;
	[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:documentPickerVc animated:YES completion:nil];

	documentPickerDelegate.eventLoop.exec(QEventLoop::DialogExec);
	if(!documentPickerDelegate.selectedDirectoryURL)
		return {};

	[documentPickerDelegate.selectedDirectoryURL startAccessingSecurityScopedResource];
	return QString::fromNSString(documentPickerDelegate.selectedDirectoryURL.path);
}

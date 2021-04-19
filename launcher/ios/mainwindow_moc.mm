/*
 * mainwindow_moc.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../mainwindow_moc.h"

#import <UIKit/UIKit.h>

void MainWindow::startExecutable(QString /*name*/)
{
    NSString * vcmiScheme = NSBundle.mainBundle.infoDictionary[@"LSApplicationQueriesSchemes"][0];
    [UIApplication.sharedApplication openURL:[NSURL URLWithString:[vcmiScheme stringByAppendingString:@":"]] options:@{} completionHandler:^(BOOL success) {
        if (success)
            return;
        auto alert = [UIAlertController alertControllerWithTitle:@"Can't open VCMI client" message:nil preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        [UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:alert animated:YES completion:nil];
    }];
}

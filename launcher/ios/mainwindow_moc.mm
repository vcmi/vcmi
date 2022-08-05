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

#include <QGuiApplication>
#include <QWindow>

#import <UIKit/UIKit.h>

void MainWindow::startExecutable(QString /*name*/)
{
	qApp->quit();
	[NSNotificationCenter.defaultCenter postNotificationName:@"StartGame" object:nil];
}

void showQtWindow()
{
	auto app = UIApplication.sharedApplication;
	auto qtNativeWindowIndex = [app.windows indexOfObjectPassingTest:^BOOL(__kindof UIWindow * _Nonnull window, NSUInteger idx, BOOL * _Nonnull stop) {
		return [NSStringFromClass([window class]) isEqualToString:@"QUIWindow"];
	}];
	Q_ASSERT(qtNativeWindowIndex != NSNotFound);

	auto qtWindow = qApp->topLevelWindows()[0];
	auto qtWindowNativeView = (__bridge UIView*)reinterpret_cast<void*>(qtWindow->winId());

	auto qtNativeWindow = app.windows[qtNativeWindowIndex];
	[qtNativeWindow.rootViewController.view addSubview:qtWindowNativeView];
	[qtNativeWindow makeKeyAndVisible];
}

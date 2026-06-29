/*
 * imageviewer_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>

class QShortcut;

namespace Ui
{
class ImageViewer;
}

class ImageViewer : public QDialog
{
	Q_OBJECT

	void changeEvent(QEvent *event) override;
	bool event(QEvent * event) override;
public:
	explicit ImageViewer(QWidget * parent = nullptr);
	~ImageViewer();

	void setImages(const QStringList & imagePaths, int startIndex);

	static void showImages(const QStringList & imagePaths, int startIndex, QWidget * parent = nullptr);

protected:
	void keyPressEvent(QKeyEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent * event) override;
	QSize calculateWindowSize();
	void updateResponsiveLayout(const QSize & windowSize);

private:
	void showCurrentImage();
	void showPreviousImage();
	void showNextImage();
	void updateDisplayedPixmap();
	void handleHorizontalSwipe(int deltaX);
	void applyZoomMultiplier(qreal multiplier);
	void panImage(const QPointF & delta);
	void setButtonSize(int size);
	void setButtonStyle(int size);

	Ui::ImageViewer * ui;
	QShortcut * shortcutPrevious = nullptr;
	QShortcut * shortcutNext = nullptr;
	QShortcut * shortcutClose = nullptr;
	QStringList imagePaths;
	QPixmap currentPixmap;
	int currentImageIndex = -1;
	int touchStartPositionX = 0;
	int mouseStartPositionX = 0;
	qreal zoomFactor = 1.0;
	QPointF panOffset;
	QPointF lastDragPosition;
	bool touchSwipeActive = false;
	bool touchPanActive = false;
	bool mousePanActive = false;
	bool touchSwipeSuppressed = false;
	bool desktopWindowInitialized = false;
};

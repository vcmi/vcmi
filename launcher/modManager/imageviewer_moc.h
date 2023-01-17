/*
 * imageviewer_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>

namespace Ui
{
class ImageViewer;
}

class ImageViewer : public QDialog
{
	Q_OBJECT

	void changeEvent(QEvent *event) override;
public:
	explicit ImageViewer(QWidget * parent = 0);
	~ImageViewer();

	void setPixmap(QPixmap & pixmap);

	static void showPixmap(QPixmap & pixmap, QWidget * parent = 0);

protected:
	void mousePressEvent(QMouseEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	QSize calculateWindowSize();

private:
	Ui::ImageViewer * ui;
};

#endif // IMAGEVIEWER_H

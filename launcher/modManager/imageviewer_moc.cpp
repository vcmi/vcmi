/*
 * imageviewer_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <QGuiApplication>

#include "imageviewer_moc.h"
#include "ui_imageviewer_moc.h"

ImageViewer::ImageViewer(QWidget * parent)
	: QDialog(parent), ui(new Ui::ImageViewer)
{
	ui->setupUi(this);
}

void ImageViewer::changeEvent(QEvent *event)
{
	if ( event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QDialog::changeEvent(event);
}

ImageViewer::~ImageViewer()
{
	delete ui;
}

QSize ImageViewer::calculateWindowSize()
{
	return QGuiApplication::primaryScreen()->availableGeometry().size() * 0.8;
}

void ImageViewer::showPixmap(QPixmap & pixmap, QWidget * parent)
{
	assert(!pixmap.isNull());

	ImageViewer * iw = new ImageViewer(parent);

	QSize size = pixmap.size();
	size.scale(iw->calculateWindowSize(), Qt::KeepAspectRatio);
	iw->resize(size);

	iw->setPixmap(pixmap);
	iw->setAttribute(Qt::WA_DeleteOnClose, true);
	iw->setModal(Qt::WindowModal);
	iw->show();
}

void ImageViewer::setPixmap(QPixmap & pixmap)
{
	ui->label->setPixmap(pixmap);
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	close();
}

void ImageViewer::keyPressEvent(QKeyEvent * event)
{
	close(); // FIXME: it also closes on pressing modifiers (e.g. Ctrl/Alt). Not exactly expected
}

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
#include <QGestureEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QScreen>
#include <QShortcut>
#include <QSwipeGesture>
#include <QTouchEvent>

#include "imageviewer_moc.h"
#include "ui_imageviewer_moc.h"

namespace
{
int touchPointsCount(const QTouchEvent * touchEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return static_cast<int>(touchEvent->points().size());
#else
	return touchEvent->touchPoints().size();
#endif
}

int touchEventX(const QTouchEvent * touchEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	if(touchEvent->points().empty())
		return 0;
	return static_cast<int>(touchEvent->points().first().position().x());
#else
	if(touchEvent->touchPoints().empty())
		return 0;
	return static_cast<int>(touchEvent->touchPoints().first().pos().x());
#endif
}

QPointF touchEventPos(const QTouchEvent * touchEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	if(touchEvent->points().empty())
		return QPointF{};
	return touchEvent->points().first().position();
#else
	if(touchEvent->touchPoints().empty())
		return QPointF{};
	return touchEvent->touchPoints().first().pos();
#endif
}

constexpr int sideButtonSize = 48;

int mouseEventX(const QMouseEvent * mouseEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return static_cast<int>(mouseEvent->position().x());
#else
	return static_cast<int>(mouseEvent->localPos().x());
#endif
}
}

ImageViewer::ImageViewer(QWidget * parent)
	: QDialog(parent), ui(new Ui::ImageViewer)
{
	ui->setupUi(this);

	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_AcceptTouchEvents, true);
	ui->imageLabel->setAttribute(Qt::WA_AcceptTouchEvents, true);
	grabGesture(Qt::SwipeGesture);
	grabGesture(Qt::PinchGesture);

	shortcutPrevious = new QShortcut(QKeySequence(Qt::Key_Left), this);
	shortcutNext = new QShortcut(QKeySequence(Qt::Key_Right), this);
	shortcutClose = new QShortcut(QKeySequence(Qt::Key_Escape), this);

	connect(shortcutPrevious, &QShortcut::activated, this, &ImageViewer::showPreviousImage);
	connect(shortcutNext, &QShortcut::activated, this, &ImageViewer::showNextImage);
	connect(shortcutClose, &QShortcut::activated, this, &ImageViewer::close);

	connect(ui->buttonPrevious, &QPushButton::clicked, this, &ImageViewer::showPreviousImage);
	connect(ui->buttonNext, &QPushButton::clicked, this, &ImageViewer::showNextImage);
	connect(ui->buttonClose, &QPushButton::clicked, this, &ImageViewer::close);

	updateResponsiveLayout(size());
}

void ImageViewer::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QDialog::changeEvent(event);
}

bool ImageViewer::event(QEvent * event)
{
	if(event->type() == QEvent::Gesture)
	{
		auto * gestureEvent = static_cast<QGestureEvent *>(event);
		if(auto * pinch = static_cast<QPinchGesture *>(gestureEvent->gesture(Qt::PinchGesture)))
		{
			if(pinch->state() == Qt::GestureStarted || pinch->state() == Qt::GestureUpdated)
			{
				touchSwipeSuppressed = true;
				touchSwipeActive = false;
			}
			if(pinch->state() == Qt::GestureUpdated)
				applyZoomMultiplier(pinch->scaleFactor());
			return true;
		}

		if(auto * swipe = static_cast<QSwipeGesture *>(gestureEvent->gesture(Qt::SwipeGesture)))
		{
			if(swipe->state() == Qt::GestureFinished)
			{
				if(swipe->horizontalDirection() == QSwipeGesture::Left)
					showNextImage();
				else if(swipe->horizontalDirection() == QSwipeGesture::Right)
					showPreviousImage();
			}
			return true;
		}
	}

	if(event->type() == QEvent::TouchBegin)
	{
		auto * touchEvent = static_cast<QTouchEvent *>(event);
		if(touchPointsCount(touchEvent) > 1)
		{
			touchSwipeSuppressed = true;
			touchSwipeActive = false;
			touchPanActive = false;
			return true;
		}

		const auto pos = touchEventPos(touchEvent);
		if(pos.isNull())
			return QDialog::event(event);

		auto * touched = childAt(pos.toPoint());
		if(touched == ui->buttonPrevious || touched == ui->buttonNext || touched == ui->buttonClose)
			return QDialog::event(event);

		lastDragPosition = pos;
		touchStartPositionX = static_cast<int>(pos.x());
		touchPanActive = zoomFactor > 1.0;
		touchSwipeActive = !touchPanActive;
		return true;
	}

	if(event->type() == QEvent::TouchUpdate)
	{
		auto * touchEvent = static_cast<QTouchEvent *>(event);
		if(!touchPanActive)
			return QDialog::event(event);

		const auto pos = touchEventPos(touchEvent);
		if(pos.isNull())
			return true;

		panImage(pos - lastDragPosition);
		lastDragPosition = pos;
		return true;
	}

	if(event->type() == QEvent::TouchEnd)
	{
		auto * touchEvent = static_cast<QTouchEvent *>(event);
		if(touchSwipeSuppressed)
		{
			touchSwipeSuppressed = false;
			touchSwipeActive = false;
			touchPanActive = false;
			return true;
		}

		if(touchPanActive)
		{
			touchPanActive = false;
			return true;
		}

		if(!touchSwipeActive)
			return QDialog::event(event);

		const int touchEndX = touchEventX(touchEvent);
		handleHorizontalSwipe(touchEndX - touchStartPositionX);
		touchSwipeActive = false;
		return true;
	}

	return QDialog::event(event);
}

ImageViewer::~ImageViewer()
{
	delete ui;
}

QSize ImageViewer::calculateWindowSize()
{
#ifdef VCMI_MOBILE
	return QGuiApplication::primaryScreen()->availableGeometry().size();
#else
	return QGuiApplication::primaryScreen()->availableGeometry().size() * 0.8;
#endif
}

void ImageViewer::showImages(const QStringList & imagePaths, int startIndex, QWidget * parent)
{
	if(imagePaths.empty())
		return;

	auto * viewer = new ImageViewer(parent);
	viewer->setImages(imagePaths, startIndex);
	viewer->setAttribute(Qt::WA_DeleteOnClose, true);
	viewer->setModal(Qt::WindowModal);
#ifdef VCMI_MOBILE
	viewer->setGeometry(QGuiApplication::primaryScreen()->availableGeometry());
#endif
	viewer->show();
	viewer->setFocus();
}

void ImageViewer::setImages(const QStringList & imagePaths, int startIndex)
{
	assert(!imagePaths.empty());

	this->imagePaths = imagePaths;
	const int lastImageIndex = static_cast<int>(imagePaths.size() - 1);
	currentImageIndex = std::clamp(startIndex, 0, lastImageIndex);
	showCurrentImage();
}

void ImageViewer::showCurrentImage()
{
	assert(!imagePaths.empty());

	QPixmap pixmap(imagePaths.at(currentImageIndex));
	if(pixmap.isNull())
		return;

	currentPixmap = pixmap;
	zoomFactor = 1.0;
	panOffset = QPointF{};

#ifndef VCMI_MOBILE
	if(!desktopWindowInitialized)
	{
		const QSize availableWindowSize = calculateWindowSize();
		const int desktopMargin = 12;
		const int desktopSpacing = 8;
		const int reservedWidth = (sideButtonSize * 2) + (desktopMargin * 2) + desktopSpacing;
		const int reservedHeight = desktopMargin * 2;
		const int maxLabelWidth = std::max(1, std::min(availableWindowSize.width() - reservedWidth, availableWindowSize.height()));
		const int maxLabelHeight = std::max(1, availableWindowSize.height() - reservedHeight);

		QSize labelTargetSize = currentPixmap.size();
		labelTargetSize.scale(QSize(maxLabelWidth, maxLabelHeight), Qt::KeepAspectRatio);
		resize(labelTargetSize.width() + reservedWidth, labelTargetSize.height() + reservedHeight);
		desktopWindowInitialized = true;
	}
#endif
	updateDisplayedPixmap();

	ui->buttonPrevious->setVisible(imagePaths.size() > 1);
	ui->buttonNext->setVisible(imagePaths.size() > 1);
#ifdef VCMI_MOBILE
	ui->buttonClose->setVisible(true);
#else
	ui->buttonClose->setVisible(false);
#endif
}

void ImageViewer::showPreviousImage()
{
	if(imagePaths.size() <= 1)
		return;

	currentImageIndex = (currentImageIndex - 1 + imagePaths.size()) % imagePaths.size();
	showCurrentImage();
}

void ImageViewer::showNextImage()
{
	if(imagePaths.size() <= 1)
		return;

	currentImageIndex = (currentImageIndex + 1) % imagePaths.size();
	showCurrentImage();
}

void ImageViewer::updateDisplayedPixmap()
{
	if(currentPixmap.isNull())
		return;

	const QSize labelSize = ui->imageLabel->size();
	if(labelSize.isEmpty())
		return;

	QSize targetSize = currentPixmap.size();
	targetSize.scale(labelSize, Qt::KeepAspectRatio);
	targetSize = targetSize * zoomFactor;
	targetSize = targetSize.boundedTo(calculateWindowSize() * 2);

	const auto scaledPixmap = currentPixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	const int maxPanX = std::max(0, (scaledPixmap.width() - labelSize.width()) / 2);
	const int maxPanY = std::max(0, (scaledPixmap.height() - labelSize.height()) / 2);
	panOffset.setX(std::clamp(panOffset.x(), static_cast<qreal>(-maxPanX), static_cast<qreal>(maxPanX)));
	panOffset.setY(std::clamp(panOffset.y(), static_cast<qreal>(-maxPanY), static_cast<qreal>(maxPanY)));

	QPixmap canvas(labelSize);
	canvas.fill(palette().color(QPalette::Window));
	QPainter painter(&canvas);
	const QPoint drawPos((labelSize.width() - scaledPixmap.width()) / 2 + static_cast<int>(panOffset.x()), (labelSize.height() - scaledPixmap.height()) / 2 + static_cast<int>(panOffset.y()));
	painter.drawPixmap(drawPos, scaledPixmap);
	painter.end();
	ui->imageLabel->setPixmap(canvas);
}

void ImageViewer::applyZoomMultiplier(qreal multiplier)
{
	if(multiplier <= 0.0)
		return;

	zoomFactor *= multiplier;

	constexpr qreal minZoomFactor = 1.0;

	zoomFactor = std::clamp(zoomFactor, minZoomFactor, 3.0);
	if(zoomFactor <= 1.0)
		panOffset = QPointF{};
	updateDisplayedPixmap();
}

void ImageViewer::panImage(const QPointF & delta)
{
	if(zoomFactor <= 1.0)
		return;

	panOffset += delta;
	updateDisplayedPixmap();
}

void ImageViewer::handleHorizontalSwipe(int deltaX)
{
	constexpr int swipeThreshold = 40;
	if(deltaX > swipeThreshold)
		showPreviousImage();
	else if(deltaX < -swipeThreshold)
		showNextImage();
}

void ImageViewer::resizeEvent(QResizeEvent * event)
{
	QDialog::resizeEvent(event);
	updateResponsiveLayout(event->size());
	updateDisplayedPixmap();
}

void ImageViewer::setButtonSize(int size)
{
	ui->buttonPrevious->setMinimumSize(size, size);
	ui->buttonPrevious->setMaximumSize(size, size);
	ui->buttonNext->setMinimumSize(size, size);
	ui->buttonNext->setMaximumSize(size, size);
	ui->buttonClose->setMinimumSize(size, size);
	ui->buttonClose->setMaximumSize(size, size);
}

void ImageViewer::setButtonStyle(int size)
{
	const int buttonRadius = size / 2;
	const int buttonFontSize = std::max(14, size / 2);
	const QString buttonStyle = QStringLiteral(
		"QPushButton#buttonPrevious, QPushButton#buttonNext, QPushButton#buttonClose {"
		" border: 1px solid palette(mid);"
		" border-radius: %1px;"
		" padding: 3px;"
		" background: palette(button);"
		" font-size: %2px;"
		" font-weight: 700;"
		" }"
		"QPushButton#buttonPrevious:hover, QPushButton#buttonNext:hover, QPushButton#buttonClose:hover {"
		" background: palette(light);"
		" }").arg(buttonRadius).arg(buttonFontSize);

	ui->buttonPrevious->setStyleSheet(buttonStyle);
	ui->buttonNext->setStyleSheet(buttonStyle);
	ui->buttonClose->setStyleSheet(buttonStyle);
}

void ImageViewer::updateResponsiveLayout(const QSize & windowSize)
{
#ifdef VCMI_MOBILE
	const int shortestSide = std::min(windowSize.width(), windowSize.height());
	const bool compactLayout = shortestSide < 560;
	const int spacing = compactLayout ? 2 : 4;
	const int margin = compactLayout ? 2 : 4;
	const int buttonSize = compactLayout ? 40 : sideButtonSize;
#else
	Q_UNUSED(windowSize);
	const int spacing = 8;
	const int margin = 12;
	const int buttonSize = sideButtonSize;
#endif

	ui->gridLayout->setContentsMargins(margin, margin, margin, margin);
	ui->gridLayout->setHorizontalSpacing(spacing);
	ui->gridLayout->setVerticalSpacing(spacing);

	setButtonSize(buttonSize);
	setButtonStyle(buttonSize);

	ui->gridLayout->setColumnStretch(0, 0);
	ui->gridLayout->setColumnStretch(1, 1);
	ui->gridLayout->setColumnStretch(2, 0);
	ui->gridLayout->setColumnMinimumWidth(0, buttonSize);
	ui->gridLayout->setColumnMinimumWidth(2, buttonSize);
	ui->imageLabel->setMaximumWidth(QWIDGETSIZE_MAX);
	ui->imageLabel->setMinimumWidth(0);
}


void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	if(event->button() == Qt::LeftButton && zoomFactor > 1.0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		lastDragPosition = event->position();
#else
		lastDragPosition = event->localPos();
#endif
		mousePanActive = true;
		event->accept();
		return;
	}

	mouseStartPositionX = mouseEventX(event);
	QDialog::mousePressEvent(event);
}

void ImageViewer::mouseMoveEvent(QMouseEvent * event)
{
	if(!mousePanActive)
	{
		QDialog::mouseMoveEvent(event);
		return;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const QPointF currentPosition = event->position();
#else
	const QPointF currentPosition = event->localPos();
#endif
	panImage(currentPosition - lastDragPosition);
	lastDragPosition = currentPosition;
	event->accept();
}

void ImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
	if(mousePanActive && event->button() == Qt::LeftButton)
	{
		mousePanActive = false;
		event->accept();
		return;
	}

	const int mouseEndX = mouseEventX(event);
	handleHorizontalSwipe(mouseEndX - mouseStartPositionX);
	QDialog::mouseReleaseEvent(event);
}

void ImageViewer::wheelEvent(QWheelEvent * event)
{
	const QPoint numDegrees = event->angleDelta() / 8;
	if(numDegrees.y() == 0)
	{
		QDialog::wheelEvent(event);
		return;
	}

	const qreal zoomStep = numDegrees.y() > 0 ? 1.1 : (1.0 / 1.1);
	applyZoomMultiplier(zoomStep);
	event->accept();
}

void ImageViewer::keyPressEvent(QKeyEvent * event)
{
	switch(event->key())
	{
	case Qt::Key_Left:
		showPreviousImage();
		event->accept();
		break;
	case Qt::Key_Right:
		showNextImage();
		event->accept();
		break;
	case Qt::Key_Escape:
		close();
		event->accept();
		break;
	default:
		QDialog::keyPressEvent(event);
		break;
	}
}

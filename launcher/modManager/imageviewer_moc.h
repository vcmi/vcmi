#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>

namespace Ui {
	class ImageViewer;
}

class ImageViewer : public QDialog
{
	Q_OBJECT
	
public:
	explicit ImageViewer(QWidget *parent = 0);
	~ImageViewer();

	void setPixmap(QPixmap & pixmap);

	static void showPixmap(QPixmap & pixmap, QWidget *parent = 0);
protected:
	void mousePressEvent(QMouseEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	QSize calculateWindowSize();


private:
	Ui::ImageViewer *ui;
};

#endif // IMAGEVIEWER_H

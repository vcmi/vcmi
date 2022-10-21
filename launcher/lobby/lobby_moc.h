#ifndef LOBBY_MOC_H
#define LOBBY_MOC_H

#include <QWidget>
#include <QTcpSocket>
#include <QAbstractSocket>

class SocketTest : public QObject
{
	Q_OBJECT
public:
	explicit SocketTest(QObject *parent = 0);
	void Test();

	void send(const QString &);

signals:

	void text(QString);

public slots:

	void connected();
	void disconnected();
	void bytesWritten(qint64 bytes);
	void readyRead();

private:
	QTcpSocket *socket;
	bool isConnected = false;

};

namespace Ui {
class Lobby;
}

class Lobby : public QWidget
{
	Q_OBJECT

public:
	explicit Lobby(QWidget *parent = nullptr);
	~Lobby();

private slots:
	void on_lineEdit_returnPressed();

	void text(QString);

private:
	Ui::Lobby *ui;
	SocketTest mTest;
};

#endif // LOBBY_MOC_H

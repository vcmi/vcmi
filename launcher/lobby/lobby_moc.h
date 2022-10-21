#ifndef LOBBY_MOC_H
#define LOBBY_MOC_H

#include <QWidget>
#include <QTcpSocket>
#include <QAbstractSocket>

enum ProtocolConsts
{
	GREETING, USERNAME, MESSAGE, VERSION
};

const QMap<ProtocolConsts, QString> ProtocolStrings
{
	{GREETING, "<GREETINGS>%1"},
	{USERNAME, "<USER>%1"},
	{MESSAGE, "<MSG>%1"},
	{VERSION, "<VER>%1"}
};

class SocketLobby : public QObject
{
	Q_OBJECT
public:
	explicit SocketLobby(QObject *parent = 0);
	void connectServer(const QString & host, int port, const QString & username);
	void disconnectServer();

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
	QString username;

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
	void on_messageEdit_returnPressed();

	void text(QString);

	void on_connectButton_toggled(bool checked);

private:
	Ui::Lobby *ui;
	SocketLobby socketLobby;
};

#endif // LOBBY_MOC_H

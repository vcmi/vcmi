#include "lobby_moc.h"
#include "ui_lobby_moc.h"

SocketTest::SocketTest(QObject *parent) :
	QObject(parent)
{
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

void SocketTest::Test()
{
	qDebug() << "Connecting,..";
	emit text("Connecting to 127.0.0.1:5002...");

	socket->connectToHost("127.0.0.1", 5002);

	if(!socket->waitForDisconnected(1000) && !isConnected)
	{
		emit text("Error: " + socket->errorString());
	}

}

void SocketTest::send(const QString & msg)
{
	socket->write(qPrintable(msg));
}

void SocketTest::connected()
{
	isConnected = true;
	emit text("Connected!");
}

void SocketTest::disconnected()
{
	isConnected = false;
	emit text("Disconnected!");
}

void SocketTest::bytesWritten(qint64 bytes)
{
	qDebug() << "We wrote: " << bytes;
}

void SocketTest::readyRead()
{
	qDebug() << "Reading...";
	emit text(socket->readAll());
}



Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);

	connect(&mTest, SIGNAL(text(QString)), this, SLOT(text(QString)));

	mTest.Test();
}

Lobby::~Lobby()
{
	delete ui;
}

void Lobby::on_lineEdit_returnPressed()
{
	mTest.send(ui->lineEdit->text());
	ui->lineEdit->clear();
}

void Lobby::text(QString txt)
{
	QTextCursor curs(ui->chat->document());
	curs.movePosition(QTextCursor::End);
	curs.insertText(txt + "\n");
}


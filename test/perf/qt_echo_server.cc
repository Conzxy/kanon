#include <QSignalMapper>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QObject>

class QTcpServer;

class EchoServer : public QObject
{
    Q_OBJECT
public:
    explicit EchoServer(QObject *parent = 0);

    bool listen(const QHostAddress &address, quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead(QObject *socketObject);
    void onDisconnected(QObject *socketObject);

private:
    QTcpServer *m_server;
    QSignalMapper *m_readyReadSignalMapper;
    QSignalMapper *m_disconnectedSignalMapper;
};

EchoServer::EchoServer(QObject *parent) :
    QObject(parent),
    m_server(new QTcpServer(this)),
    m_readyReadSignalMapper(new QSignalMapper(this)),
    m_disconnectedSignalMapper(new QSignalMapper(this))
{
    connect(m_server, SIGNAL(newConnection()), SLOT(onNewConnection()));
    connect(m_readyReadSignalMapper, SIGNAL(mapped(QObject*)), SLOT(onReadyRead(QObject*)));
    connect(m_disconnectedSignalMapper, SIGNAL(mapped(QObject*)), SLOT(onDisconnected(QObject*)));
}

bool EchoServer::listen(const QHostAddress &address, quint16 port)
{
    if (!m_server->listen(address, port)) {
        // qCritical("Cannot start server: %s", qPrintable(m_server->errorString()));
        return false;
    }

    qDebug("Echo server is listening on %s:%d.", qPrintable(address.toString()), port);

    return true;
}

void EchoServer::onNewConnection()
{
    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    // qDebug("New connection from %s:%d.",
    //        qPrintable(socket->peerAddress().toString()), socket->peerPort());

    connect(socket, SIGNAL(readyRead()), m_readyReadSignalMapper, SLOT(map()));
    m_readyReadSignalMapper->setMapping(socket, socket);

    connect(socket, SIGNAL(disconnected()), m_disconnectedSignalMapper, SLOT(map()));
    m_disconnectedSignalMapper->setMapping(socket, socket);
}

void EchoServer::onReadyRead(QObject *socketObject)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(socketObject);
    if (!socket || !socket->bytesAvailable())
        return;

    QByteArray ba = socket->readAll();
    if (ba.isEmpty())
        return;

    // qDebug("Received data from %s:%d.",
    //        qPrintable(socket->peerAddress().toString()), socket->peerPort());
    socket->write(ba);
}

void EchoServer::onDisconnected(QObject *socketObject)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(socketObject);
    if (!socket)
        return;

    // qDebug("Client %s:%d has disconnected.",
    //        qPrintable(socket->peerAddress().toString()), socket->peerPort());

    socket->deleteLater();
}

#include "qt_echo_server.moc"
#include <QCoreApplication>
#include <QScopedPointer>

int main(int argc, char *argv[])
{
    QScopedPointer<QCoreApplication> app(new QCoreApplication(argc, argv));

    QScopedPointer<EchoServer> server(new EchoServer());

    if (!server->listen(QHostAddress(QStringLiteral("0.0.0.0")), 10000))
        return 1;

    return app->exec();
}

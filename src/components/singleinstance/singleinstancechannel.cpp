#include "singleinstancechannel.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QThread>

#include <utility>

#include <windows.h>

// Role decisions and hand-off results. Warnings and errors are always shown;
// enable the rest with QT_LOGGING_RULES="wynpix.singleinstance.info=true".
Q_LOGGING_CATEGORY(lcSingleInstance, "wynpix.singleinstance", QtWarningMsg)

namespace {
using namespace Qt::StringLiterals;

constexpr QLatin1StringView serverNamePrefix = "wynpix-single-instance-"_L1;
// The "Local\" namespace is per logon session, like the pipe the instances
// of one user session share.
constexpr QLatin1StringView mutexNamePrefix = "Local\\"_L1;

// The primary instance answers every complete message with this byte.
constexpr char ackByte = '\x06';
constexpr qsizetype ackSize = 1;

int remainingMs(const QDeadlineTimer &deadline) {
    return static_cast<int>(deadline.remainingTime());
}
} // namespace

void SingleInstanceChannel::MutexHandleCloser::operator()(void *handle) const noexcept {
    CloseHandle(handle);
}

SingleInstanceChannel::SingleInstanceChannel(QString serverName,
                                             SingleInstanceTimeouts timeouts,
                                             QObject *parent)
    : QObject(parent),
      serverName(std::move(serverName)),
      timeouts(timeouts) {
}

// Pending client sockets are children of the server and go with it. The mutex
// is released here, by the thread that owns it; its handle is closed after.
SingleInstanceChannel::~SingleInstanceChannel() {
    if (ownsMutex && !ReleaseMutex(mutex.get())) {
        qCWarning(lcSingleInstance)
            << "cannot release the instance mutex: error" << GetLastError();
    }
}

QString SingleInstanceChannel::serverNameFor(const QString &tempPath) {
    return serverNamePrefix +
           QString::fromLatin1(
               QCryptographicHash::hash(tempPath.toUtf8(), QCryptographicHash::Md5)
                   .toHex());
}

RoleClaim SingleInstanceChannel::claimRole() {
    if (mutex)
        return ownsMutex ? RoleClaim::Primary : RoleClaim::Secondary;

    const QString mutexName = mutexNamePrefix + serverName;
    mutex.reset(CreateMutexW(nullptr, TRUE,
                             reinterpret_cast<LPCWSTR>(mutexName.utf16())));
    // Read right away: logging below may overwrite the last error.
    const DWORD createError = GetLastError();
    if (!mutex) {
        qCCritical(lcSingleInstance) << "cannot create the instance mutex"
                                     << mutexName << ": error" << createError;
        return RoleClaim::Failed;
    }

    // An existing mutex is not owned by this process even though ownership
    // was requested.
    ownsMutex = createError != ERROR_ALREADY_EXISTS;
    qCInfo(lcSingleInstance) << "role claimed:"
                             << (ownsMutex ? "primary" : "secondary")
                             << "mutex" << mutexName;
    return ownsMutex ? RoleClaim::Primary : RoleClaim::Secondary;
}

bool SingleInstanceChannel::isPrimary() const {
    return ownsMutex;
}

bool SingleInstanceChannel::tryTakeOver() {
    if (ownsMutex)
        return true;

    // The mutex is free only when its owner is gone: a crashed process leaves
    // it abandoned, one that exited normally has closed its handle.
    switch (WaitForSingleObject(mutex.get(), 0)) {
    case WAIT_OBJECT_0:
    case WAIT_ABANDONED:
        ownsMutex = true;
        qCInfo(lcSingleInstance)
            << "the primary instance is gone; taking over the primary role";
        return true;
    case WAIT_TIMEOUT:
        return false;
    default:
        qCWarning(lcSingleInstance)
            << "cannot query the instance mutex: error" << GetLastError();
        return false;
    }
}

ForwardOutcome SingleInstanceChannel::forwardToPrimary(const QString &path) {
    if (!mutex || ownsMutex) {
        qCWarning(lcSingleInstance)
            << "forwardToPrimary() is for secondary instances only";
        return ForwardOutcome::Failed;
    }

    QByteArray message;
    QDataStream out(&message, QIODevice::WriteOnly);
    out << path;

    const QDeadlineTimer deadline(timeouts.forwardDeadlineMs);
    while (true) {
        if (tryTakeOver())
            return ForwardOutcome::BecamePrimary;

        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(remainingMs(deadline)))
            return deliver(socket, message, deadline);

        // The mutex is held, so a primary instance exists; it has just not
        // started listening yet.
        if (deadline.hasExpired()) {
            qCCritical(lcSingleInstance)
                << "the primary instance did not accept a connection within"
                << timeouts.forwardDeadlineMs
                << "ms; last error:" << socket.errorString();
            return ForwardOutcome::Failed;
        }
        QThread::msleep(static_cast<unsigned long>(timeouts.retryIntervalMs));
    }
}

ForwardOutcome SingleInstanceChannel::deliver(QLocalSocket &socket,
                                              const QByteArray &message,
                                              const QDeadlineTimer &deadline) {
    // Lets the primary instance bring its window to the foreground.
    AllowSetForegroundWindow(ASFW_ANY);
    socket.write(message);
    if (!socket.waitForBytesWritten(remainingMs(deadline))) {
        qCCritical(lcSingleInstance)
            << "the path was not delivered to the running instance:"
            << socket.errorString();
        return ForwardOutcome::Failed;
    }

    const bool acknowledged =
        socket.waitForReadyRead(remainingMs(deadline)) &&
        socket.read(ackSize) == QByteArray(ackSize, ackByte);
    socket.disconnectFromServer();
    if (!acknowledged) {
        // The message was written, so it waits in the pipe for the primary
        // instance, which is alive (it owns the mutex).
        qCWarning(lcSingleInstance)
            << "the running instance did not acknowledge the path in time";
        return ForwardOutcome::DeliveredUnacknowledged;
    }
    return ForwardOutcome::Delivered;
}

bool SingleInstanceChannel::listen() {
    if (!ownsMutex) {
        qCCritical(lcSingleInstance)
            << "listen() requires the primary role of this process";
        return false;
    }
    if (server && server->isListening())
        return true;

    server = std::make_unique<QLocalServer>();
    connect(server.get(), &QLocalServer::newConnection, this,
            &SingleInstanceChannel::acceptConnection);
    if (!server->listen(serverName)) {
        qCCritical(lcSingleInstance) << "cannot listen on" << serverName << ":"
                                     << server->errorString();
        server.reset();
        return false;
    }
    qCInfo(lcSingleInstance) << "listening on" << serverName;
    return true;
}

void SingleInstanceChannel::drainPendingPaths() {
    delivering = true;
    const QStringList paths = std::exchange(pendingPaths, QStringList());
    for (const QString &path : paths)
        emit pathReceived(path);
}

void SingleInstanceChannel::acceptConnection() {
    while (QLocalSocket *clientSocket = server->nextPendingConnection()) {
        connect(clientSocket, &QLocalSocket::disconnected, clientSocket,
                &QLocalSocket::deleteLater);
        connect(clientSocket, &QLocalSocket::readyRead, this,
                [this, clientSocket]() { readMessage(clientSocket); });
        // The client may have written before the slot above was connected.
        if (clientSocket->bytesAvailable() > 0)
            readMessage(clientSocket);
    }
}

void SingleInstanceChannel::readMessage(QLocalSocket *clientSocket) {
    QDataStream in(clientSocket);
    in.startTransaction();
    QString path;
    in >> path;
    // Incomplete message: wait for the rest.
    if (!in.commitTransaction())
        return;

    // Acknowledged before delivering: the receiver may take long to react
    // and the secondary instance must not wait for that.
    clientSocket->write(QByteArray(ackSize, ackByte));
    clientSocket->flush();
    receivePath(path);
}

void SingleInstanceChannel::receivePath(const QString &path) {
    if (delivering)
        emit pathReceived(path);
    else
        pendingPaths.append(path);
}

#pragma once

#include <QLocalServer>
#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

class QDeadlineTimer;
class QLocalSocket;

// Timing of the hand-off from a secondary to the primary instance.
struct SingleInstanceTimeouts {
    // One budget for the whole hand-off: connecting (retried while the
    // primary instance is still starting), writing the path and waiting for
    // the acknowledgement.
    int forwardDeadlineMs = 10000;
    // Pause between two connection attempts.
    int retryIntervalMs = 50;
};

// Which role this process plays; decided by claimRole().
enum class RoleClaim {
    Primary,   // this process owns the application
    Secondary, // another process owns it; forward to that one and exit
    Failed,    // the role could not be decided (the reason is logged)
};

// Result of forwardToPrimary().
enum class ForwardOutcome {
    Delivered,               // the primary instance acknowledged the path
    DeliveredUnacknowledged, // written, but no acknowledgement before the
                             // deadline; the path waits in the pipe
    BecamePrimary,           // the primary instance disappeared; this process
                             // owns the application now and must listen()
    Failed,                  // not delivered (the reason is logged)
};

// Local-socket channel between the instances of the application. The role of
// each process is decided by a named mutex (claimRole()), never by whether
// the pipe answers: the pipe appears only when the primary instance has
// started listening and, on Windows, any number of servers can share one
// pipe name. A secondary instance hands its path to the primary with
// forwardToPrimary() and exits; the primary listen()s and receives the paths
// through pathReceived. Both user interfaces use it, so a second launch
// always raises the running one.
//
// The mutex is owned by the thread that claimed the role, so claimRole(),
// forwardToPrimary() and the destruction must happen on one thread (the GUI
// thread in the application).
class SingleInstanceChannel final : public QObject {
    Q_OBJECT
public:
    // serverName identifies the channel (and the mutex); see serverNameFor().
    explicit SingleInstanceChannel(QString serverName,
                                   SingleInstanceTimeouts timeouts = {},
                                   QObject *parent = nullptr);
    ~SingleInstanceChannel() override;

    // Server name shared by the instances that use the same temporary
    // directory (one per user session).
    [[nodiscard]] static QString serverNameFor(const QString &tempPath);

    // Decides the role of this process. Call once, before anything else.
    [[nodiscard]] RoleClaim claimRole();

    [[nodiscard]] bool isPrimary() const;

    // Secondary instances only. Sends path (may be empty: "just raise") to
    // the primary instance, retrying until the deadline while the primary is
    // starting. Never starts a second application instance: when the primary
    // is gone it returns BecamePrimary instead.
    [[nodiscard]] ForwardOutcome forwardToPrimary(const QString &path);

    // Primary instance only: starts accepting the paths of secondary
    // instances. Paths that arrive before drainPendingPaths() are kept.
    // Returns false (and logs the reason) when the server could not listen.
    [[nodiscard]] bool listen();

    // Emits pathReceived for every path received so far, in order, and from
    // then on emits it as soon as a path arrives. Call it once the receiver
    // of pathReceived is ready.
    void drainPendingPaths();

signals:
    // A secondary instance asked to open path (empty: raise the window).
    void pathReceived(const QString &path);

private:
    // Closes the mutex handle (the handle type is hidden from this header).
    struct MutexHandleCloser {
        void operator()(void *handle) const noexcept;
    };

    // True when this process acquires the mutex of a vanished primary.
    bool tryTakeOver();
    ForwardOutcome deliver(QLocalSocket &socket, const QByteArray &message,
                           const QDeadlineTimer &deadline);
    void acceptConnection();
    void readMessage(QLocalSocket *clientSocket);
    void receivePath(const QString &path);

    QString serverName;
    SingleInstanceTimeouts timeouts;
    std::unique_ptr<void, MutexHandleCloser> mutex;
    bool ownsMutex = false;
    bool delivering = false;
    QStringList pendingPaths;
    std::unique_ptr<QLocalServer> server;
};

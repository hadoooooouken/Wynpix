#pragma once

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMap>
#include <QDebug>
#include <QStringList>
#include <QProcess>
#include "utils/script.h"
#include "sourcecontainers/image.h"
class ScriptManager : public QObject {
    Q_OBJECT
public:
    static ScriptManager* getInstance();
    ~ScriptManager();
    void runScript(const QString &scriptName, const QString &filePath);
    bool scriptExists(QString scriptName);
    void readScripts();
    void saveScripts();
    void removeScript(QString scriptName);
    const QMap<QString, Script> &allScripts();
    QList<QString> scriptNames();
    Script getScript(QString scriptName);
    void addScript(QString scriptName, Script script);
    static QStringList splitCommandLine(const QString &cmdLine);

signals:
    void error(QString);

private:
    explicit ScriptManager(QObject *parent = nullptr);
    static constexpr int kBlockingScriptTimeoutMs = 10000;
    QMap<QString, Script> scripts; // <name, script>
    bool blockingScriptActive = false;
    void processArguments(QStringList &cmd, const QString &filePath);

};

extern ScriptManager *scriptManager;

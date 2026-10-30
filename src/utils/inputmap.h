#pragma once

#include <QMap>
#include <QString>

class InputMap {
public:
    InputMap();
    static InputMap *getInstance();
    const QMap<quint32, QString> &keys();
    const QMap<QString, Qt::KeyboardModifier> &modifiers();
    static QString keyNameCtrl();
    static QString keyNameAlt();
    static QString keyNameShift();
    // The name of the numpad key of scanCode by its meaning under the NumLock
    // state that key shows ("Num7" or "NumHome"); empty for a key off the
    // numpad.
    static QString numpadKeyName(quint32 scanCode, Qt::Key key);
    // shortcut of a numpad key with the main keyboard key of the same meaning
    // ("Ctrl+Home" for "Ctrl+NumHome"); empty for any other shortcut.
    static QString mainKeyShortcut(const QString &shortcut);

private:
    void initKeyMap();
    void initModMap();
    QMap<quint32, QString> keyMap;
    QMap<QString, Qt::KeyboardModifier> modMap;
};

extern InputMap *inputMap;

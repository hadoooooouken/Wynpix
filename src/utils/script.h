#pragma once

#include <QDataStream>
#include <QMetaType>
#include <QString>

class Script {
public:
    Script();
    Script(QString _path, bool _blocking);
    QString command;
    bool blocking;
};

// Declared next to the type so that every translation unit instantiating its
// QMetaType sees them; Qt stores the stream operators in the metatype.
QDataStream &operator<<(QDataStream &out, const Script &v);
QDataStream &operator>>(QDataStream &in, Script &v);

Q_DECLARE_METATYPE(Script)

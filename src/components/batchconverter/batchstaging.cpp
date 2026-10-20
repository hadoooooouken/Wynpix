#include "batchstaging.h"

#include <QDir>
#include <QFileInfo>

namespace {
// Two spellings of one file (case, separators, "..") are the same entry.
QString pathKey(const QString &path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();
}
} // namespace

void BatchStaging::append(const QList<QString> &paths, const QString &defaultOutputDirectory) {
    if (mDefaultOutputDirectory.isEmpty())
        mDefaultOutputDirectory = defaultOutputDirectory;
    mEntries.reserve(mEntries.size() + paths.size());
    for (const QString &path : paths) {
        const QString key = pathKey(path);
        if (mKeys.contains(key))
            continue;
        mKeys.insert(key);
        mEntries.append({.path = path, .checked = true});
    }
}

void BatchStaging::replace(const QList<BatchListEntry> &entries) {
    mEntries = entries;
    mKeys.clear();
    for (const BatchListEntry &entry : mEntries)
        mKeys.insert(pathKey(entry.path));
}

void BatchStaging::clear() {
    mEntries.clear();
    mKeys.clear();
    mDefaultOutputDirectory.clear();
}

bool BatchStaging::isEmpty() const {
    return mEntries.isEmpty();
}

const QList<BatchListEntry> &BatchStaging::entries() const {
    return mEntries;
}

const QString &BatchStaging::defaultOutputDirectory() const {
    return mDefaultOutputDirectory;
}

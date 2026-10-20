#pragma once

#include <QList>
#include <QSet>
#include <QString>

#include "batchjob.h"

// The files collected for the batch converter across several visits of its
// dialog, so a batch can be put together from different folders and drives.
// The list lives from the first file added until the dialog closes without
// keeping it (see BatchConversionResult::listKept). Only the data: when the
// dialog opens, what the person sees and how the list is handed over are
// Core's and the dialog port's concern.
//
// A file appears once, however often it is added. GUI thread only.
class BatchStaging {
public:
    // Appends the files not listed yet, selected for the batch. The first
    // non-empty defaultOutputDirectory given since the list was empty is
    // kept as the dialog's default output folder.
    void append(const QList<QString> &paths, const QString &defaultOutputDirectory);
    // Replaces the files with entries the dialog handed back (their
    // selection included); the default output folder stays.
    void replace(const QList<BatchListEntry> &entries);
    // Forgets the files and the default output folder.
    void clear();

    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] const QList<BatchListEntry> &entries() const;
    [[nodiscard]] const QString &defaultOutputDirectory() const;

private:
    QList<BatchListEntry> mEntries;
    // pathKey() of every entry.
    QSet<QString> mKeys;
    QString mDefaultOutputDirectory;
};

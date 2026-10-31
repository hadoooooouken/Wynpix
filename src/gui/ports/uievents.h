#pragma once

#include <QList>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "settings_types.h"
#include "utils/coloradjustments.h"

// Inbound UI port: user intents and UI readiness notifications that a user
// interface reports to the core. The active UI emits these signals (usually
// by forwarding its own); Core and its controllers connect to them. Lives on
// the GUI thread.
class UiEvents final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

signals:
    // Navigation and file activation
    void pathOpened(QString path);
    void droppedIn(QList<QUrl> urls, QObject *source);
    void draggedOut();
    void nextImageRequested();
    void prevImageRequested();
    void pageRequested(int pageIndex);
    // A non-looping animation stopped after its last frame.
    void playbackFinished();
    // An animation stopped on an open or decode error (the UI has already
    // shown the error); it will not report playbackFinished().
    void playbackFailed();

    // File operations
    void copyRequested(QString destDirectory);
    void moveRequested(QString destDirectory);
    void copyUrlsRequested(QList<QString> paths, QString destDirectory);
    void moveUrlsRequested(QList<QString> paths, QString destDirectory);
    void renameRequested(QString newName);
    // The Batch convert button: opens the converter with the files collected
    // so far, or with the selection when none were collected.
    void batchConversionRequested();
    // The "Add to Batch Converter" menu row: adds the selection to the
    // collected files and opens the converter.
    void batchAddRequested();

    // Editing and saving
    void cropRequested(QRect rect);
    void cropAndSaveRequested(QRect rect);
    void colorAdjustmentsApplyRequested(ColorAdjustments adjustments);
    void discardEditsRequested();
    void saveRequested();
    void saveAsRequested();

    // Folder view configuration
    void sortingSelected(SortingMode mode);
    void folderSortingSelected(SortingMode mode);
    void formatFilterSelected(QStringList extensions);
    void nameFilterSelected(QString nameFilter);

    // Viewer
    // The visible area may need an AI upscale at the displayed size (device
    // pixels). The viewer scales on the GPU and needs no CPU-scaled copy.
    void upscaleRequested(QSize size);
    // The view left the upscale range: a pending AI run is no longer wanted.
    void upscaleWithdrawn();

    // Application
    void clearThumbnailCacheRequested();
    void suspendRequested();

    // Readiness (used to reveal the window without flashing)
    void documentRenderingSettled();
    void visibleThumbnailsReady();
    void filesystemViewReady();
};

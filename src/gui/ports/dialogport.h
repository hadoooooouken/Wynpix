#pragma once

#include <QImage>
#include <QList>
#include <QSize>
#include <QString>
#include <memory>
#include <optional>

#include "components/batchconverter/batchjob.h"
#include "settings_types.h"

struct ConfirmationRequest {
    QString title;
    QString message;
};

struct ConfirmationResult {
    bool accepted = false;
};

// Kind of collision a FileReplaceRequest describes.
enum FileReplaceMode {
    FILE_TO_FILE,
    DIR_TO_DIR,
    FILE_TO_DIR,
    DIR_TO_FILE
};

struct FileReplaceRequest {
    QString sourcePath;
    QString targetPath;
    FileReplaceMode mode = FILE_TO_FILE;
    // True when more collisions may follow, so "apply to all" is offered.
    bool multiple = false;
};

// Answer to a FileReplaceRequest. Converts to bool as "replace this item".
struct FileReplaceDecision {
    bool yes = false;
    bool all = false;
    bool cancel = false;

    bool operator==(bool const &cmp) const {
        return yes == cmp;
    }
    operator bool() const {
        return yes;
    }
};

struct SavePathRequest {
    // Pre-filled path; its suffix selects the initial file type filter.
    QString suggestedPath;
};

struct SavePathResult {
    QString path;

    [[nodiscard]] bool accepted() const noexcept { return !path.isEmpty(); }
};

// Parameters chosen in the resize dialog.
struct ResizeRequest {
    QSize size;
    ScalingFilter filter = QI_FILTER_BILINEAR;
    bool useUpscayl = false;
    QString upscaylModel;
};

struct TextInputRequest {
    QString title;
    QString label;
    QString initialText;
};

struct TextInputResult {
    bool accepted = false;
    QString text;
};

struct BatchConversionRequest {
    // The whole list the dialog opens with.
    QList<BatchListEntry> files;
    QString defaultOutputDirectory;
};

struct BatchConversionResult {
    bool conversionStarted = false;
    // "Add more" closed the dialog: `files` is the list to keep and show
    // again when the dialog reopens. False for every other way of closing,
    // which ends the list.
    bool listKept = false;
    QList<BatchListEntry> files;
};

struct PrintRequest {
    std::shared_ptr<const QImage> image;
    // Default target when printing to PDF.
    QString pdfOutputPath;
};

// Outbound UI port for modal dialogs. Every call blocks until the dialog is
// closed and reports the outcome as a value object. Must only be called on
// the GUI thread; worker threads marshal through their owning controller.
class IDialogPort {
public:
    virtual ~IDialogPort() = default;

    virtual ConfirmationResult confirm(const ConfirmationRequest &request) = 0;
    virtual FileReplaceDecision resolveFileReplace(const FileReplaceRequest &request) = 0;
    virtual SavePathResult requestSavePath(const SavePathRequest &request) = 0;
    // std::nullopt when the dialog was cancelled or the size was left unchanged.
    virtual std::optional<ResizeRequest> requestResize(QSize initialSize) = 0;
    virtual TextInputResult requestText(const TextInputRequest &request) = 0;
    virtual BatchConversionResult runBatchConverter(const BatchConversionRequest &request) = 0;
    virtual void print(const PrintRequest &request) = 0;

protected:
    IDialogPort() = default;
    IDialogPort(const IDialogPort &) = default;
    IDialogPort &operator=(const IDialogPort &) = default;
};

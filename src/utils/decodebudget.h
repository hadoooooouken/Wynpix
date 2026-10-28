#pragma once

#include "utils/decodecontext.h"

#include <QSemaphore>
#include <QtGlobal>

#include <optional>

class QImageReader;

// Decoded size from the reader's header, without decoding; nullopt when the
// reader cannot report the dimensions.
[[nodiscard]] std::optional<qint64> estimateDecodedBytes(QImageReader &reader);

// A memory budget sized for 16 GiB of RAM, scaled to the RAM of this machine
// and capped at maxBytes. Without a reported RAM size, the 16 GiB size.
[[nodiscard]] qint64 scaledToInstalledMemory(qint64 bytesAt16GiB, qint64 maxBytes);
[[nodiscard]] qint64 scaledToInstalledMemory(qint64 bytesAt16GiB, qint64 maxBytes,
                                             std::optional<qint64> installedBytes);

// Decoded pixels that concurrent decodes share. A decode reserves its size
// and waits until it fits; one larger than the whole budget waits until the
// budget is free and then runs alone.
class DecodeBudget {
public:
    // Returns its bytes to the budget when destroyed.
    class Reservation {
    public:
        Reservation(Reservation &&other) noexcept;
        Reservation &operator=(Reservation &&other) noexcept;
        Reservation(const Reservation &) = delete;
        Reservation &operator=(const Reservation &) = delete;
        ~Reservation();

    private:
        friend class DecodeBudget;
        Reservation(QSemaphore *units, int count) : units(units), count(count) {}
        void release();
        QSemaphore *units;
        int count;
    };

    explicit DecodeBudget(qint64 capacityBytes);

    // Blocks until the bytes fit; nullopt when the decode is cancelled while
    // it waits.
    [[nodiscard]] std::optional<Reservation> reserve(qint64 bytes,
                                                     const DecodeContext &context);

private:
    // The budget counts whole MiB, so a QSemaphore holds it.
    QSemaphore units;
    int capacityUnits;
};

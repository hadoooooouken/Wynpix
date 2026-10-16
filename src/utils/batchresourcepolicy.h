#pragma once

#include <QThread>
#include <QtGlobal>

#include <algorithm>
#include <atomic>
#include <QVariant>

class BatchResourcePolicy {
public:
    struct Limits {
        int fileConcurrency = 1;
        int scalingThreads = 1;
        int codecThreads = 1;
        int cpuBudget = 1;
    };

    static Limits defaultLimits() {
        return limitsForThreadCount(QThread::idealThreadCount());
    }

    static Limits limitsForThreadCount(int availableThreads) {
        const int available = qMax(1, availableThreads);
        const int cpuBudget = qMax(1, available - 2);
        const int scalingThreads = 1;
        const int fileConcurrency = qMax(1, cpuBudget - scalingThreads);
        return {fileConcurrency, scalingThreads, 1, cpuBudget};
    }

    static void beginBatch() {
        const auto limits = defaultLimits();
        if (g_batchDepth.fetch_add(1, std::memory_order_acq_rel) == 0) {
            g_batchFileConcurrency.store(limits.fileConcurrency, std::memory_order_release);
            g_batchScalingThreads.store(limits.scalingThreads, std::memory_order_release);
        }
    }

    static void endBatch() {
        const int previousDepth = g_batchDepth.fetch_sub(1, std::memory_order_acq_rel);
        if (previousDepth <= 1) {
            g_batchDepth.store(0, std::memory_order_release);
            g_batchFileConcurrency.store(0, std::memory_order_release);
            g_batchScalingThreads.store(0, std::memory_order_release);
        }
    }

    static bool isBatchActive() {
        return g_batchDepth.load(std::memory_order_acquire) > 0;
    }

    class Scope {
    public:
        Scope()
            : m_thread(QThread::currentThread()),
              m_previousValue(m_thread->property(kBatchThreadProperty)) {
            m_thread->setProperty(kBatchThreadProperty, true);
        }

        ~Scope() {
            m_thread->setProperty(kBatchThreadProperty, m_previousValue);
        }

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

    private:
        static constexpr const char *kBatchThreadProperty = "wynpix.batchThread";
        QThread *m_thread;
        QVariant m_previousValue;
    };

    static bool isBatchThread() {
        return QThread::currentThread()->property("wynpix.batchThread").toBool();
    }

    static int fileLimit() {
        const int limit = g_batchFileConcurrency.load(std::memory_order_acquire);
        return limit > 0 ? limit : qMax(1, QThread::idealThreadCount());
    }

    static int scalingLimit() {
        const int limit = g_batchScalingThreads.load(std::memory_order_acquire);
        return limit > 0 ? limit : qMax(1, QThread::idealThreadCount());
    }

    static int codecLimit(int defaultThreads) {
        return qBound(1, std::min(defaultThreads, defaultLimits().codecThreads),
                      defaultThreads);
    }

private:
    static inline std::atomic<int> g_batchDepth{0};
    static inline std::atomic<int> g_batchFileConcurrency{0};
    static inline std::atomic<int> g_batchScalingThreads{0};
};

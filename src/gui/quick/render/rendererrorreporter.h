#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <memory>

#include "gui/quick/render/rendererrorchannel.h"

// Render-thread error log of one ImageRenderer, shared by its RenderDevice
// and ImageLayers: posts each message to the item's RenderErrorChannel, and
// keeps messages reported before the first synchronize() supplied the
// channel. Each distinct message is reported once per shown image, so a
// failure that repeats every frame (or every animation frame) is reported
// once.
//
// Render thread only.
class RenderErrorReporter {
public:
  void report(const QString &message);
  // Called from synchronize(); delivers the messages kept so far.
  void setChannel(std::shared_ptr<RenderErrorChannel> channel);
  // A new image is shown: messages reported so far are reported again when
  // they repeat.
  void clearRepeatGuard();

private:
  std::shared_ptr<RenderErrorChannel> mChannel;
  QStringList mUnsent;
  QSet<QString> mReported;
};

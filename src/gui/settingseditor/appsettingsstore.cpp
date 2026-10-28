#include "appsettingsstore.h"

#include "components/actionmanager/actionmanager.h"
#include "components/cache/thumbnailcache.h"
#include "components/scriptmanager/scriptmanager.h"
#include "settings.h"

//------------------------------------------------------------------------------
AppSettingsStore::AppSettingsStore(Settings &settings, ActionManager &actions,
                                   ScriptManager &scripts, QObject *parent)
    : QObject(parent), mSettings(settings), mMapping(settings), mActions(actions),
      mScripts(scripts) {}

SettingsValues AppSettingsStore::load() {
    return mMapping.load();
}

SettingsEnvironment AppSettingsStore::environment() {
    return {.upscaylModels = mSettings.availableUpscaylModels(),
            .defaultUpscaylModel = Settings::defaultUpscaylModel(),
            .defaultZoomLevels = mSettings.defaultZoomLevels()};
}

void AppSettingsStore::apply(const SettingsValues &values, const ShortcutList &shortcuts) {
    mMapping.store(values);
    storeShortcuts(mActions, shortcuts);
    mScripts.saveScripts();
    mActions.saveShortcuts();
    mSettings.sendChangeNotification();
}

//------------------------------------------------------------------------------
QColor AppSettingsStore::previewThemeMode(int themeMode) {
    mSettings.setThemeMode(static_cast<ThemeMode>(themeMode));
    mSettings.loadTheme();
    mSettings.sendChangeNotification();
    return mSettings.colorScheme().accent;
}

QColor AppSettingsStore::previewBlackBackground(bool useBlackBackground) {
    mSettings.setUseBlackBackground(useBlackBackground);
    mSettings.loadTheme();
    mSettings.sendChangeNotification();
    return mSettings.colorScheme().accent;
}

void AppSettingsStore::previewThumbnailOpacity(qreal opacity) {
    mSettings.setThumbnailOpacity(opacity);
    mSettings.loadTheme();
    mSettings.sendChangeNotification();
}

QColor AppSettingsStore::previewAccent(std::optional<QColor> customAccent) {
    if (customAccent)
        mMapping.storeCustomAccent(*customAccent);
    else
        mSettings.clearCustomAccent();
    mSettings.sendChangeNotification();
    return mSettings.colorScheme().accent;
}

//------------------------------------------------------------------------------
qint64 AppSettingsStore::thumbnailCacheBytes() {
    return ThumbnailCache::currentDiskUsageBytes();
}

void AppSettingsStore::clearThumbnailCache() {
    emit clearThumbnailCacheRequested();
}

//------------------------------------------------------------------------------
AppShortcutStore::AppShortcutStore(ActionManager &actions, ScriptManager &scripts)
    : mActions(actions), mScripts(scripts) {}

ShortcutList AppShortcutStore::shortcuts() {
    ShortcutList list;
    const QMap<QString, QString> &all = mActions.allShortcuts(); // <shortcut, action>
    list.reserve(all.size());
    for (auto it = all.cbegin(); it != all.cend(); ++it)
        list.append({.action = it.value(), .shortcut = it.key()});
    return list;
}

ShortcutList AppShortcutStore::resetShortcuts() {
    mActions.resetDefaults();
    return shortcuts();
}

QStringList AppShortcutStore::actionNames() {
    return mActions.actionList();
}

QStringList AppShortcutStore::scriptNames() {
    return mScripts.scriptNames();
}

bool AppShortcutStore::scriptExists(const QString &name) {
    return mScripts.scriptExists(name);
}

ScriptDefinition AppShortcutStore::script(const QString &name) {
    const Script script = mScripts.getScript(name);
    return {.command = script.command, .blocking = script.blocking};
}

void AppShortcutStore::addScript(const QString &name, const ScriptDefinition &script) {
    mScripts.addScript(name, Script(script.command, script.blocking));
}

ShortcutList AppShortcutStore::removeScript(const QString &name, const ShortcutList &table) {
    storeShortcuts(mActions, table);
    mActions.removeAllShortcuts(kScriptActionPrefix.toString() + name);
    const ShortcutList remaining = shortcuts();
    mScripts.removeScript(name);
    return remaining;
}

//------------------------------------------------------------------------------
void storeShortcuts(ActionManager &actions, const ShortcutList &table) {
    actions.removeAllShortcuts();
    for (const ShortcutEntry &entry : table)
        actions.addShortcut(entry.shortcut, entry.action);
}

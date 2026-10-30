#include "appservices.h"

#include "components/actionmanager/actionmanager.h"
#include "components/scriptmanager/scriptmanager.h"
#include "settings.h"
#include "utils/actions.h"
#include "utils/iconfontmanager.h"
#include "utils/inputmap.h"

//------------------------------------------------------------------------------
AppServices::AppServices() {
    // Must run before the UI is created: the folder tree resolves its icons
    // through IconFontManager::pixmap() and the QML style reads
    // IconFontManager::family(). Safe to call unconditionally: init() is
    // idempotent and a failed font load just means glyph rendering will log
    // a warning and return null pixmaps instead of crashing.
    IconFontManager::init();
    // Each instance is published through its global before it initializes,
    // so later services can already use the earlier ones while they
    // initialize (ScriptManager reads its scripts from settings).
    mInputMap = std::make_unique<InputMap>();
    inputMap = mInputMap.get();
    mActions = std::make_unique<Actions>();
    appActions = mActions.get();
    mSettings = std::make_unique<Settings>();
    settings = mSettings.get();
    mSettings->init();
    mScriptManager = std::make_unique<ScriptManager>();
    scriptManager = mScriptManager.get();
    mScriptManager->readScripts();
    mActionManager = std::make_unique<ActionManager>();
    actionManager = mActionManager.get();
    mActionManager->init();
}
//------------------------------------------------------------------------------
AppServices::~AppServices() {
    // ScriptManager saves its scripts through settings in its destructor, so
    // it goes before Settings. Each global is cleared right after its
    // instance is gone so that no stale pointer stays reachable during the
    // remaining teardown.
    mActionManager.reset();
    actionManager = nullptr;
    mScriptManager.reset();
    scriptManager = nullptr;
    mSettings.reset();
    settings = nullptr;
    mInputMap.reset();
    inputMap = nullptr;
    mActions.reset();
    appActions = nullptr;
}

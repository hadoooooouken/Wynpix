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
    // getInstance() publishes each instance through its global before
    // returning, so later services can already use the earlier ones while
    // they initialize (ScriptManager reads its scripts from settings).
    mInputMap.reset(InputMap::getInstance());
    mActions.reset(Actions::getInstance());
    mSettings.reset(Settings::getInstance());
    mScriptManager.reset(ScriptManager::getInstance());
    mActionManager.reset(ActionManager::getInstance());
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

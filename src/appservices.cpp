#include "appservices.h"

#include "components/actionmanager/actionmanager.h"
#include "components/scriptmanager/scriptmanager.h"
#include "settings.h"
#include "utils/actions.h"
#include "utils/inputmap.h"

//------------------------------------------------------------------------------
AppServices::AppServices(Scope scope) {
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
    if (scope == Scope::SettingsOnly)
        return;
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

#pragma once

#include <memory>

class ActionManager;
class Actions;
class InputMap;
class ScriptManager;
class Settings;

// Owns the process-wide services published through the legacy globals
// (inputMap, appActions, settings, scriptManager, actionManager)
// for the lifetime of one application run. Construction creates them in
// dependency order, publishing each through its global before initializing
// it; destruction releases them in the reverse dependency order and resets
// every global to nullptr. getInstance() only hands out the published
// instance.
class AppServices {
public:
    AppServices();
    ~AppServices();

    AppServices(const AppServices &) = delete;
    AppServices &operator=(const AppServices &) = delete;
    AppServices(AppServices &&) = delete;
    AppServices &operator=(AppServices &&) = delete;

private:
    // Declared in reverse release order so that implicit member destruction
    // matches the explicit order used by the destructor.
    std::unique_ptr<Actions> mActions;
    std::unique_ptr<InputMap> mInputMap;
    std::unique_ptr<Settings> mSettings;
    std::unique_ptr<ScriptManager> mScriptManager;
    std::unique_ptr<ActionManager> mActionManager;
};

// veronicarecomp - ReXGlue Recompiled Project
//
// Code Veronica RexGue Cvars

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <veronicarecomp_settings.h>

REXCVAR_DEFINE_STRING(veronicarecomp_app_version, "", "VeronicaRecomp/Setup", "App Version").lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace CodeVeronica {
    void VeronicaRecompSettings::SetDefaultPaths(rex::PathConfig& paths) {
        // Use default assets directory path if one isn't provided!
        const auto exe_dir = paths.config_path.parent_path();
        if (paths.game_data_root.empty()) {
            paths.game_data_root = exe_dir / "Assets";
        }
        //Keep cache folder in the same folder as EXE
        const auto cur_cache_folder = rex::filesystem::GetUserFolder() / "veronicarecomp" / "cache";
        if (paths.cache_root == cur_cache_folder || paths.cache_root.empty()) {
            paths.cache_root = exe_dir / "Cache";
        }
    }

    //Initializes CVars
    void VeronicaRecompSettings::InitializeSettings(const rex::PathConfig& paths, rex::ui::Window *curWindow) {
        //Initializes required settings, sets window's title.
        if( curWindow ){
            curWindow->SetTitle(std::format("{} - v{}", _title, _version ) ); //Updated Window title
            if ( curWindow && !rex::cvar::HasNonDefaultValue("fullscreen") ){ //Setup fullscreen if it wasn't defined.
                curWindow->SetFullscreen(true);
            }
        }
        //Initializes default vlaues and saves .toml when one isn't found, or if version number differs!
        std::string cfgVersion = rex::cvar::GetFlagByName("veronicarecomp_app_version");
        const bool bVersionChanged = cfgVersion.empty() || cfgVersion != _version;

        //Required settings override values saved by an older version, since those were written to the .toml.
        InitializeSettingsList("required", _requiredSettings, bVersionChanged);

        if(!std::filesystem::is_regular_file(paths.config_path) || bVersionChanged ) { 
            InitializeSettingsList("default", _defaultConfig);
            rex::cvar::SetFlagByName("veronicarecomp_app_version", _version); //Set version
            rex::cvar::SaveConfig(paths.config_path);
        }
    }

    //Initializes CVars based on provided settings.
    void VeronicaRecompSettings::InitializeSettingsList(std::string name, std::vector<ConfigSetting> settings, bool force){
        REXLOG_INFO("Initializing {} settings...", name);
        for (const auto& [k, v] : settings) {
            if ( force || !rex::cvar::HasNonDefaultValue(k) ){
                bool bSuccess = rex::cvar::SetFlagByName(k, v);
                if(bSuccess){
                    REXLOG_INFO("[veronicarecomp_settings] {}: {}", k, v );
                }else{
                    REXLOG_ERROR("[veronicarecomp_settings] Failing to set {}: {}", k, v );
                }
            }
        }
    }
}
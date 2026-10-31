// veronicarecomp - ReXGlue Recompiled Project
//
// veronicarecomp ReXApp

#pragma once

#include <filesystem>

#include <rex/rex_app.h>
#include <rex/ui/keybinds.h>

#include <veronicarecomp_settings.h>
#include "ui/veronica_settings_menu.h"

namespace CodeVeronica {
  class VeronicarecompApp : public rex::ReXApp {
    public:
      using rex::ReXApp::ReXApp;

      static std::unique_ptr<rex::ui::WindowedApp> Create( rex::ui::WindowedAppContext& ctx) {
        return std::unique_ptr<VeronicarecompApp>(new VeronicarecompApp(ctx, "veronicarecomp", PPCImageConfig));
      }

      void OnPreSetup(rex::RuntimeConfig& config) override {
        config.gpu_plugin = "xenos";
      }

      void OnConfigurePaths(rex::PathConfig& paths) override {
        VeronicaRecompSettings::SetDefaultPaths(paths);
        config_path_ = paths.config_path;
      }

      std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults, std::function<void(rex::PathConfig)> resume) override {
        VeronicaRecompSettings::InitializeSettings(defaults, window());
        config_path_ = defaults.config_path;
        (void)resume;
        return defaults;
      }

      void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
        (void)drawer;
        rex::ui::RegisterBind("bind_veronica_menu", "F1", "Toggle Veronica settings", [this]() {
          if (settings_menu_) {
            settings_menu_->RequestClose();
          } else {
            OpenSettingsMenu();
          }
        });
      }

      void OnShutdown() override {
        rex::ui::UnregisterBind("bind_veronica_menu");
        delete settings_menu_;
        settings_menu_ = nullptr;
      }

    private:
      void OpenSettingsMenu() {
        if (settings_menu_ || !imgui_drawer()) return;
        settings_menu_ = new VeronicaSettingsMenu(
            imgui_drawer(), config_path_, [this]() { settings_menu_ = nullptr; });
      }

      std::filesystem::path config_path_;
      VeronicaSettingsMenu* settings_menu_ = nullptr;
    };
}
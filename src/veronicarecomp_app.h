// veronicarecomp - ReXGlue Recompiled Project
//
// veronicarecomp ReXApp

#pragma once

#include <rex/rex_app.h>
#include <veronicarecomp_settings.h>
#include <veronicarecomp_achievement_toast.h>

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
      }

      std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog() override {
        if (!imgui_drawer() || !immediate_drawer() || !runtime()) {
          return nullptr;
        }
        return std::make_unique<VeronicaAchievementToast>(app_context(), imgui_drawer(), immediate_drawer(), runtime());
      }

      std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults, std::function<void(rex::PathConfig)> resume) override {
        VeronicaRecompSettings::InitializeSettings(defaults, window());
        VeronicaRecompSettings::SeedShaderCache(defaults);
        (void)resume;
        return defaults;
      }
    };
}
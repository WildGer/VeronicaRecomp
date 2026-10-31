#pragma once

#include <filesystem>
#include <functional>

#include <rex/ui/imgui_dialog.h>

class VeronicaSettingsMenu final : public rex::ui::ImGuiDialog {
 public:
  VeronicaSettingsMenu(rex::ui::ImGuiDrawer* drawer,
                       std::filesystem::path config_path,
                       std::function<void()> on_closed);
  ~VeronicaSettingsMenu() override = default;

  void RequestClose();

 protected:
  void OnDraw(ImGuiIO& io) override;
  void OnClose() override;

 private:
  bool HasFlag(const char* name) const;
  std::string Get(const char* name) const;
  void Set(const char* name, const std::string& value, bool restart_required = false);
  bool Save();

  void DrawDisplayTab();
  void DrawGraphicsTab();
  void DrawAudioTab();
  void DrawAdvancedTab();
  void DrawAboutTab();

  void DrawBool(const char* cvar, const char* label, const char* help,
                bool restart_required = false);
  void DrawChoice(const char* cvar, const char* label, const char* help,
                  const std::pair<const char*, const char*>* choices,
                  size_t choice_count, bool restart_required = false);
  void DrawInt(const char* cvar, const char* label, const char* help,
               int min_value, int max_value, bool restart_required = false);
  void DrawUnavailable(const char* label);

  std::filesystem::path config_path_;
  std::function<void()> on_closed_;
  bool dirty_ = false;
  bool restart_required_ = false;
  bool save_failed_ = false;
};

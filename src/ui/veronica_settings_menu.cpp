#include "veronica_settings_menu.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <string>
#include <utility>

#include <imgui.h>
#include <rex/cvar.h>

namespace {
constexpr ImVec4 kBrass(0.73f, 0.64f, 0.49f, 1.0f);
constexpr ImVec4 kWarning(0.85f, 0.50f, 0.38f, 1.0f);
constexpr ImVec4 kOk(0.55f, 0.72f, 0.56f, 1.0f);

void Help(const char* text) {
  if (!text || !*text) return;
  ImGui::TextDisabled("%s", text);
}

void Section(const char* title) {
  ImGui::Spacing();
  ImGui::TextColored(kBrass, "%s", title);
  ImGui::Separator();
}
}  // namespace

VeronicaSettingsMenu::VeronicaSettingsMenu(rex::ui::ImGuiDrawer* drawer,
                                           std::filesystem::path config_path,
                                           std::function<void()> on_closed)
    : rex::ui::ImGuiDialog(drawer),
      config_path_(std::move(config_path)),
      on_closed_(std::move(on_closed)) {}

void VeronicaSettingsMenu::RequestClose() {
  if (dirty_ && !Save()) return;
  Close();
}

void VeronicaSettingsMenu::OnClose() {
  if (on_closed_) on_closed_();
}

bool VeronicaSettingsMenu::HasFlag(const char* name) const {
  return rex::cvar::GetFlagInfo(name) != nullptr;
}

std::string VeronicaSettingsMenu::Get(const char* name) const {
  return HasFlag(name) ? rex::cvar::GetFlagByName(name) : std::string();
}

void VeronicaSettingsMenu::Set(const char* name, const std::string& value,
                               bool restart_required) {
  if (!HasFlag(name)) return;
  if (rex::cvar::GetFlagByName(name) == value) return;
  if (rex::cvar::SetFlagByName(name, value)) {
    dirty_ = true;
    restart_required_ |= restart_required;
    save_failed_ = false;
  }
}

bool VeronicaSettingsMenu::Save() {
  try {
    rex::cvar::SaveConfig(config_path_);
    dirty_ = false;
    save_failed_ = false;
    return true;
  } catch (const std::exception&) {
    save_failed_ = true;
    return false;
  } catch (...) {
    save_failed_ = true;
    return false;
  }
}

void VeronicaSettingsMenu::DrawUnavailable(const char* label) {
  ImGui::BeginDisabled();
  ImGui::Text("%s", label);
  ImGui::SameLine();
  ImGui::TextDisabled("Unavailable with this ReXGlue build");
  ImGui::EndDisabled();
}

void VeronicaSettingsMenu::DrawBool(const char* cvar, const char* label,
                                    const char* help, bool restart_required) {
  if (!HasFlag(cvar)) {
    DrawUnavailable(label);
    return;
  }
  bool value = Get(cvar) == "true";
  if (ImGui::Checkbox(label, &value)) Set(cvar, value ? "true" : "false", restart_required);
  Help(help);
}

void VeronicaSettingsMenu::DrawChoice(
    const char* cvar, const char* label, const char* help,
    const std::pair<const char*, const char*>* choices, size_t choice_count,
    bool restart_required) {
  if (!HasFlag(cvar)) {
    DrawUnavailable(label);
    return;
  }

  const std::string current = Get(cvar);
  const char* preview = current.c_str();
  for (size_t i = 0; i < choice_count; ++i) {
    if (current == choices[i].first) {
      preview = choices[i].second;
      break;
    }
  }

  if (ImGui::BeginCombo(label, preview)) {
    for (size_t i = 0; i < choice_count; ++i) {
      const bool selected = current == choices[i].first;
      if (ImGui::Selectable(choices[i].second, selected)) {
        Set(cvar, choices[i].first, restart_required);
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  Help(help);
}

void VeronicaSettingsMenu::DrawInt(const char* cvar, const char* label,
                                   const char* help, int min_value,
                                   int max_value, bool restart_required) {
  if (!HasFlag(cvar)) {
    DrawUnavailable(label);
    return;
  }
  int value = std::atoi(Get(cvar).c_str());
  if (ImGui::SliderInt(label, &value, min_value, max_value)) {
    Set(cvar, std::to_string(value), restart_required);
  }
  Help(help);
}

void VeronicaSettingsMenu::DrawDisplayTab() {
  Section("SCREEN");
  DrawBool("fullscreen", "Fullscreen", "Toggle the ReXGlue host window between fullscreen and windowed mode.");
  DrawBool("vsync", "VSync", "Synchronize presentation to the display refresh rate.");
  DrawBool("d3d12_allow_variable_refresh_rate_and_tearing", "Allow tearing / VRR",
           "Useful with G-Sync or FreeSync when using the D3D12 backend.", true);

  static constexpr std::pair<const char*, const char*> kRefresh[] = {
      {"60", "60 Hz"}, {"60.000000", "60 Hz"}, {"120", "120 Hz"},
      {"120.000000", "120 Hz"}, {"144", "144 Hz"}, {"144.000000", "144 Hz"}};
  DrawChoice("video_mode_refresh_rate", "Reported refresh rate",
             "Code Veronica is designed around 60 Hz. Non-60 values may affect game timing.",
             kRefresh, std::size(kRefresh), true);
}

void VeronicaSettingsMenu::DrawGraphicsTab() {
  Section("RESOLUTION");
  static constexpr std::pair<const char*, const char*> kScale[] = {
      {"1", "1x native"}, {"2", "2x"}, {"3", "3x"}, {"4", "4x"}};
  DrawChoice("resolution_scale", "Resolution scale",
             "Internal rendering resolution multiplier. Restart recommended after changing it.",
             kScale, std::size(kScale), true);

  Section("IMAGE");
  DrawBool("native_2x_msaa", "Native 2x MSAA",
           "Use native multisampling where the Xbox 360 game requests it.", true);

  static constexpr std::pair<const char*, const char*> kAniso[] = {
      {"-1", "Game default"}, {"0", "Off"}, {"2", "2x"}, {"3", "4x"},
      {"4", "8x"}, {"5", "16x"}};
  DrawChoice("anisotropic_override", "Anisotropic filtering",
             "Override texture filtering for sharper surfaces viewed at an angle.",
             kAniso, std::size(kAniso));

  static constexpr std::pair<const char*, const char*> kPresent[] = {
      {"bilinear", "Bilinear"}, {"cas", "AMD CAS"}, {"fsr", "AMD FSR 1"},
      {"fsr_fast", "AMD FSR 1 Fast"}};
  DrawChoice("present_effect", "Presentation filter",
             "Host-side scaling / sharpening filter, when supported by the installed ReXGlue runtime.",
             kPresent, std::size(kPresent), true);
}

void VeronicaSettingsMenu::DrawAudioTab() {
  Section("OUTPUT");
  DrawBool("audio_mute", "Mute", "Mute ReXGlue audio output.");
  DrawInt("audio_maxqframes", "Audio buffer", "Raise this if audio crackles; lower values reduce latency.",
          4, 64, true);
}

void VeronicaSettingsMenu::DrawAdvancedTab() {
  Section("GPU ACCURACY");
  static constexpr std::pair<const char*, const char*> kReadback[] = {
      {"none", "Off"}, {"some", "Occasional"}, {"fast", "Fast"}, {"full", "Full"}};
  DrawChoice("readback_resolve", "Resolve readback",
             "Copies resolved render data back to CPU-visible memory. Veronica defaults to Full.",
             kReadback, std::size(kReadback));
  DrawBool("readback_memexport", "Memexport readback",
           "Keep CPU memory synchronized with GPU memexport writes.");
  DrawBool("clear_memory_page_state", "Clear memory page state",
           "May improve coherency at a performance cost.");
  DrawBool("gpu_allow_invalid_fetch_constants", "Allow invalid fetch constants",
           "Compatibility option. Leave disabled unless a renderer issue specifically requires it.", true);
  DrawBool("snorm16_render_target_full_range", "SNORM16 full range",
           "Compatibility option. Veronica's project defaults this off because enabling it can break lighting.",
           true);

  Section("D3D12");
  static constexpr std::pair<const char*, const char*> kRtPath[] = {
      {"rtv", "RTV"}, {"rov", "ROV"}};
  DrawChoice("render_target_path_d3d12", "Render target path",
             "RTV is the VeronicaRecomp project default. Change only for troubleshooting.",
             kRtPath, std::size(kRtPath), true);
  DrawBool("d3d12_readback_resolve", "D3D12 resolve readback", nullptr);
  DrawBool("d3d12_readback_memexport", "D3D12 memexport readback", nullptr);
}

void VeronicaSettingsMenu::DrawAboutTab() {
  ImGui::TextColored(kBrass, "Resident Evil CODE:Veronica X HD");
  ImGui::Spacing();
  ImGui::TextWrapped("Native PC recompilation using ReXGlue. This F1 settings menu is adapted from the host-side menu architecture used by MKVSDC-Recomp, without its mod loader, telemetry, profiler, or game-specific input layer.");
  ImGui::Spacing();
  ImGui::TextDisabled("F1: open / close settings");
  ImGui::TextDisabled("Esc: close settings");
}

void VeronicaSettingsMenu::OnDraw(ImGuiIO& io) {
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    RequestClose();
    return;
  }

  const float width = std::min(900.0f, io.DisplaySize.x - 60.0f);
  const float height = std::min(680.0f, io.DisplaySize.y - 60.0f);
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                          ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Appearing);

  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  if (!ImGui::Begin("Resident Evil CODE:Veronica - ReXGlue Settings", nullptr, flags)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTabBar("##veronica_settings_tabs")) {
    if (ImGui::BeginTabItem("DISPLAY")) {
      DrawDisplayTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("GRAPHICS")) {
      DrawGraphicsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("AUDIO")) {
      DrawAudioTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("ADVANCED")) {
      DrawAdvancedTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("ABOUT")) {
      DrawAboutTab();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::Separator();
  if (restart_required_) {
    ImGui::TextColored(kWarning, "Some changed settings take effect after restarting the game.");
  } else if (dirty_) {
    ImGui::TextColored(kWarning, "Settings have changed and have not been saved yet.");
  } else if (save_failed_) {
    ImGui::TextColored(kWarning, "Could not save the configuration file.");
  } else {
    ImGui::TextColored(kOk, "Settings are saved.");
  }

  if (ImGui::Button("Save", ImVec2(120, 0))) Save();
  ImGui::SameLine();
  if (ImGui::Button("Close", ImVec2(120, 0))) RequestClose();
  ImGui::SameLine();
  ImGui::TextDisabled("F1 toggles this menu");

  ImGui::End();
}

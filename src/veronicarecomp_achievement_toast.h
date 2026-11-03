// veronicarecomp - ReXGlue Recompiled Project
//
// Achievement toast that only stays registered with the ImGui drawer while a
// toast is on screen. Any registered dialog switches the presenter to painting
// from the UI thread and repainting every frame, so an idle toast would keep
// presenting at the display refresh rate even though the game swaps at 30 Hz.

#pragma once

#include <rex/ui/imgui_drawer.h>
#include <rex/ui/overlay/achievement_toast.h>
#include <rex/ui/windowed_app_context.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

namespace CodeVeronica {
    class VeronicaAchievementToast : public rex::ui::AchievementToastDialog {
        public:
            // Must be created in the UI thread.
            VeronicaAchievementToast(rex::ui::WindowedAppContext& ctx, rex::ui::ImGuiDrawer* drawer,
                                     rex::ui::ImmediateDrawer* immediate_drawer, rex::Runtime* runtime)
                : AchievementToastDialog(drawer, immediate_drawer, runtime), _ctx(ctx) {
                drawer->RemoveDialog(this); // Registered by the base class, stay idle until a toast arrives.
            }

            ~VeronicaAchievementToast() override {
                _alive->store(false);
            }

            void Push(const rex::system::AchievementEvent& event) override {
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    // Toasts are shown one after another, each for kDisplaySeconds (4.5 s).
                    _hideAt = std::max(_hideAt, Clock::now()) + kToastDuration;
                }
                AchievementToastDialog::Push(event);
                _ctx.CallInUIThreadDeferred([this, alive = _alive] {
                    if (alive->load()) {
                        imgui_drawer()->AddDialog(this);
                    }
                });
            }

        protected:
            void OnDraw(ImGuiIO& io) override {
                AchievementToastDialog::OnDraw(io);
                bool bExpired;
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    bExpired = Clock::now() >= _hideAt;
                }
                if (bExpired) {
                    imgui_drawer()->RemoveDialog(this); // Safe while drawing, the drawer defers detaching.
                }
            }

        private:
            using Clock = std::chrono::steady_clock;
            static constexpr auto kToastDuration = std::chrono::milliseconds(5000);

            rex::ui::WindowedAppContext& _ctx;
            std::shared_ptr<std::atomic<bool>> _alive = std::make_shared<std::atomic<bool>>(true);
            std::mutex _mutex;
            Clock::time_point _hideAt{};
    };
}

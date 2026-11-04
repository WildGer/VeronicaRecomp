// veronicarecomp - ReXGlue Recompiled Project
//
// The SDK signs the local profile in with state 1 (signed in locally) and
// denies every privilege, so the game asks to sign in to Xbox Live when
// starting a new game. These override the game's XDK wrappers (the generated
// sub_ functions are weak) to report the profile as signed in to Live and to
// grant privileges. __imp__ is the original guest code.

#include "veronicarecomp_pch.h"

#include <rex/cvar.h>
#include <mutex>
#include <set>
#include <utility>

REXCVAR_DEFINE_BOOL(veronicarecomp_fake_live_signin, true, "VeronicaRecomp/Setup",
                    "Report the profile as signed in to Xbox Live, so the game skips its sign-in prompt");

extern "C" REX_FUNC(__imp__sub_82BEFAD8);
extern "C" REX_FUNC(__imp__sub_82BEFAE8);

namespace {
    constexpr uint32_t kSignedInLocally = 1;
    constexpr uint32_t kSignedInToLive = 2;
    constexpr uint32_t kAnyUser = 0xFF;

    // Logs each guest caller once, to find which game code reacts to the fake state.
    void LogCaller(const char* name, uint32_t caller, uint32_t arg) {
        static std::mutex mutex;
        static std::set<std::pair<uint32_t, uint32_t>> seen;
        std::lock_guard<std::mutex> lock(mutex);
        if (seen.emplace(caller, arg).second) {
            REXLOG_INFO("[veronicarecomp_live] {} from {:08X} ({})", name, caller, arg);
        }
    }
}

// XUserGetSigninState(user_index)
extern "C" REX_FUNC(sub_82BEFAD8) {
    const uint32_t caller = ctx.lr;
    const uint32_t user_index = ctx.r3.u32;
    __imp__sub_82BEFAD8(ctx, base);
    if (REXCVAR_GET(veronicarecomp_fake_live_signin) && ctx.r3.u32 == kSignedInLocally) {
        ctx.r3.u64 = kSignedInToLive;
        LogCaller("XUserGetSigninState", caller, user_index);
    }
}

// XUserCheckPrivilege(user_index, privilege, BOOL* result)
extern "C" REX_FUNC(sub_82BEFAE8) {
    const uint32_t user_index = ctx.r3.u32;
    if (!REXCVAR_GET(veronicarecomp_fake_live_signin) || (user_index != 0 && user_index != kAnyUser)) {
        __imp__sub_82BEFAE8(ctx, base);
        return;
    }
    LogCaller("XUserCheckPrivilege", ctx.lr, ctx.r4.u32);
    REX_STORE_U32(ctx.r5.u32, 1);
    ctx.r3.u64 = 0; // ERROR_SUCCESS
}

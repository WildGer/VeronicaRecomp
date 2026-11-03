// veronicarecomp - ReXGlue Recompiled Project
//
// Backoff for the game's Direct3D wait on free space in the GPU command ring.
// sub_82992B58 loops on the ring read pointer and only runs db16cyc hints
// between checks (sub_8297F940), so the render thread kept a whole core busy
// while the GPU worker waited for the next vblank. The hook spins briefly for
// latency, then sleeps in short steps. The guest's 5 s watchdog is untouched.

#include <chrono>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace {
    using Clock = std::chrono::steady_clock;

    constexpr auto kSpinTime = std::chrono::microseconds(50); // Spin this long before sleeping.
    constexpr auto kSleepTime = std::chrono::microseconds(200);
    constexpr auto kNewStallGap = std::chrono::milliseconds(2); // No call for this long means a new stall.

    void CpuRelax() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
        _mm_pause();
#endif
    }

#if defined(_WIN32)
    // Sleep() rounds up to the system timer period (up to 15.6 ms), so use a high resolution timer.
    struct HighResolutionTimer {
        HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        ~HighResolutionTimer() {
            if (handle) {
                CloseHandle(handle);
            }
        }
    };

    void ShortSleep() {
        static thread_local HighResolutionTimer timer;
        if (!timer.handle) {
            std::this_thread::yield();
            return;
        }
        LARGE_INTEGER due;
        due.QuadPart = -LONGLONG(std::chrono::duration_cast<std::chrono::nanoseconds>(kSleepTime).count() / 100); // Relative, in 100 ns units.
        if (SetWaitableTimer(timer.handle, &due, 0, nullptr, nullptr, FALSE)) {
            WaitForSingleObject(timer.handle, INFINITE);
        } else {
            std::this_thread::yield();
        }
    }
#else
    void ShortSleep() {
        std::this_thread::sleep_for(kSleepTime);
    }
#endif
}

// Mid-asm hook at guest 0x82992BE8, run on every retry of the ring space wait.
void veronicarecomp_command_ring_backoff() {
    static thread_local Clock::time_point stall_start{};
    static thread_local Clock::time_point last_call{};

    const auto now = Clock::now();
    if (now - last_call > kNewStallGap) {
        stall_start = now;
    }
    if (now - stall_start < kSpinTime) {
        CpuRelax();
    } else {
        ShortSleep();
    }
    last_call = Clock::now();
}

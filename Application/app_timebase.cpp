
#include "Application/app_timebase.h"
#include "Application/app_printf.h"
#include "stm32h7xx_hal.h"
#include <cstdio>

#ifndef APP_ENABLE_RTOS_STORE_LOCKS
#define APP_ENABLE_RTOS_STORE_LOCKS 0u
#endif

#if !defined(UNIT_TEST_ENV) && (APP_ENABLE_RTOS_STORE_LOCKS != 0u)
#include "cmsis_os.h"
#else
using osMutexId_t = void *;
constexpr uint32_t osWaitForever = 0xFFFFFFFFu;
inline int32_t osMutexAcquire(osMutexId_t, uint32_t) { return 0; }
inline int32_t osMutexRelease(osMutexId_t) { return 0; }
#endif

#if defined(UNIT_TEST_ENV) || (APP_ENABLE_RTOS_STORE_LOCKS == 0u)
osMutexId_t eventStoreMutexHandle = nullptr;
osMutexId_t navigationDataMutexHandle = nullptr;
#else
extern osMutexId_t eventStoreMutexHandle;
extern osMutexId_t navigationDataMutexHandle;
#endif

namespace {

struct AppTimebaseState {
  bool initialized = false;
  bool use_dwt = false;
  uint32_t last_tick_ms = 0;
  uint32_t last_cycle = 0;
  uint32_t cycle_remainder = 0;
  uint64_t acc_us = 0;
};

// Place in DTCM (non-cacheable, zero wait state) to avoid D-cache coherency
// issues when accessed from both main thread and ISR context.
__attribute__((section(".dtcm_bss"))) AppTimebaseState g_app_timebase;

bool g_app_timebase_initialized = false;

// Diagnostic: capture DWT state at init for later printing
static uint32_t g_timebase_init_hal_ms = 0;
static uint32_t g_timebase_init_dwt_pre = 0;
static uint32_t g_timebase_init_dwt_ctrl_pre = 0;
static uint32_t g_timebase_init_last_cycle = 0;
static uint64_t g_timebase_init_acc_us = 0;

inline void lock_event_store() {
  if (eventStoreMutexHandle != nullptr) {
    osMutexAcquire(eventStoreMutexHandle, osWaitForever);
  }
}

inline void unlock_event_store() {
  if (eventStoreMutexHandle != nullptr) {
    osMutexRelease(eventStoreMutexHandle);
  }
}

inline void lock_navigation_data() {
  if (navigationDataMutexHandle != nullptr) {
    osMutexAcquire(navigationDataMutexHandle, osWaitForever);
  }
}

inline void unlock_navigation_data() {
  if (navigationDataMutexHandle != nullptr) {
    osMutexRelease(navigationDataMutexHandle);
  }
}

#if !defined(UNIT_TEST_ENV)
inline void app_timebase_enable_dwt() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0u;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

inline bool app_timebase_is_dwt_running() {
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0u) {
    return false;
  }
  if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0u) {
    return false;
  }

  const uint32_t a = DWT->CYCCNT;
  __NOP();
  const uint32_t b = DWT->CYCCNT;
  return (a != b);
}
#endif

inline uint32_t app_timebase_enter_critical() {
#if !defined(UNIT_TEST_ENV)
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
#else
  return 0u;
#endif
}

inline void app_timebase_exit_critical(uint32_t primask) {
#if !defined(UNIT_TEST_ENV)
  if ((primask & 0x1u) == 0u) {
    __enable_irq();
  }
#else
  (void)primask;
#endif
}

void app_timebase_init_locked() {
  if (g_app_timebase.initialized) {
    return;
  }

#if !defined(UNIT_TEST_ENV)
  g_timebase_init_hal_ms = HAL_GetTick();
  g_timebase_init_dwt_pre = DWT->CYCCNT;
  g_timebase_init_dwt_ctrl_pre = DWT->CTRL;

  g_app_timebase.last_tick_ms = HAL_GetTick();
  g_app_timebase.acc_us = static_cast<uint64_t>(g_app_timebase.last_tick_ms) * 1000ULL;
  g_app_timebase.last_cycle = 0u;
  g_app_timebase.cycle_remainder = 0u;
#endif

#if !defined(UNIT_TEST_ENV)
  app_timebase_enable_dwt();
  g_app_timebase.use_dwt = app_timebase_is_dwt_running();
  if (g_app_timebase.use_dwt) {
    g_app_timebase.last_cycle = DWT->CYCCNT;
  }
#else
  g_app_timebase.use_dwt = false;
#endif

  g_app_timebase.initialized = true;

  g_timebase_init_last_cycle = g_app_timebase.last_cycle;
  g_timebase_init_acc_us = g_app_timebase.acc_us;
}

// Diagnostic: capture first few calls
static uint32_t g_timebase_first_call_delta = 0xDEAD;
static uint32_t g_timebase_first_call_now_cycle = 0;
static uint32_t g_timebase_first_call_last_cycle = 0;
static uint32_t g_timebase_first_call_count = 0;

uint64_t app_timebase_now_us_locked() {
  app_timebase_init_locked();

  if (g_app_timebase.use_dwt) {
#if !defined(UNIT_TEST_ENV)
    const uint32_t cycles_per_us = SystemCoreClock / 1000000u;
    if (cycles_per_us > 0u) {
      // Hybrid approach: HAL_GetTick (ms, SysTick-based, never wraps for 49 days)
      // + DWT sub-ms interpolation.
      //
      // The previous pure-DWT incremental accumulator suffered from a bug where
      // accumulated µs diverged from real time by ~30x. Root cause unclear
      // (possibly compiler optimization across critical sections, or DWT
      // counting issue after soft reset without power cycle).
      //
      // This approach anchors to SysTick (known correct) and uses DWT only
      // for the fractional millisecond.
      const uint32_t now_ms    = HAL_GetTick();
      const uint32_t now_cycle = DWT->CYCCNT;

      // Capture first call diagnostics
      if (g_timebase_first_call_count == 0) {
        g_timebase_first_call_now_cycle = now_cycle;
        g_timebase_first_call_last_cycle = g_app_timebase.last_cycle;
        g_timebase_first_call_delta = static_cast<uint32_t>(now_cycle - g_app_timebase.last_cycle);
      }
      g_timebase_first_call_count++;

      // Sub-millisecond: cycles elapsed since last SysTick increment.
      // SysTick fires every (SystemCoreClock / 1000) cycles = cycles_per_us * 1000.
      // SysTick->VAL counts DOWN from reload value.  At the instant HAL_GetTick()
      // increments, SysTick->VAL reloads.  The elapsed cycles within the current
      // ms tick = reload - VAL.
      // Guard against SysTick roll-over between reading now_ms and VAL:
      // if VAL rolled (SysTick pending), re-read now_ms.
      uint32_t ms1 = now_ms;
      uint32_t val = SysTick->VAL;
      if (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) {
        // SysTick fired between now_ms read and VAL read
        ms1 = HAL_GetTick();
        val = SysTick->VAL;
      }
      const uint32_t reload = SysTick->LOAD;
      const uint32_t elapsed_in_tick = (val <= reload) ? (reload - val) : 0u;
      const uint32_t sub_ms_us = (elapsed_in_tick / cycles_per_us) % 1000u;

      g_app_timebase.last_cycle = now_cycle;
      const uint64_t candidate = static_cast<uint64_t>(ms1) * 1000ULL + sub_ms_us;
      if (candidate > g_app_timebase.acc_us) {
        g_app_timebase.acc_us = candidate;
      }
      return g_app_timebase.acc_us;
    }
#endif
    g_app_timebase.use_dwt = false;
    g_app_timebase.last_tick_ms = HAL_GetTick();
    g_app_timebase.cycle_remainder = 0u;
  }

  const uint32_t now_ms = HAL_GetTick();
  const uint32_t delta_ms =
      static_cast<uint32_t>(now_ms - g_app_timebase.last_tick_ms);
  g_app_timebase.last_tick_ms = now_ms;
  g_app_timebase.acc_us += static_cast<uint64_t>(delta_ms) * 1000ULL;
  return g_app_timebase.acc_us;
}

} // namespace

extern "C" void app_timebase_init(void) {
  /*if (!g_app_timebase_initialized) {
	  g_app_timebase_initialized = true;
	  g_app_timebase = AppTimebaseState{};
  }

  const uint32_t primask = app_timebase_enter_critical();
  app_timebase_init_locked();
  app_timebase_exit_critical(primask);*/
}

extern "C" uint64_t app_timebase_now_us(void) {
  //const uint32_t primask = app_timebase_enter_critical();
  //const uint64_t now_us = app_timebase_now_us_locked();
  //app_timebase_exit_critical(primask);
  return 1000L * app_timebase_now_ms();
}

extern "C" uint32_t app_timebase_now_ms(void) {
  return HAL_GetTick();
	//return static_cast<uint32_t>(app_timebase_now_us() / 1000ULL);
}

extern "C" void app_timebase_print_init_diag(void) {
#if !defined(UNIT_TEST_ENV)
  app_printf("[TIMEBASE-INIT] pre: HAL=%lu DWT=0x%08lX CTRL=0x%08lX  "
         "post: last_cycle=0x%08lX acc_us=%lu\r\n",
         static_cast<unsigned long>(g_timebase_init_hal_ms),
         static_cast<unsigned long>(g_timebase_init_dwt_pre),
         static_cast<unsigned long>(g_timebase_init_dwt_ctrl_pre),
         static_cast<unsigned long>(g_timebase_init_last_cycle),
         static_cast<unsigned long>(g_timebase_init_acc_us));
  app_printf("[TIMEBASE-FIRST] delta=0x%08lX (%lu us)  now=0x%08lX  last=0x%08lX  calls=%lu\r\n",
         static_cast<unsigned long>(g_timebase_first_call_delta),
         static_cast<unsigned long>(g_timebase_first_call_delta / (SystemCoreClock / 1000000u)),
         static_cast<unsigned long>(g_timebase_first_call_now_cycle),
         static_cast<unsigned long>(g_timebase_first_call_last_cycle),
         static_cast<unsigned long>(g_timebase_first_call_count));
#endif
}

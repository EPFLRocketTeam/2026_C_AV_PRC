#include "Application/FlightControl/prc_shell.hpp"

#include <atomic>

#include "Application/app_printf.h"

namespace {

// -- Ring buffer --------------------------------------------------------------
// Single producer (Prc_Shell_ProcessRxBytes, USB interrupt) and single
// consumer (Prc_Shell_Tick, main loop). Only the producer writes g_head and
// only the consumer writes g_tail; there is no shared length field, so no
// read-modify-write ever races. One slot stays empty to tell full from empty.
static_assert((prc_shell::kRingSize & (prc_shell::kRingSize - 1)) == 0, "ring size must be a power of two");
constexpr uint16_t kRingMask = prc_shell::kRingSize - 1;

uint8_t           g_ring[prc_shell::kRingSize];
volatile uint16_t g_head = 0;
volatile uint16_t g_tail = 0;

// Producer private: a packet was dropped and the consumer has not been told
// yet. Told by writing a NUL marker into the ring ahead of the next packet
// that fits, so the consumer knows exactly where the hole is.
bool g_marker_owed = false;
constexpr uint8_t kGapMarker = 0;

// -- Line assembler (consumer side only) ---------------------------------------
const prc_cli::driver *g_driver = nullptr;
prc_shell::Sink        g_sink   = nullptr;
prc_cli::context       g_ctx;

char     g_line[prc_shell::kLineMax];
uint32_t g_line_len = 0;
// The current line is past kLineMax or crossed a dropped packet: discard
// everything up to the next newline instead of running a damaged command.
bool g_too_long = false;
bool g_damaged  = false;
bool g_usb_log_checked = false;

void Say(const char *text) {
  if (g_sink) g_sink(text);
}

void RunLine() {
  // Trim trailing spaces so "valve vent open " still matches the %bool
  // leaf's exact strcmp against "open" instead of reading as close.
  while (g_line_len > 0 && g_line[g_line_len - 1] == ' ') {
    g_line_len--;
  }
  if (g_line_len == 0 || g_driver == nullptr) return;

  g_ctx.state   = prc_cli::State_0;
  g_ctx.arg_len = 0;
  for (uint32_t i = 0; i < g_line_len; i++) {
    prc_cli::push_char(&g_ctx, g_driver, g_line[i]);
  }
  prc_cli::push_char(&g_ctx, g_driver, '\n');
}

void FeedByte(uint8_t byte) {
  if (byte == kGapMarker) {
    g_damaged = true;
    return;
  }
  // Skip CR so CRLF terminals do not leave a stray \r in bool/float args.
  if (byte == '\r') return;

  if (byte == '\n') {
    if (g_damaged) {
      Say("[SHELL] input overflow, line dropped\r\n");
    } else if (g_too_long) {
      Say("[SHELL] line longer than 128 bytes, dropped\r\n");
    } else {
      RunLine();
    }
    g_line_len = 0;
    g_too_long = false;
    g_damaged  = false;
    return;
  }

  if (g_line_len < prc_shell::kLineMax) {
    g_line[g_line_len++] = static_cast<char>(byte);
  } else {
    g_too_long = true;
  }
}

}  // namespace

void prc_shell::Configure(const prc_cli::driver *drv, Sink sink) {
  g_driver   = drv;
  g_sink     = sink;
  g_ctx      = prc_cli::context();
  g_line_len = 0;
  g_too_long = false;
  g_damaged  = false;
  g_usb_log_checked = false;
}

extern "C" void Prc_Shell_ProcessRxBytes(const uint8_t *data, uint32_t length) {
  if (data == nullptr || length == 0) return;

  const uint16_t head = g_head;
  const uint16_t tail = g_tail;
  const uint32_t free_slots = static_cast<uint16_t>(tail - head - 1) & kRingMask;
  const uint32_t needed = length + (g_marker_owed ? 1u : 0u);
  if (needed > free_slots) {
    g_marker_owed = true;
    return;
  }

  uint16_t h = head;
  if (g_marker_owed) {
    g_ring[h] = kGapMarker;
    h = (h + 1) & kRingMask;
    g_marker_owed = false;
  }
  for (uint32_t i = 0; i < length; i++) {
    g_ring[h] = data[i];
    h = (h + 1) & kRingMask;
  }
  // Publish the bytes before the new head becomes visible to the consumer.
  std::atomic_signal_fence(std::memory_order_release);
  g_head = h;
}

extern "C" void Prc_Shell_Tick(void) {
  if (g_driver == nullptr) Prc_Shell_Init();

  const uint16_t head = g_head;
  std::atomic_signal_fence(std::memory_order_acquire);
  uint16_t tail = g_tail;
  if (tail == head) return;

  // Bytes arrived, so the host has configured the CDC interface and is
  // talking to us: from here CDC_Transmit_HS is safe to call. Turn the USB
  // log on once so the shell replies are visible without "logs usb on"
  // first, unless something already turned it on. Doing this at boot
  // instead would send every app_printf into CDC_Transmit_HS on a board
  // with no USB host, where pClassData is still NULL and CDC_Transmit_HS
  // reads hcdc->TxState through it without a check (usbd_cdc_if.c).
  if (!g_usb_log_checked) {
    g_usb_log_checked = true;
    if (!app_printf_is_usb_enabled()) app_printf_use_usb(true);
  }

  while (tail != head) {
    const uint8_t byte = g_ring[tail];
    tail = (tail + 1) & kRingMask;
    FeedByte(byte);
  }
  std::atomic_signal_fence(std::memory_order_release);
  g_tail = tail;
}

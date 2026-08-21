#pragma once
#include <cstdint>

/* SD card DMA timing statistics (populated by plume_driver, read from main) */
struct SdTimingStats {
    volatile uint64_t dma_start_us;
    volatile uint64_t dma_cb_us;
    uint32_t last_batch_size;
    uint32_t dma_count;
    uint32_t dma_error_count;
    uint32_t max_xfer_us;      /* max DMA transfer time (start→callback) */
    uint32_t max_prog_us;      /* max card programming (callback→ready)  */
    uint32_t max_cycle_us;     /* max full cycle (start→ready)           */
    uint64_t sum_cycle_us;
    uint32_t total_blocks;
    uint32_t min_batch;
    uint32_t max_batch;
    uint32_t last_error_code;  /* hsd->ErrorCode from last error callback */
};
static_assert(sizeof(SdTimingStats) == 64);

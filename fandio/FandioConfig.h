#pragma once
#include <cstdint>

namespace fandio {

// Compile-time configuration flags (replaces Core/switches.h)
// These can be overridden by defining them before including this header

#ifndef FANDIO_NETWORK_ENABLED
#define FANDIO_NETWORK_ENABLED true
#endif

#ifndef FANDIO_DEMO_MODE
#define FANDIO_DEMO_MODE false
#endif

#ifndef FANDIO_DEMO_MAX_RECORDS
#define FANDIO_DEMO_MAX_RECORDS 100
#endif

// Runtime configuration for fandio library
struct FandioConfig {
    // Network mode enabled (replaces FandNetV)
    static constexpr bool NetworkEnabled = FANDIO_NETWORK_ENABLED;

    // Demo mode limitations (replaces FandDemo)
    static constexpr bool DemoMode = FANDIO_DEMO_MODE;
    static constexpr int DemoMaxRecords = FANDIO_DEMO_MAX_RECORDS;

    // Lock retry settings
    uint16_t lock_retries = 10;
    uint16_t lock_delay_ms = 100;
    uint16_t net_delay_ms = 1000;

    // LAN node identifier (replaces LANNode from CommonVariables)
    uint16_t lan_node = 0;

    // Cache settings
    bool enable_cache = true;
    size_t cache_size = 64 * 1024; // 64KB default

    // Default configuration instance
    static FandioConfig& GetDefault() {
        static FandioConfig instance;
        return instance;
    }
};

// Backward compatibility macros for existing code
// These can be used during transition period
#if FANDIO_NETWORK_ENABLED
#define FandNetV 1
#endif

#if FANDIO_DEMO_MODE
#define FandDemo 5
#endif

} // namespace fandio

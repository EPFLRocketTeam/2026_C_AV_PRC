
#include "Modules/Sensors/drivers/SensataSensor.hpp"

namespace sensata {
    mux::Pca9547Mux globalMux(&hi2c1, GLOBAL_MUX_ADDRESS, GLOBAL_I2C_TIMEOUT_MS);
};

const char* sensata::internal::poll_mode_str (sensata::SensataPollMode mode) {
    switch (mode) {
        case sensata::PRESSURE: return "Pressure";
        case sensata::TEMPERATURE: return "Temperature";
    }

    return "Unknown";
}


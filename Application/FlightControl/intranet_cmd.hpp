// Shared intranet-command IDs consumed by PrcState::fromXxx() (prc_state.cpp)
// and produced by the CAN RX decode layer (prc_can.cpp) -- pulled out to its
// own header so both can reference the same source of truth instead of
// prc_state.cpp's transition logic owning a private copy.

#ifndef APP_PRC_INTRANET_CMD_H
#define APP_PRC_INTRANET_CMD_H

#include <cstdint>

namespace prc {

} // namespace prc

#endif // APP_PRC_INTRANET_CMD_H

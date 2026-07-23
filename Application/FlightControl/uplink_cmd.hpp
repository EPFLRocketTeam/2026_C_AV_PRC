// Shared uplink-command IDs consumed by PrcState::fromXxx() (prc_state.cpp)
// and produced by the CAN RX decode layer (prc_can.cpp) -- pulled out to its
// own header so both can reference the same source of truth instead of
// prc_state.cpp's transition logic owning a private copy.

#ifndef APP_PRC_UPLINK_CMD_H
#define APP_PRC_UPLINK_CMD_H

#include <cstdint>

namespace prc {

enum UplinkCmdId : uint8_t {
  kCmdNone          = 0,
  kCmdPressurizeOn  = 1,
  kCmdPressurizeOff = 2,
  kCmdPassivate     = 3,
  kCmdAbort         = 4,
  kCmdReset         = 5,
};

} // namespace prc

#endif // APP_PRC_UPLINK_CMD_H

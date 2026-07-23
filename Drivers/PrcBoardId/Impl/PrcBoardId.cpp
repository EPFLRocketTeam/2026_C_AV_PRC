#include "../PrcBoardId.hpp"

#include "main.h" // ENG_SETUP_Pin/ETH_SETUP_Pin/LOX_SETUP_Pin + their GPIO ports

using namespace prc;

BoardRole Prc_DetectBoardRole()
{
    const bool eng = HAL_GPIO_ReadPin(ENG_SETUP_GPIO_Port, ENG_SETUP_Pin) == GPIO_PIN_SET;
    const bool eth = HAL_GPIO_ReadPin(ETH_SETUP_GPIO_Port, ETH_SETUP_Pin) == GPIO_PIN_SET;
    const bool lox = HAL_GPIO_ReadPin(LOX_SETUP_GPIO_Port, LOX_SETUP_Pin) == GPIO_PIN_SET;

    // Exactly one strap must read high -- none (open strap) or more than
    // one (wiring fault) is ambiguous and left as Unknown rather than
    // guessed, since a wrong LOX/Eth identification has real consequences.
    const uint8_t high_count = (eng ? 1 : 0) + (eth ? 1 : 0) + (lox ? 1 : 0);
    if (high_count != 1) {
        return BoardRole::Unknown;
    }

    if (eng) return BoardRole::EngineBay;
    if (eth) return BoardRole::DprEth;
    return BoardRole::DprLox;
}

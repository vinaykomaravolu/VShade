#pragma once

#include <cstdint>

namespace vshade::platform {

/** @brief Engine-independent mouse button identifiers. */
enum class MouseButton : std::uint8_t {
    Button1 = 0,
    Button2,
    Button3,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,

    /** @brief Internal number of mouse buttons; not a physical button. */
    Count,

    /** @brief Alias for the primary mouse button. */
    Left = Button1,
    /** @brief Alias for the secondary mouse button. */
    Right = Button2,
    /** @brief Alias for the middle mouse button. */
    Middle = Button3
};

} // namespace vshade::platform

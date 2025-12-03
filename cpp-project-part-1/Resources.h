#pragma once
#include <vector>
#include <string>

namespace Resources {
    // Returns cached lines of riddle screen template (UTF-8 BOM removed if present).
    const std::vector<std::string>& getRiddleTemplate();

    // Returns cached lines of pause screen template (UTF-8 BOM removed if present).
    const std::vector<std::string>& getPauseTemplate();
}

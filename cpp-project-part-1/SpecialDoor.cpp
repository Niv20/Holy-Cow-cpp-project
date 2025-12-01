#include "SpecialDoor.h"
#include "Game.h"
#include "Switch.h"
#include <algorithm>

bool SpecialDoor::areConditionsMet(Game& game) {
    if (isOpen) return true;
    if (keysInserted.size() != requiredKeys.size()) return false;
    for (const auto& rk : requiredKeys) {
        auto it = std::find_if(keysInserted.begin(), keysInserted.end(), [&](const Key& k){ return k.equals(rk); });
        if (it == keysInserted.end()) return false;
    }
    for (const auto& req : requiredSwitches) {
        SwitchData* sw = game.findSwitchAt(roomIdx, req.pos);
        if (!sw) continue;
        if (sw->isOn != req.requiredState) return false;
    }
    isOpen = true; return true;
}

bool SpecialDoor::useKey(const Key& key) {
    if (isOpen || !key.valid()) return false;
    auto needed = std::find_if(requiredKeys.begin(), requiredKeys.end(), [&](const Key& k){ return k.equals(key); });
    if (needed == requiredKeys.end()) return false;
    auto already = std::find_if(keysInserted.begin(), keysInserted.end(), [&](const Key& k){ return k.equals(key); });
    if (already != keysInserted.end()) return false;
    keysInserted.push_back(key); return true;
}

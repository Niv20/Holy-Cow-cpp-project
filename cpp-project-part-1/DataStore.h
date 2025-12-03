#pragma once
#include <vector>
#include <string>
#include "Screen.h"

// Centralized data access layer. Currently falls back to existing text loaders.
namespace DataStore {
    // Load all world screens from data storage.
    std::vector<Screen> loadWorldScreens();

    // Load a generic UI screen by name (e.g., Start, Instructions, Lose).
    std::vector<std::string> loadUiScreen(const std::string& name);

    // Load special doors configuration as a single text blob (temporary until binary format is defined).
    std::string loadSpecialDoorsConfig();
}

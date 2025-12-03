#include "Resources.h"
#include "DataStore.h"
#include <fstream>
#include <string>

namespace {
    std::vector<std::string> g_riddleTemplate;
    std::vector<std::string> g_pauseTemplate;

    void ensureLoadedFromStore(std::vector<std::string>& cache, const char* name) {
        if (!cache.empty()) return;
        cache = DataStore::loadUiScreen(name);
    }
}

const std::vector<std::string>& Resources::getRiddleTemplate() {
    ensureLoadedFromStore(g_riddleTemplate, "riddle");
    return g_riddleTemplate;
}

const std::vector<std::string>& Resources::getPauseTemplate() {
    ensureLoadedFromStore(g_pauseTemplate, "Pause");
    return g_pauseTemplate;
}

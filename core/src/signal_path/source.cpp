#include <signal_path/source.h>
#include <spdlog/spdlog.h>
#include <signal_path/signal_path.h>

SourceManager::SourceManager() {

}

void SourceManager::registerSource(std::string name, SourceHandler* handler) {
    if (sources.find(name) != sources.end()) {
        spdlog::error("Tried to register new source with existing name: {0}", name);
        return;
    }
    sources[name] = handler;
    onSourceRegistered.emit(name);
}

void SourceManager::unregisterSource(std::string name) {
    if (sources.find(name) == sources.end()) {
        spdlog::error("Tried to unregister non existant source: {0}", name);
        return;
    }
    onSourceUnregister.emit(name);
    if (name == selectedName) {
        sigpath::signalPath.setInput(&nullSource);
        selectedHandler = NULL;
    }
    sources.erase(name);
    onSourceUnregistered.emit(name);
}

std::vector<std::string> SourceManager::getSourceNames() {
    std::vector<std::string> names;
    for (auto const&  [name, src] : sources) { names.push_back(name); }
    return names;
}

void SourceManager::selectSource(std::string  name) {
    if (sources.find(name) == sources.end()) {
        spdlog::error("Tried to select non existant source: {0}", name);
        return;
    }
    if (selectedHandler != NULL) {
        sources[selectedName]->deselectHandler(sources[selectedName]->ctx);
    }
    selectedHandler = sources[name];
    selectedHandler->selectHandler(selectedHandler->ctx);
    selectedName = name;
    sigpath::signalPath.setInput(selectedHandler->stream);
}

void SourceManager::showSelectedMenu() {
    if (selectedHandler == NULL) {
        return;
    }
    selectedHandler->menuHandler(selectedHandler->ctx);
}

void SourceManager::start() {
    if (selectedHandler == NULL) {
        return;
    }
    selectedHandler->startHandler(selectedHandler->ctx);
}

void SourceManager::stop() {
    if (selectedHandler == NULL) {
        return;
    }
    selectedHandler->stopHandler(selectedHandler->ctx);
}

void SourceManager::tune(double freq) {
    if (selectedHandler == NULL) {
        return;
    }
    selectedHandler->tuneHandler(freq + tuneOffset, selectedHandler->ctx);
    currentFreq = freq;
}

void SourceManager::setTuningOffset(double offset) {
    tuneOffset = offset;
    tune(currentFreq);
}
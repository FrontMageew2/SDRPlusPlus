#include <imgui.h>
#include <spdlog/spdlog.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <core.h>
#include <gui/style.h>
#include <config.h>
#include <options.h>
#include <gui/widgets/stepped_slider.h>
#include <libbladeRF.h>
#include <dsp/processing.h>

#define CONCAT(a, b) ((std::string(a) + b).c_str())

#define NUM_BUFFERS     128
#define NUM_TRANSFERS   1

SDRPP_MOD_INFO {
    /* Name:            */ "bladerf_source",
    /* Description:     */ "BladeRF source module for SDR++",
    /* Author:          */ "Ryzerth",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ 1
};

const uint64_t sampleRates[] = {
    520834,
    1000000,
    2000000,
    4000000,
    5000000,
    8000000,
    10000000,
    15000000,
    20000000,
    25000000,
    30000000,
    35000000,
    40000000,
    45000000,
    50000000,
    55000000,
    61440000
};

const char* sampleRatesTxt = 
    "520.834KHz\0"
    "1MHz\0"
    "2MHz\0"
    "4MHz\0"
    "5MHz\0"
    "8MHz\0"
    "10MHz\0"
    "15MHz\0"
    "20MHz\0"
    "25MHz\0"
    "30MHz\0"
    "35MHz\0"
    "40MHz\0"
    "45MHz\0"
    "50MHz\0"
    "55MHz\0"
    "61.44MHz\0";

ConfigManager config;

class BladeRFSourceModule : public ModuleManager::Instance {
public:
    BladeRFSourceModule(std::string name) {
        this->name = name;

        sampleRate = 768000.0;

        handler.ctx = this;
        handler.selectHandler = menuSelected;
        handler.deselectHandler = menuDeselected;
        handler.menuHandler = menuHandler;
        handler.startHandler = start;
        handler.stopHandler = stop;
        handler.tuneHandler = tune;
        handler.stream = &stream;

        refresh();
        selectFirst();

        // Select device here
        core::setInputSampleRate(sampleRate);

        sigpath::sourceManager.registerSource("BladeRF", &handler);
    }

    ~BladeRFSourceModule() {
        
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

    void refresh() {
        devListTxt = "";

        if (devInfoList != NULL) {
            bladerf_free_device_list(devInfoList);
        }

        devCount = bladerf_get_device_list(&devInfoList);
        if (devCount < 0) {
            spdlog::error("Could not list devices");
            return;
        }
        for (int i = 0; i < devCount; i++) {
            devListTxt += devInfoList[i].serial;
            devListTxt += '\0';
        }
    }

    void selectFirst() {
        if (devCount > 0) { selectByInfo(&devInfoList[0]); }
    }

    void selectByInfo(bladerf_devinfo* info) {
        int ret = bladerf_open_with_devinfo(&openDev, info);
        if (ret != 0) {
            spdlog::error("Could not open device {0}", info->serial);
            return;
        }

        // Gather info about the BladeRF
        channelCount = bladerf_get_channel_count(openDev, BLADERF_RX);
        bladerf_get_sample_rate_range(openDev, BLADERF_CHANNEL_RX(0), &srRange);
        bladerf_get_bandwidth_range(openDev, BLADERF_CHANNEL_RX(0), &bwRange);
        bladerf_get_gain_range(openDev, BLADERF_CHANNEL_RX(0), &gainRange);

        spdlog::warn("SR Range: {0}, {1}, {2}, {3}", srRange->min, srRange->max, srRange->step, srRange->scale);
        spdlog::warn("BW Range: {0}, {1}, {2}, {3}", bwRange->min, bwRange->max, bwRange->step, bwRange->scale);
        spdlog::warn("Gain Range: {0}, {1}, {2}, {3}", gainRange->min, gainRange->max, gainRange->step, gainRange->scale);

        // TODO: Gen sample rate list automatically by detecting which version is selected

        bladerf_close(openDev);
    }

private:
    std::string getBandwdithScaled(double bw) {
        char buf[1024];
        if (bw >= 1000000.0) {
            sprintf(buf, "%.1lfMHz", bw / 1000000.0);
        }
        else if (bw >= 1000.0) {
            sprintf(buf, "%.1lfKHz", bw / 1000.0);
        }
        else {
            sprintf(buf, "%.1lfHz", bw);
        }
        return std::string(buf);
    }

    static void menuSelected(void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        core::setInputSampleRate(_this->sampleRate);
        spdlog::info("BladeRFSourceModule '{0}': Menu Select!", _this->name);
    }

    static void menuDeselected(void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        spdlog::info("BladeRFSourceModule '{0}': Menu Deselect!", _this->name);
    }
    
    static void start(void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        if (_this->running) {
            return;
        }
        if (_this->devCount == 0) { return; }

        // Open device
        bladerf_devinfo info = _this->devInfoList[_this->devId];
        int ret = bladerf_open_with_devinfo(&_this->openDev, &info);
        if (ret != 0) {
            spdlog::error("Could not open device {0}", info.serial);
            return;
        }

        _this->bufferSize = _this->sampleRate / 200.0;
        _this->bufferSize /= 1024;
        _this->bufferSize *= 1024;
        if (_this->bufferSize < 1024) { _this->bufferSize = 1024; }

        bladerf_sample_rate wantedSr = _this->sampleRate;
        bladerf_sample_rate actualSr;
        bladerf_sample_rate actualBw;
        bladerf_set_sample_rate(_this->openDev, BLADERF_CHANNEL_RX(0), wantedSr, &actualSr);
        bladerf_set_frequency(_this->openDev, BLADERF_CHANNEL_RX(0), _this->freq);
        bladerf_set_bandwidth(_this->openDev, BLADERF_CHANNEL_RX(0), 56000000, &actualBw);
        bladerf_set_gain_mode(_this->openDev, BLADERF_CHANNEL_RX(0), BLADERF_GAIN_MANUAL);
        bladerf_set_gain(_this->openDev, BLADERF_CHANNEL_RX(0), _this->testGain);

        if (actualSr != wantedSr) {
            spdlog::warn("Sample rate rejected: {0} vs {1}", actualSr, wantedSr);
            return;
        }

        // Setup syncronous transfer
        bladerf_sync_config(_this->openDev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 16, _this->bufferSize, 8, 3500);

        bladerf_enable_module(_this->openDev, BLADERF_CHANNEL_RX(0), true);

        _this->running = true;
        _this->workerThread = std::thread(&BladeRFSourceModule::worker, _this);

        spdlog::info("BladeRFSourceModule '{0}': Start!", _this->name);
    }
    
    static void stop(void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        if (!_this->running) {
            return;
        }
        _this->running = false;
        _this->stream.stopWriter();
        
        bladerf_enable_module(_this->openDev, BLADERF_CHANNEL_RX(0), false);
        if (_this->workerThread.joinable()) {
            _this->workerThread.join();
        }

        bladerf_close(_this->openDev);

        _this->stream.clearWriteStop();
        spdlog::info("BladeRFSourceModule '{0}': Stop!", _this->name);
    }
    
    static void tune(double freq, void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        _this->freq = freq;
        if (_this->running) {
            bladerf_set_frequency(_this->openDev, BLADERF_CHANNEL_RX(0), _this->freq);
        }
        spdlog::info("BladeRFSourceModule '{0}': Tune: {1}!", _this->name, freq);
    }
    
    static void menuHandler(void* ctx) {
        BladeRFSourceModule* _this = (BladeRFSourceModule*)ctx;
        float menuWidth = ImGui::GetContentRegionAvailWidth();

        if (_this->running) { style::beginDisabled(); }

        ImGui::SetNextItemWidth(menuWidth);
        if (ImGui::Combo(CONCAT("##_balderf_dev_sel_", _this->name), &_this->devId, _this->devListTxt.c_str())) {
            // Select device
            core::setInputSampleRate(_this->sampleRate);
            // Save config
        }

        if (ImGui::Combo(CONCAT("##_balderf_sr_sel_", _this->name), &_this->srId, sampleRatesTxt)) {
            _this->sampleRate = sampleRates[_this->srId];
            core::setInputSampleRate(_this->sampleRate);
            // Save config
        }

        ImGui::SameLine();
        float refreshBtnWdith = menuWidth - ImGui::GetCursorPosX();
        if (ImGui::Button(CONCAT("Refresh##_balderf_refr_", _this->name), ImVec2(refreshBtnWdith, 0))) {
            _this->refresh();
            config.aquire();
            std::string devSerial = config.conf["device"];
            config.release();
            // Reselect device
            core::setInputSampleRate(_this->sampleRate);
        }

        if (_this->running) { style::endDisabled(); }

        // General config BS
        if (ImGui::SliderInt("Test Gain", &_this->testGain, (_this->gainRange != NULL) ? _this->gainRange->min : 0, (_this->gainRange != NULL) ? _this->gainRange->max : 60)) {
            if (_this->running) {
                spdlog::info("Setting gain to {0}", _this->testGain);
                bladerf_set_gain(_this->openDev, BLADERF_CHANNEL_RX(0), _this->testGain);
            }
        }

    }

    void worker() {
        int16_t* buffer = new int16_t[bufferSize * 2];
        bladerf_metadata meta;
        while (true) {
            int ret = bladerf_sync_rx(openDev, buffer, bufferSize, &meta, 3500);
            if (ret != 0) { printf("Error: %d\n", ret); break; }
            volk_16i_s32f_convert_32f((float*)stream.writeBuf, buffer, 32768.0f, bufferSize * 2);
            if (!stream.swap(bufferSize)) { break; }
        }
        delete[] buffer;
    }

    std::string name;
    bladerf* openDev;
    bool enabled = true;
    dsp::stream<dsp::complex_t> stream;
    double sampleRate;
    SourceManager::SourceHandler handler;
    bool running = false;
    double freq;
    int devId = 0;
    int srId = 0;
    int chanId = 0;

    int channelCount;

    const bladerf_range* srRange = NULL;
    const bladerf_range* bwRange = NULL;
    const bladerf_range* gainRange = NULL;

    int bufferSize;
    struct bladerf_stream* rxStream;

    int testGain = 0;

    std::thread workerThread;

    int devCount = 0;
    bladerf_devinfo* devInfoList = NULL;
    std::string devListTxt;
};

MOD_EXPORT void _INIT_() {
    json def = json({});
    def["devices"] = json({});
    def["device"] = "";
    config.setPath(options::opts.root + "/bladerf_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    return new BladeRFSourceModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance) {
    delete (BladeRFSourceModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}
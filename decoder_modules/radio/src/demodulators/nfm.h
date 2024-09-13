#pragma once
#include "../demod.h"
#include <dsp/demod/fm.h>
#include <dsp/convert/mono_to_stereo.h>

namespace demod {
    class NFM : public Demodulator {
    public:
        NFM() {}

        NFM(std::string name, ConfigManager* config, dsp::stream<dsp::complex_t>* input, double bandwidth, EventHandler<dsp::stream<dsp::stereo_t>*> outputChangeHandler, EventHandler<float> afbwChangeHandler, double audioSR) {
            init(name, config, input, bandwidth, outputChangeHandler, afbwChangeHandler, audioSR);
        }

        ~NFM() {
            stop();
        }

        void init(std::string name, ConfigManager* config, dsp::stream<dsp::complex_t>* input, double bandwidth, EventHandler<dsp::stream<dsp::stereo_t>*> outputChangeHandler, EventHandler<float> afbwChangeHandler, double audioSR) {
            this->name = name;

            // Define structure
            demod.init(input, bandwidth / 2.0, getIFSampleRate());
            m2s.init(&demod.out);
        }

        void start() {
            demod.start();
            m2s.start();
        }

        void stop() {
            demod.stop();
            m2s.stop();
        }

        void showMenu() {}

        void setBandwidth(double bandwidth) {
            demod.setDeviation(bandwidth / 2.0, getIFSampleRate());
        }

        void setInput(dsp::stream<dsp::complex_t>* input) {
            demod.setInput(input);
        }

        void AFSampRateChanged(double newSR) {}

        // ============= INFO =============

        const char* getName() { return "FM"; }
        double getIFSampleRate() { return 50000.0; }
        double getAFSampleRate() { return getIFSampleRate(); }
        double getDefaultBandwidth() { return 12500.0; }
        double getMinBandwidth() { return 1000.0; }
        double getMaxBandwidth() { return getIFSampleRate(); }
        bool getBandwidthLocked() { return false; }
        double getMaxAFBandwidth() { return getIFSampleRate() / 2.0; }
        double getDefaultSnapInterval() { return 2500.0; }
        int getVFOReference() { return ImGui::WaterfallVFO::REF_CENTER; }
        bool getDeempAllowed() { return true; }
        bool getPostProcEnabled() { return true; }
        int getDefaultDeemphasisMode() { return DEEMP_MODE_NONE; }
        double getAFBandwidth(double bandwidth) { return bandwidth / 2.0; }
        bool getDynamicAFBandwidth() { return true; }
        bool getFMIFNRAllowed() { return true; }
        bool getNBAllowed() { return false; }
        dsp::stream<dsp::stereo_t>* getOutput() { return &m2s.out; }

    private:
        dsp::demod::FM demod;
        dsp::convert::MonoToStereo m2s;

        std::string name;
    };
}
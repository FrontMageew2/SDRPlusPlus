#pragma once
#include <dsp/filter.h>
#include <dsp/resampling.h>
#include <dsp/source.h>
#include <dsp/math.h>
#include <dsp/demodulator.h>
#include <dsp/routing.h>
#include <dsp/sink.h>
#include <dsp/vfo.h>
#include <map>
#include <module.h>

class SignalPath {
public:
    SignalPath();
    void init(uint64_t sampleRate, int fftRate, int fftSize, dsp::stream<dsp::complex_t>* input, dsp::complex_t* fftBuffer, void fftHandler(dsp::complex_t*,int,void*));
    void start();
    void setSampleRate(double sampleRate);
    void setFFTRate(double rate);
    double getSampleRate();
    dsp::VFO* addVFO(std::string name, double outSampleRate, double bandwidth, double offset);
    void removeVFO(std::string name);
    void setInput(dsp::stream<dsp::complex_t>* input);
    void bindIQStream(dsp::stream<dsp::complex_t>* stream);
    void unbindIQStream(dsp::stream<dsp::complex_t>* stream);

private:
    struct VFO_t {
        dsp::stream<dsp::complex_t>* inputStream;
        dsp::VFO* vfo;
    };

    dsp::Splitter<dsp::complex_t> split;

    // FFT
    dsp::stream<dsp::complex_t> fftStream;
    dsp::Reshaper<dsp::complex_t> reshape;
    dsp::HandlerSink<dsp::complex_t> fftHandlerSink;

    // VFO
    std::map<std::string, VFO_t> vfos;

    double sampleRate;
    double fftRate;
    int fftSize;
    int inputBlockSize;
};
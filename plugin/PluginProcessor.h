#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "FilterBank.h"

class SuzgecProcessor : public juce::AudioProcessor {
public:
    SuzgecProcessor();
    ~SuzgecProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // --- arayüz yardımcıları (mesaj iş parçacığı)
    static juce::String bandParamId(int band); // "band01" .. "band48"
    juce::RangedAudioParameter* bandParam(int band) const { return bandParams[(size_t) band]; }
    float getBandLevel(int band) const;          // 0..1
    void flipBands();                            // her bandı 1 - seviye yap
    juce::String presetToString();               // panoya kopyalama
    bool presetFromString(const juce::String&);  // panodan yapıştırma

    // Ring Mod bant seçimi (mesaj iş parçacığı)
    std::uint64_t getRingMask() const { return ringMask.load(); }
    void setRingMask(std::uint64_t mask);
    void toggleRingBand(int band);
    void randomOrClearRing(); // seçim yoksa rastgele 3-5 bant, varsa temizle

    // Trip'in o anki hızı (senkronda tempodan hesaplanan), Hz
    float getTripRateHz() const { return tripRateShown.load(std::memory_order_relaxed); }

    // Arayüz ayarları (projeyle birlikte saklanır, ses etkilemez)
    int getPalette() const;
    void setPalette(int index);
    bool getCrt() const;
    void setCrt(bool on);
    int getUiWidth() const;
    void setUiWidth(int w);

    sz::FilterBank bank;
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    sz::Params readParams() const;
    void loadRingMaskFromState();

    std::atomic<std::uint64_t> ringMask { 0 };
    std::atomic<float> tripRateShown { 0.5f };
    std::atomic<float>* pTripOn = nullptr;
    std::atomic<float>* pTripSpeed = nullptr;
    std::atomic<float>* pTripSync = nullptr;
    std::atomic<float>* pTripDiv = nullptr;
    std::atomic<float>* pTripIntensity = nullptr;
    std::atomic<float>* pTripShape = nullptr;
    std::atomic<float>* pTripPhase = nullptr;
    std::atomic<float>* pTripToPan = nullptr;
    std::atomic<float>* pRingOn = nullptr;
    std::atomic<float>* pDownsample = nullptr;

    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pSoftClip = nullptr;
    std::atomic<float>* pShift = nullptr;
    std::atomic<float>* pSlope = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pPanEven = nullptr;
    std::atomic<float>* pPanOdd = nullptr;
    std::array<std::atomic<float>*, sz::kNumBands> pBands{};
    std::array<juce::RangedAudioParameter*, sz::kNumBands> bandParams{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuzgecProcessor)
};

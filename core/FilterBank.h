#pragma once
// Süzgeç — çerçeveden bağımsız DSP çekirdeği.
// 48 bant geçiren filtre, merkezleri 55 Hz'den başlayıp 9:8 oranıyla (tam ses) dizilir.

#include <array>
#include <atomic>
#include <string>

namespace sz {

constexpr int kNumBands = 48;
constexpr double kBaseFreq = 55.0;
constexpr double kRatio = 9.0 / 8.0;

// Bant merkez frekansı (band: 0..47, shift: -1/0/+1 yarım ton)
double bandFrequency(int band, int shiftSemitones = 0);

// Frekansın en yakın nota adı, örn. "A4" ya da "C#5" (cents isteğe bağlı)
std::string noteName(double hz, double* centsOut = nullptr);

// Rezonans (0..1) -> Q
float resonanceToQ(float resonance01);

// Yumuşak kırpma eğrisi (0.6'ya kadar doğrusal, sonra 1'e yumuşakça yaklaşır)
float softClip(float x);

struct Params {
    float inputDb = 0.f;     // -24..+24
    float outputDb = 0.f;    // -24..+24
    float mix = 1.f;         // 0..1
    bool softClip = false;
    int shift = 0;           // -1, 0, +1
    bool slope24 = false;
    float resonance = 0.3f;  // 0..1
    float panEven = 0.f;     // -1..+1 (çift numaralı bantlar: 2, 4, ...)
    float panOdd = 0.f;      // -1..+1 (tek numaralı bantlar: 1, 3, ...)
    std::array<float, kNumBands> level{}; // 0..1
};

class FilterBank {
public:
    FilterBank();

    void prepare(double sampleRate);
    void reset();
    void setParams(const Params& p) { target = p; }

    // left/right yerinde işlenir. right == nullptr ise mono.
    void process(float* left, float* right, int numSamples);

    // Arayüz için (her blok sonunda güncellenir, 0..~1)
    float outputMeter(int band) const { return outMeter[(size_t) band].load(std::memory_order_relaxed); }
    float inputMeter(int band) const { return inMeter[(size_t) band].load(std::memory_order_relaxed); }

    // Tüm bantlar %100 iken toplamı yaklaşık birim kazanca getiren telafi
    float makeupGain(float resonance01, bool slope24) const;

private:
    struct Coeff { float k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f; };
    struct State { float ic1 = 0.f, ic2 = 0.f; };

    void updateCoefficients(float q, int shift);
    void buildMakeupTable();

    double fs = 48000.0;
    Params target;

    // yumuşatılmış durum
    std::array<float, kNumBands> level{};
    float inGain = 1.f, outGain = 1.f, mix = 1.f, panE = 0.f, panO = 0.f;
    float resSmoothed = 0.3f;
    float makeup = 1.f;
    float lastQ = -1.f;
    int lastShift = 99;
    bool lastSlope24 = false;
    float levelCoef = 0.f, meterCoef = 0.f;

    std::array<Coeff, kNumBands> coeff;
    // [kanal][kademe][bant]
    State st[2][2][kNumBands];

    std::array<float, kNumBands> envOut{}, envIn{};
    std::array<std::atomic<float>, kNumBands> outMeter, inMeter;

    static constexpr int kTableSize = 65;
    float makeupTable[2][kTableSize]{};
};

} // namespace sz

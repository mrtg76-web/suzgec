#pragma once
// Süzgeç — çerçeveden bağımsız DSP çekirdeği.
// 48 bant geçiren filtre, merkezleri 55 Hz'den başlayıp 9:8 oranıyla (tam ses) dizilir.

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <cmath>
#include <vector>

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

// Trip senkron bölümleri: 64 ölçü .. 1/64, noktalı (D) ve üçleme (T) dahil
struct SyncDivision {
    std::string name;
    double beats; // dörtlük cinsinden uzunluk (1 ölçü = 4)
};
const std::vector<SyncDivision>& syncDivisions();
int defaultSyncDivision(); // "1 ölçü"

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

    // Trip
    bool tripOn = false;
    double tripRateHz = 0.5;     // 0.01..2000
    bool tripLocked = false;     // true ise blok başında faz host konumuna kilitlenir
    double tripPosition = 0.0;   // host konumu, tur cinsinden (ppq / bölüm uzunluğu; ters yönde negatif)
    float tripIntensity = 0.5f;  // 0..1
    float tripShape = 0.f;       // 0 = sıralı, 0.5 = tek/çift, 1 = karışık
    bool tripReverse = false;
    float tripToPan = 0.f;       // 0..1

    // Ring Mod / Downsample
    bool ringOn = false;
    std::uint64_t ringMask = 0;  // bit b = bant b seçili
    bool downsample = false;
    int downsampleHold = 4;      // kaç örnek tutulacak (4 x oversampling çarpanı)

    // MIDI
    bool keyboardMode = false;   // true: notalar en yakın bandı nota frekansına akort eder
};

class FilterBank {
public:
    FilterBank();

    void prepare(double sampleRate);
    void reset();
    void setParams(const Params& p);

    // --- MIDI (ses iş parçacığında, process çağrıları arasında)
    void noteOn(int note, float velocity01);
    void noteOff(int note);
    void allNotesOff();
    void setAttackSeconds(float s);  // pitch wheel: 0.02 .. 5
    void setDecaySeconds(float s);   // mod wheel (CC1): 0.02 .. 5
    static float wheelToSeconds(float t01) { return 0.02f * std::pow(250.f, t01); }
    // Notanın gideceği bant (sabit bankada shift dikkate alınır)
    static int nearestBand(double hz, int shiftSemitones);

    // left/right yerinde işlenir. right == nullptr ise mono.
    void process(float* left, float* right, int numSamples);

    // Arayüz için (her blok sonunda güncellenir)
    float outputMeter(int band) const { return outMeter[(size_t) band].load(std::memory_order_relaxed); }
    float inputMeter(int band) const { return inMeter[(size_t) band].load(std::memory_order_relaxed); }
    float tripFactor(int band) const { return tripView[(size_t) band].load(std::memory_order_relaxed); }
    float tripPhaseNow() const { return phaseView.load(std::memory_order_relaxed); }
    float effectiveLevel(int band) const { return levelView[(size_t) band].load(std::memory_order_relaxed); }
    float bandFrequencyNow(int band) const { return freqView[(size_t) band].load(std::memory_order_relaxed); }
    bool bandMidiDriven(int band) const { return midiView[(size_t) band].load(std::memory_order_relaxed); }
    float attackSeconds() const { return attackSec.load(std::memory_order_relaxed); }
    float decaySeconds() const { return decaySec.load(std::memory_order_relaxed); }

    // Tüm bantlar %100 iken toplamı yaklaşık birim kazanca getiren telafi
    float makeupGain(float resonance01, bool slope24) const;

    // Trip deseninde bandın faz kayması (0..1), shape: 0..1
    static float tripOffset(int band, float shape);

private:
    struct Coeff { float k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f; };
    struct State { float ic1 = 0.f, ic2 = 0.f; };

    void updateCoefficients(float q, int shift);
    void updateBand(int b);
    void resetMidi();
    void buildMakeupTable();
    void refreshNoise();

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
    float levelCoef = 0.f, meterCoef = 0.f, ringCoef = 0.f, ringAttack = 0.f;

    // Trip durumu
    double tripPhase = 0.0;      // 0..1
    std::int64_t tripCycle = 0;  // tamamlanan tur sayısı (gürültü deseni için)
    std::int64_t noiseSegment = -1;
    float tripI = 0.f, tripP = 0.f, tripS = 0.f; // yumuşatılmış intensity / toPan / shape
    std::array<float, kNumBands> offsets{};
    std::array<float, kNumBands> noiseA{}, noiseB{};

    // Ring Mod / Downsample durumu
    float ringEnv = 0.f;
    int dsCounter = 0;
    float dsHoldL = 0.f, dsHoldR = 0.f;

    std::array<Coeff, kNumBands> coeff;
    // [kanal][kademe][bant]
    State st[2][2][kNumBands];

    std::array<float, kNumBands> envOut{}, envIn{}, lastTrip{};
    std::array<std::atomic<float>, kNumBands> outMeter, inMeter, tripView;
    std::atomic<float> phaseView { 0.f };
    std::array<std::atomic<float>, kNumBands> levelView, freqView;
    std::array<std::atomic<bool>, kNumBands> midiView;

    // MIDI durumu
    static constexpr int kStack = 8;
    std::array<float, kNumBands> env{}, envTarget{}, prevDrawn{};
    std::array<double, kNumBands> freqOverride{};   // 0 = yok
    std::array<bool, kNumBands> owned{};            // sabit bankada MIDI'nin devraldığı bant
    std::array<std::array<int, kStack>, kNumBands> held{};
    std::array<int, kNumBands> heldCount{};
    std::array<int, 128> noteBand{};
    std::array<float, 128> noteVel{};
    bool coeffDirty = true;
    float atkCoef = 0.f, decCoef = 0.f, fastCoef = 0.f;
    float curQ = 2.f;
    std::atomic<float> attackSec { 0.02f }, decaySec { 0.02f };

    static constexpr int kTableSize = 65;
    float makeupTable[2][kTableSize]{};
};

} // namespace sz

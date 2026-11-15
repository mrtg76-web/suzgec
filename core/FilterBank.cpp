#include "FilterBank.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace sz {

namespace {
constexpr double kPi = 3.14159265358979323846;

inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

// Hızlı tanh (|x| > 3 için ±1)
inline float fastTanh(float x)
{
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

// Bant seviyesi -> kazanç (algısal: %50 ≈ -12 dB)
inline float levelCurve(float l) { return l * l; }

// Kosinüs tablosu (argüman tur cinsinden)
struct CosTable {
    static constexpr int N = 2048;
    float t[N + 1];
    CosTable()
    {
        for (int i = 0; i <= N; ++i) t[i] = (float) std::cos(2.0 * kPi * i / N);
    }
};
const CosTable& cosTable()
{
    static const CosTable tb;
    return tb;
}
inline float cosTurn(float x)
{
    x -= std::floor(x);
    const float pos = x * CosTable::N;
    const int i = std::min((int) pos, CosTable::N - 1);
    const float fr = pos - (float) i;
    const float* t = cosTable().t;
    return t[i] + (t[i + 1] - t[i]) * fr;
}

// Tamsayı karması -> [-1, 1]
inline float hashNoise(std::uint64_t a, std::uint64_t b)
{
    std::uint64_t z = a * 0x9E3779B97F4A7C15ull + b * 0xBF58476D1CE4E5B9ull + 0x94D049BB133111EBull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return (float) ((double) (z >> 11) / (double) (1ull << 53)) * 2.f - 1.f;
}

// Karışık desen için sabit bir permütasyon
const std::array<int, kNumBands>& shufflePerm()
{
    static const std::array<int, kNumBands> perm = [] {
        std::array<int, kNumBands> p {};
        for (int i = 0; i < kNumBands; ++i) p[(size_t) i] = i;
        std::uint32_t r = 0x5eed1234u;
        for (int i = kNumBands - 1; i > 0; --i) {
            r = r * 1664525u + 1013904223u;
            const int j = (int) ((r >> 8) % (std::uint32_t) (i + 1));
            std::swap(p[(size_t) i], p[(size_t) j]);
        }
        return p;
    }();
    return perm;
}

// Stereo pan: p > 0 sola düşen sinyali sağa taşır, enerji korunur
inline void panPair(float& l, float& r, float p)
{
    if (p > 0.f) {
        const float moved = l * p;
        l -= moved;
        r += moved;
    } else if (p < 0.f) {
        const float moved = r * -p;
        r -= moved;
        l += moved;
    }
}
} // namespace

double bandFrequency(int band, int shiftSemitones)
{
    return kBaseFreq * std::pow(kRatio, (double) band) * std::pow(2.0, shiftSemitones / 12.0);
}

std::string noteName(double hz, double* centsOut)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (hz <= 0.0) return "-";
    const double m = 69.0 + 12.0 * std::log2(hz / 440.0);
    const int midi = (int) std::lround(m);
    if (centsOut) *centsOut = (m - midi) * 100.0;
    const int octave = (int) std::floor(midi / 12.0) - 1;
    return std::string(names[((midi % 12) + 12) % 12]) + std::to_string(octave);
}

float resonanceToQ(float r)
{
    r = std::clamp(r, 0.f, 1.f);
    return 2.f * std::pow(50.f, r); // 2 .. 100
}

float softClip(float x)
{
    constexpr float t = 0.6f;
    const float a = std::fabs(x);
    if (a <= t) return x;
    const float y = t + (1.f - t) * std::tanh((a - t) / (1.f - t));
    return x < 0.f ? -y : y;
}

const std::vector<SyncDivision>& syncDivisions()
{
    static const std::vector<SyncDivision> list = [] {
        std::vector<SyncDivision> v;
        struct Base { const char* name; double beats; };
        const Base bases[] = {
            { "64 \xC3\xB6l\xC3\xA7\xC3\xBC", 256 }, { "32 \xC3\xB6l\xC3\xA7\xC3\xBC", 128 },
            { "16 \xC3\xB6l\xC3\xA7\xC3\xBC", 64 },  { "8 \xC3\xB6l\xC3\xA7\xC3\xBC", 32 },
            { "4 \xC3\xB6l\xC3\xA7\xC3\xBC", 16 },   { "2 \xC3\xB6l\xC3\xA7\xC3\xBC", 8 },
            { "1 \xC3\xB6l\xC3\xA7\xC3\xBC", 4 },    { "1/2", 2 }, { "1/4", 1 }, { "1/8", 0.5 },
            { "1/16", 0.25 }, { "1/32", 0.125 }, { "1/64", 0.0625 },
        };
        bool first = true;
        for (const auto& b : bases) {
            if (!first) v.push_back({ std::string(b.name) + "D", b.beats * 1.5 });
            v.push_back({ b.name, b.beats });
            v.push_back({ std::string(b.name) + "T", b.beats * 2.0 / 3.0 });
            first = false;
        }
        return v;
    }();
    return list;
}

int defaultSyncDivision()
{
    const auto& v = syncDivisions();
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i].beats == 4.0) return (int) i;
    return 0;
}

float FilterBank::tripOffset(int b, float shape)
{
    shape = std::clamp(shape, 0.f, 1.f);
    const float seq = (float) b / kNumBands;
    const float alt = (b & 1) ? 0.5f : 0.f;
    const float shuf = (float) shufflePerm()[(size_t) b] / kNumBands;
    if (shape < 0.5f) return seq + (alt - seq) * (shape * 2.f);
    return alt + (shuf - alt) * ((shape - 0.5f) * 2.f);
}

FilterBank::FilterBank()
{
    for (auto& m : tripView) m.store(1.f);
    for (auto& m : outMeter) m.store(0.f);
    for (auto& m : inMeter) m.store(0.f);
    buildMakeupTable();
    reset();
}

void FilterBank::buildMakeupTable()
{
    // Tüm bantlar açıkken toplam yanıtın ortalama gücünü analog modelden ölç.
    constexpr int kGrid = 160;
    for (int s = 0; s < 2; ++s) {
        for (int t = 0; t < kTableSize; ++t) {
            const double q = resonanceToQ((float) t / (kTableSize - 1));
            const double k = 1.0 / q;
            double power = 0.0;
            for (int i = 0; i < kGrid; ++i) {
                const double f = 110.0 * std::pow(7000.0 / 110.0, (double) i / (kGrid - 1));
                std::complex<double> sum(0.0, 0.0);
                for (int b = 0; b < kNumBands; ++b) {
                    const double x = f / bandFrequency(b);
                    std::complex<double> h = std::complex<double>(0.0, k * x)
                                           / std::complex<double>(1.0 - x * x, k * x);
                    if (s == 1) h *= h;
                    sum += h;
                }
                power += std::norm(sum);
            }
            const double rms = std::sqrt(power / kGrid);
            makeupTable[s][t] = (float) std::clamp(1.0 / rms, 0.25, 4.0);
        }
    }
}

float FilterBank::makeupGain(float r, bool slope24) const
{
    const float pos = std::clamp(r, 0.f, 1.f) * (kTableSize - 1);
    const int i = std::min((int) pos, kTableSize - 2);
    const float fr = pos - (float) i;
    const auto& tb = makeupTable[slope24 ? 1 : 0];
    return tb[i] + (tb[i + 1] - tb[i]) * fr;
}

void FilterBank::prepare(double sampleRate)
{
    fs = sampleRate > 0 ? sampleRate : 48000.0;
    levelCoef = (float) (1.0 - std::exp(-1.0 / (0.015 * fs)));
    meterCoef = (float) (1.0 - std::exp(-1.0 / (0.05 * fs)));
    ringCoef = (float) (1.0 - std::exp(-1.0 / (0.05 * fs)));       // bırakma
    ringAttack = (float) (1.0 - std::exp(-1.0 / (0.001 * fs)));     // atak
    reset();
}

void FilterBank::reset()
{
    for (auto& ch : st)
        for (auto& stage : ch)
            for (auto& s : stage) s = State{};
    level = target.level;
    inGain = dbToGain(target.inputDb);
    outGain = dbToGain(target.outputDb);
    mix = target.mix;
    panE = target.panEven;
    panO = target.panOdd;
    resSmoothed = target.resonance;
    lastSlope24 = target.slope24;
    lastQ = -1.f;
    lastShift = 99;
    envOut.fill(0.f);
    envIn.fill(0.f);
    lastTrip.fill(1.f);
    tripPhase = 0.0;
    tripCycle = 0;
    noiseSegment = -1;
    tripI = target.tripIntensity;
    tripP = target.tripToPan;
    tripS = target.tripShape;
    ringEnv = 0.f;
    dsCounter = 0;
    dsHoldL = dsHoldR = 0.f;
    updateCoefficients(resonanceToQ(resSmoothed), target.shift);
    makeup = makeupGain(resSmoothed, target.slope24);
}

void FilterBank::refreshNoise()
{
    for (int b = 0; b < kNumBands; ++b) {
        noiseA[(size_t) b] = hashNoise((std::uint64_t) b, (std::uint64_t) noiseSegment);
        noiseB[(size_t) b] = hashNoise((std::uint64_t) b, (std::uint64_t) (noiseSegment + 1));
    }
}

void FilterBank::updateCoefficients(float q, int shift)
{
    const double k = 1.0 / q;
    const double fMax = 0.45 * fs;
    for (int b = 0; b < kNumBands; ++b) {
        const double f = std::min(bandFrequency(b, shift), fMax);
        const double g = std::tan(kPi * f / fs);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        coeff[(size_t) b].k = (float) k;
        coeff[(size_t) b].a1 = (float) a1;
        coeff[(size_t) b].a2 = (float) (g * a1);
        coeff[(size_t) b].a3 = (float) (g * g * a1);
    }
    lastQ = q;
    lastShift = shift;
}

void FilterBank::process(float* left, float* right, int n)
{
    if (n <= 0) return;

    // --- blok başı: yavaş parametreler
    const float blockCoef = 1.f - std::exp(-(float) n / (0.03f * (float) fs));
    resSmoothed += (target.resonance - resSmoothed) * blockCoef;
    const float q = resonanceToQ(resSmoothed);
    if (target.shift != lastShift || std::fabs(q - lastQ) > lastQ * 1e-4f)
        updateCoefficients(q, target.shift);

    const bool slope24 = target.slope24;
    if (slope24 != lastSlope24) {
        for (auto& ch : st)
            for (auto& s : ch[1]) s = State{};
        lastSlope24 = slope24;
    }

    const float makeupTarget = makeupGain(resSmoothed, slope24);

    // doğrusal rampalar
    const float invN = 1.f / (float) n;
    const float inT = dbToGain(target.inputDb), outT = dbToGain(target.outputDb);
    const float dIn = (inT - inGain) * invN, dOut = (outT - outGain) * invN;
    const float dMix = (target.mix - mix) * invN;
    const float dPE = (target.panEven - panE) * invN, dPO = (target.panOdd - panO) * invN;
    const float dMk = (makeupTarget - makeup) * invN;

    const bool stereo = right != nullptr;
    const bool clip = target.softClip;

    // --- Trip
    const bool tripOn = target.tripOn;
    const double dir = target.tripReverse ? -1.0 : 1.0;
    const double inc = std::clamp(target.tripRateHz, 0.0, 0.45 * fs) / fs * dir;
    if (tripOn && target.tripLocked) {
        const double pos = target.tripPosition;
        const double cyc = std::floor(pos);
        tripCycle = (std::int64_t) cyc;
        tripPhase = pos - cyc;
    }
    const float dTI = (target.tripIntensity - tripI) * invN;
    const float dTP = (target.tripToPan - tripP) * invN;
    tripS += (target.tripShape - tripS) * blockCoef;
    if (tripOn)
        for (int b = 0; b < kNumBands; ++b) offsets[(size_t) b] = tripOffset(b, tripS);

    // --- Ring Mod
    const std::uint64_t mask = target.ringOn ? target.ringMask : 0;
    const bool ring = mask != 0;
    const bool ds = target.downsample;

    auto tick = [](State& s, const Coeff& c, float v0) {
        const float v3 = v0 - s.ic2;
        const float v1 = c.a1 * s.ic1 + c.a2 * v3;
        const float v2 = s.ic2 + c.a2 * s.ic1 + c.a3 * v3;
        s.ic1 = 2.f * v1 - s.ic1;
        s.ic2 = 2.f * v2 - s.ic2;
        return c.k * v1;
    };

    for (int i = 0; i < n; ++i) {
        inGain += dIn;
        outGain += dOut;
        mix += dMix;
        panE += dPE;
        panO += dPO;
        makeup += dMk;
        tripI += dTI;
        tripP += dTP;

        float noiseFrac = 0.f;
        float ph = 0.f;
        if (tripOn) {
            tripPhase += inc;
            if (tripPhase >= 1.0) {
                tripPhase -= 1.0;
                ++tripCycle;
            } else if (tripPhase < 0.0) {
                tripPhase += 1.0;
                --tripCycle;
            }
            const double segPos = 4.0 * ((double) tripCycle + tripPhase);
            const double seg = std::floor(segPos);
            if ((std::int64_t) seg != noiseSegment) {
                noiseSegment = (std::int64_t) seg;
                refreshNoise();
            }
            const float f = (float) (segPos - seg);
            noiseFrac = f * f * (3.f - 2.f * f);
            ph = (float) tripPhase;
        }

        const float dryL = left[i];
        const float dryR = stereo ? right[i] : 0.f;
        const float xL = dryL * inGain;
        const float xR = dryR * inGain;

        float sumL = 0.f, sumR = 0.f, modL = 0.f, modR = 0.f;

        for (int b = 0; b < kNumBands; ++b) {
            const auto& c = coeff[(size_t) b];
            float& lv = level[(size_t) b];
            lv += (target.level[(size_t) b] - lv) * levelCoef;
            float gain = levelCurve(lv);
            // bant numarası = b + 1 -> b tekse bant numarası çift
            float pan = (b & 1) ? panE : panO;

            if (tripOn) {
                const float bp = ph - offsets[(size_t) b];
                const float m = 0.5f - 0.5f * cosTurn(bp); // 0..1
                const float fac = 1.f - tripI * (1.f - m);
                gain *= fac;
                lastTrip[(size_t) b] = fac;
                if (tripP > 1e-4f) {
                    const float smooth = cosTurn(bp - 0.25f); // sinüs
                    const float nz = noiseA[(size_t) b] + (noiseB[(size_t) b] - noiseA[(size_t) b]) * noiseFrac;
                    const float w = smooth + (nz - smooth) * tripS;
                    pan = std::clamp(pan + tripP * w, -1.f, 1.f);
                }
            }

            float yL = tick(st[0][0][b], c, xL);
            if (slope24) yL = tick(st[0][1][b], c, yL);
            yL = fastTanh(yL);

            float yR = 0.f;
            if (stereo) {
                yR = tick(st[1][0][b], c, xR);
                if (slope24) yR = tick(st[1][1][b], c, yR);
                yR = fastTanh(yR);
            }

            const float pw = stereo ? 0.5f * (yL * yL + yR * yR) : yL * yL;
            envIn[(size_t) b] += (pw - envIn[(size_t) b]) * meterCoef;
            envOut[(size_t) b] += (pw * gain * gain - envOut[(size_t) b]) * meterCoef;

            float bL = yL * gain, bR = yR * gain;
            if (ring && ((mask >> b) & 1u)) {
                modL += bL;
                modR += bR;
            }
            if (stereo) panPair(bL, bR, pan);
            sumL += bL;
            sumR += bR;
        }

        float wetL = sumL * makeup;
        float wetR = sumR * makeup;

        if (ring) {
            // seçili bantların toplamı taşıyıcı: seviyesi otomatik dengelenir (AGC)
            const float pw = stereo ? 0.5f * (modL * modL + modR * modR) : modL * modL;
            ringEnv += (pw - ringEnv) * (pw > ringEnv ? ringAttack : ringCoef);
            const float agc = std::min(300.f, 1.f / std::sqrt(ringEnv + 1e-10f));
            // normalize taşıyıcının tepelerini yumuşat, seviyeyi ring kapalıya yaklaştır
            wetL *= 4.5f * fastTanh(modL * agc * 0.4f);
            wetR *= 4.5f * fastTanh(modR * agc * 0.4f);
        }

        if (ds) {
            if (dsCounter == 0) {
                dsHoldL = wetL;
                dsHoldR = wetR;
            }
            dsCounter = (dsCounter + 1) & 3;
            wetL = dsHoldL;
            wetR = dsHoldR;
        }

        float outL = (dryL + (wetL - dryL) * mix) * outGain;
        if (clip) outL = softClip(outL);
        left[i] = outL;
        if (stereo) {
            float outR = (dryR + (wetR - dryR) * mix) * outGain;
            if (clip) outR = softClip(outR);
            right[i] = outR;
        }
    }

    if (!tripOn) lastTrip.fill(1.f);
    phaseView.store((float) tripPhase, std::memory_order_relaxed);
    if (!ring) ringEnv = 0.f;

    // denormal temizliği ve sayaçlar
    for (int b = 0; b < kNumBands; ++b) {
        for (auto& ch : st)
            for (auto& stage : ch) {
                auto& s = stage[b];
                if (std::fabs(s.ic1) < 1e-20f) s.ic1 = 0.f;
                if (std::fabs(s.ic2) < 1e-20f) s.ic2 = 0.f;
            }
        outMeter[(size_t) b].store(std::sqrt(envOut[(size_t) b]), std::memory_order_relaxed);
        inMeter[(size_t) b].store(std::sqrt(envIn[(size_t) b]), std::memory_order_relaxed);
        tripView[(size_t) b].store(lastTrip[(size_t) b], std::memory_order_relaxed);
    }
}

} // namespace sz

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

FilterBank::FilterBank()
{
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
    updateCoefficients(resonanceToQ(resSmoothed), target.shift);
    makeup = makeupGain(resSmoothed, target.slope24);
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

        const float dryL = left[i];
        const float dryR = stereo ? right[i] : 0.f;
        const float xL = dryL * inGain;
        const float xR = dryR * inGain;

        float eL = 0.f, eR = 0.f, oL = 0.f, oR = 0.f;

        for (int b = 0; b < kNumBands; ++b) {
            const auto& c = coeff[(size_t) b];
            float& lv = level[(size_t) b];
            lv += (target.level[(size_t) b] - lv) * levelCoef;
            const float gain = levelCurve(lv);

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

            // bant numarası = b + 1 -> b tekse bant numarası çift
            if (b & 1) {
                eL += yL * gain;
                eR += yR * gain;
            } else {
                oL += yL * gain;
                oR += yR * gain;
            }
        }

        float wetL, wetR = 0.f;
        if (stereo) {
            panPair(eL, eR, panE);
            panPair(oL, oR, panO);
            wetL = (eL + oL) * makeup;
            wetR = (eR + oR) * makeup;
        } else {
            wetL = (eL + oL) * makeup;
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
    }
}

} // namespace sz

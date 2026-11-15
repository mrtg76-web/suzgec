// Süzgeç çekirdek testleri (JUCE gerektirmez)
#include "FilterBank.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace sz;

static int failures = 0;
#define CHECK(cond, ...)                                                                     \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            ++failures;                                                                      \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__);                                 \
            std::printf(__VA_ARGS__);                                                        \
            std::printf("\n");                                                               \
        }                                                                                    \
    } while (0)

static constexpr double kFs = 48000.0;
static constexpr double kPi = 3.14159265358979323846;

static double rms(const std::vector<float>& v, size_t from = 0)
{
    double s = 0;
    for (size_t i = from; i < v.size(); ++i) s += (double) v[i] * v[i];
    return std::sqrt(s / (double) (v.size() - from));
}

static void run(FilterBank& fb, std::vector<float>& l, std::vector<float>& r, int block = 256)
{
    for (size_t i = 0; i < l.size(); i += (size_t) block) {
        const int n = (int) std::min<size_t>((size_t) block, l.size() - i);
        fb.process(l.data() + i, r.empty() ? nullptr : r.data() + i, n);
    }
}

static Params flat(float lvl)
{
    Params p;
    p.level.fill(lvl);
    return p;
}

static std::vector<float> sine(double hz, double amp, int n)
{
    std::vector<float> v((size_t) n);
    for (int i = 0; i < n; ++i) v[(size_t) i] = (float) (amp * std::sin(2 * kPi * hz * i / kFs));
    return v;
}

static void testFrequencies()
{
    CHECK(std::fabs(bandFrequency(0) - 55.0) < 1e-9, "band 1 = %f", bandFrequency(0));
    const double top = bandFrequency(47);
    CHECK(top > 13800 && top < 14000, "band 48 = %f", top);
    CHECK(std::fabs(bandFrequency(10) / bandFrequency(9) - 1.125) < 1e-9, "ratio");
    CHECK(std::fabs(bandFrequency(0, 1) - 55.0 * std::pow(2.0, 1.0 / 12)) < 1e-9, "shift +1");
    CHECK(noteName(440.0) == "A4", "A4 -> %s", noteName(440.0).c_str());
    CHECK(noteName(55.0) == "A1", "A1 -> %s", noteName(55.0).c_str());
    CHECK(noteName(261.63) == "C4", "C4 -> %s", noteName(261.63).c_str());
    std::printf("band 48 = %.1f Hz (%s)\n", top, noteName(top).c_str());
}

static void testMixZeroIsDry()
{
    FilterBank fb;
    fb.prepare(kFs);
    Params p = flat(0.7f);
    p.mix = 0.f;
    fb.setParams(p);
    fb.reset();
    auto l = sine(440, 0.5, 4800), r = sine(300, 0.5, 4800);
    auto l0 = l, r0 = r;
    run(fb, l, r);
    double err = 0;
    for (size_t i = 0; i < l.size(); ++i) err = std::max(err, (double) std::fabs(l[i] - l0[i]) + std::fabs(r[i] - r0[i]));
    CHECK(err < 1e-5, "mix 0 should be dry, err %g", err);
}

static double singleBandGain(int band, double hz, bool slope24, float res)
{
    FilterBank fb;
    fb.prepare(kFs);
    Params p = flat(0.f);
    p.level[(size_t) band] = 1.f;
    p.slope24 = slope24;
    p.resonance = res;
    fb.setParams(p);
    fb.reset();
    auto l = sine(hz, 0.05, 48000);
    std::vector<float> r;
    run(fb, l, r);
    return rms(l, 24000) / (0.05 / std::sqrt(2.0)) / fb.makeupGain(res, slope24);
}

static void testSelectivity()
{
    const int b = 20;
    const double f = bandFrequency(b);
    for (int s = 0; s < 2; ++s) {
        const double on = singleBandGain(b, f, s, 0.3f);
        const double off2 = singleBandGain(b, bandFrequency(b + 2), s, 0.3f);
        std::printf("slope %s: centre %.2f dB, 2 bands away %.1f dB\n", s ? "24" : "12", 20 * std::log10(on),
                    20 * std::log10(off2));
        CHECK(std::fabs(20 * std::log10(on)) < 0.5, "centre gain ~0 dB");
        CHECK(20 * std::log10(off2) < (s ? -16 : -8), "isolation");
    }
}

static void testFlatNoise()
{
    std::mt19937 rng(1);
    std::normal_distribution<float> nd(0.f, 0.03f);
    for (int s = 0; s < 2; ++s)
        for (float res : { 0.f, 0.3f, 0.6f, 1.f }) {
            FilterBank fb;
            fb.prepare(kFs);
            Params p = flat(1.f);
            p.slope24 = s;
            p.resonance = res;
            fb.setParams(p);
            fb.reset();
            std::vector<float> l(96000), r(96000);
            for (auto& x : l) x = nd(rng);
            for (auto& x : r) x = nd(rng);
            const double in = rms(l);
            run(fb, l, r);
            const double db = 20 * std::log10(rms(l, 48000) / in);
            std::printf("noise all bands, slope %s res %.1f: %+.1f dB\n", s ? "24" : "12", res, db);
            CHECK(db > -6 && db < 3, "all bands roughly unity");
        }
}

static void testPan()
{
    FilterBank fb;
    fb.prepare(kFs);
    Params p = flat(1.f);
    p.panEven = 1.f;
    p.panOdd = 1.f;
    fb.setParams(p);
    fb.reset();
    auto l = sine(440, 0.2, 48000), r = sine(440, 0.2, 48000);
    run(fb, l, r);
    const double L = rms(l, 24000), R = rms(r, 24000);
    std::printf("pan right: L %.4f R %.4f\n", L, R);
    CHECK(L < 1e-3 && R > 0.05, "hard right pan");
}

static void testStability()
{
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> u(0.f, 1.f);
    FilterBank fb;
    fb.prepare(44100.0);
    std::vector<float> l(256), r(256);
    bool bad = false;
    double peak = 0;
    for (int blk = 0; blk < 4000; ++blk) {
        if (blk % 20 == 0) {
            Params p;
            for (auto& v : p.level) v = u(rng);
            p.resonance = u(rng);
            p.slope24 = u(rng) > 0.5f;
            p.shift = (int) (u(rng) * 3) - 1;
            p.inputDb = u(rng) * 48 - 24;
            p.outputDb = 0;
            p.mix = u(rng);
            p.softClip = u(rng) > 0.5f;
            p.panEven = u(rng) * 2 - 1;
            p.panOdd = u(rng) * 2 - 1;
            fb.setParams(p);
        }
        for (size_t i = 0; i < l.size(); ++i) {
            l[i] = u(rng) * 2 - 1;
            r[i] = (i % 100 == 0) ? 1.f : 0.f;
        }
        fb.process(l.data(), r.data(), 256);
        for (size_t i = 0; i < l.size(); ++i) {
            if (!std::isfinite(l[i]) || !std::isfinite(r[i])) bad = true;
            peak = std::max(peak, (double) std::max(std::fabs(l[i]), std::fabs(r[i])));
        }
    }
    std::printf("stability peak %.2f\n", peak);
    CHECK(!bad, "NaN/inf in output");
    CHECK(peak < 100, "runaway output %f", peak);
}

static void testMono()
{
    FilterBank fb;
    fb.prepare(kFs);
    fb.setParams(flat(1.f));
    fb.reset();
    auto l = sine(1000, 0.1, 9600);
    fb.process(l.data(), nullptr, (int) l.size());
    CHECK(rms(l, 4800) > 0.03, "mono passes");
}

static void testPerformance()
{
    FilterBank fb;
    fb.prepare(kFs);
    Params p = flat(0.8f);
    p.slope24 = true;
    fb.setParams(p);
    std::vector<float> l(480000, 0.1f), r(480000, 0.1f);
    const auto t0 = std::chrono::steady_clock::now();
    run(fb, l, r, 512);
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("10 s stereo 24 dB/oct processed in %.3f s (%.1f%% of one core)\n", sec, sec / 10.0 * 100);
}

int main()
{
    testFrequencies();
    testMixZeroIsDry();
    testSelectivity();
    testFlatNoise();
    testPan();
    testStability();
    testMono();
    testPerformance();
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nall tests passed\n", failures);
    return failures ? 1 : 0;
}

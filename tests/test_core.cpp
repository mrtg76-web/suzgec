// Süzgeç çekirdek testleri (JUCE gerektirmez)
#include "FilterBank.h"

#include <algorithm>
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
            p.tripOn = u(rng) > 0.3f;
            p.tripRateHz = 0.01 * std::pow(200000.0, (double) u(rng));
            p.tripIntensity = u(rng);
            p.tripShape = u(rng);
            p.tripReverse = u(rng) > 0.5f;
            p.tripToPan = u(rng);
            p.ringOn = u(rng) > 0.5f;
            p.ringMask = ((std::uint64_t) rng() << 16) & ((1ull << 48) - 1) & (u(rng) > 0.5f ? 0x0F0F0F0F0F0Full : 0x8001ull);
            p.downsample = u(rng) > 0.7f;
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
    p.tripOn = true;
    p.tripToPan = 0.5f;
    p.tripShape = 0.7f;
    p.ringOn = true;
    p.ringMask = 0xF0F0;
    fb.setParams(p);
    std::vector<float> l(480000, 0.1f), r(480000, 0.1f);
    const auto t0 = std::chrono::steady_clock::now();
    run(fb, l, r, 512);
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("10 s stereo 24 dB/oct processed in %.3f s (%.1f%% of one core)\n", sec, sec / 10.0 * 100);
}

static void testSyncDivisions()
{
    const auto& d = syncDivisions();
    CHECK(d.front().beats == 256.0, "first = 64 bars");
    CHECK(std::fabs(d.back().beats - 0.0625 * 2.0 / 3.0) < 1e-12, "last = 1/64T");
    CHECK(d[(size_t) defaultSyncDivision()].beats == 4.0, "default = 1 bar");
    std::printf("%zu sync divisions, default '%s', first '%s', last '%s'\n", d.size(),
                d[(size_t) defaultSyncDivision()].name.c_str(), d.front().name.c_str(), d.back().name.c_str());
}

static void testTripOffsets()
{
    // sıralı: artan; tek/çift: 0 / 0.5; karışık: permütasyon (hepsi farklı)
    CHECK(FilterBank::tripOffset(10, 0.f) > FilterBank::tripOffset(9, 0.f), "sequential");
    CHECK(FilterBank::tripOffset(3, 0.5f) == 0.5f && FilterBank::tripOffset(4, 0.5f) == 0.f, "alternating");
    std::vector<float> v;
    for (int b = 0; b < kNumBands; ++b) v.push_back(FilterBank::tripOffset(b, 1.f));
    std::sort(v.begin(), v.end());
    CHECK(std::adjacent_find(v.begin(), v.end()) == v.end(), "shuffle unique");
}

static std::vector<float> runTrip(Params p, double hz, int n)
{
    FilterBank fb;
    fb.prepare(kFs);
    fb.setParams(p);
    fb.reset();
    auto l = sine(hz, 0.1, n), r = sine(hz, 0.1, n);
    run(fb, l, r, 128);
    return l;
}

static void testTrip()
{
    const int band = 20;
    const double f = bandFrequency(band);
    Params p = flat(0.f);
    p.level[(size_t) band] = 1.f;

    auto off = runTrip(p, f, 96000);
    Params q = p;
    q.tripOn = true;
    q.tripIntensity = 0.f;
    q.tripRateHz = 3.0;
    auto zero = runTrip(q, f, 96000);
    double err = 0;
    for (size_t i = 0; i < off.size(); ++i) err = std::max(err, (double) std::fabs(off[i] - zero[i]));
    CHECK(err < 1e-5, "intensity 0 == off (err %g)", err);

    // %100 yoğunluk, 2 Hz: 50 ms'lik pencerelerde seviye neredeyse 0 ile tam arasında dalgalanmalı
    q.tripIntensity = 1.f;
    q.tripRateHz = 2.0;
    auto mod = runTrip(q, f, 96000);
    double lo = 1e9, hi = 0;
    for (size_t w = 24000; w + 2400 <= mod.size(); w += 1200) {
        double s = 0;
        for (size_t i = w; i < w + 2400; ++i) s += (double) mod[i] * mod[i];
        lo = std::min(lo, std::sqrt(s / 2400));
        hi = std::max(hi, std::sqrt(s / 2400));
    }
    std::printf("trip 100%% @2Hz: window rms %.4f .. %.4f\n", lo, hi);
    CHECK(hi > 0.03 && lo < hi * 0.15, "trip modulates level");

    // To Pan, yoğunluk 0: sol/sağ dengesi zamanla değişmeli
    FilterBank fb;
    fb.prepare(kFs);
    Params pp = flat(1.f);
    pp.tripOn = true;
    pp.tripIntensity = 0.f;
    pp.tripToPan = 1.f;
    pp.tripRateHz = 1.0;
    fb.setParams(pp);
    fb.reset();
    auto l = sine(440, 0.1, 48000), r = sine(440, 0.1, 48000);
    run(fb, l, r, 128);
    double maxDiff = 0;
    for (size_t w = 4800; w + 2400 <= l.size(); w += 2400) {
        double a = 0, b2 = 0;
        for (size_t i = w; i < w + 2400; ++i) {
            a += (double) l[i] * l[i];
            b2 += (double) r[i] * r[i];
        }
        maxDiff = std::max(maxDiff, std::fabs(std::sqrt(a) - std::sqrt(b2)) / (std::sqrt(a) + std::sqrt(b2) + 1e-9));
    }
    std::printf("to pan: max L/R imbalance %.2f\n", maxDiff);
    CHECK(maxDiff > 0.3, "to pan moves stereo image");

    // senkron kilit: aynı host konumu -> aynı çıktı
    Params s1 = p;
    s1.tripOn = true;
    s1.tripIntensity = 1.f;
    s1.tripRateHz = 4.0;
    s1.tripLocked = true;
    s1.tripPosition = 0.25;
    FilterBank a, b;
    a.prepare(kFs);
    b.prepare(kFs);
    a.setParams(s1);
    b.setParams(s1);
    auto x1 = sine(f, 0.1, 256), x2 = x1;
    a.process(x1.data(), nullptr, 256);
    b.process(x2.data(), nullptr, 256);
    CHECK(x1 == x2, "locked phase deterministic");
}

static void testRingMod()
{
    // maske boşsa ring mod kapalıyla aynı
    Params p = flat(0.8f);
    auto base = runTrip(p, 300, 24000);
    Params q = p;
    q.ringOn = true;
    q.ringMask = 0;
    auto same = runTrip(q, 300, 24000);
    CHECK(base == same, "empty mask = unmodulated");

    // seçili bantlar: çıktı değişmeli, sonlu ve makul seviyede kalmalı
    q.ringMask = (1ull << 5) | (1ull << 17) | (1ull << 30);
    std::mt19937 rng(5);
    std::normal_distribution<float> nd(0.f, 0.1f);
    FilterBank fb;
    fb.prepare(kFs);
    fb.setParams(q);
    fb.reset();
    std::vector<float> l(48000), r(48000);
    for (auto& x : l) x = nd(rng);
    for (auto& x : r) x = nd(rng);
    const double in = rms(l);
    run(fb, l, r);
    bool finite = true;
    for (float x : l) finite &= std::isfinite(x);
    const double db = 20 * std::log10(rms(l, 24000) / in);
    std::printf("ring mod 3 bands on noise: %+.1f dB\n", db);
    CHECK(finite, "ring finite");
    CHECK(db > -20 && db < 12, "ring level reasonable");
}

static void testDownsample()
{
    Params p = flat(0.8f);
    p.downsample = true;
    FilterBank fb;
    fb.prepare(kFs);
    fb.setParams(p);
    fb.reset();
    auto l = sine(1000, 0.2, 4800), r = sine(700, 0.2, 4800);
    run(fb, l, r, 100); // blok sınırları 4'e bölünmüyor: sayaç bloklar arasında sürmeli
    bool held = true;
    for (size_t i = 0; i + 3 < l.size(); i += 4) held &= (std::fabs(l[i] - l[i + 1]) < 1e-6f && std::fabs(l[i] - l[i + 3]) < 1e-6f);
    CHECK(held, "downsample holds 4 samples");
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
    testSyncDivisions();
    testTripOffsets();
    testTrip();
    testRingMod();
    testDownsample();
    testPerformance();
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nall tests passed\n", failures);
    return failures ? 1 : 0;
}

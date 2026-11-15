// Arayüz ekran görüntüsü aracı (CI'da arayüzü kontrol etmek için)
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>

namespace {
void feed(SuzgecProcessor& proc, int blocks)
{
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    juce::Random rnd(3);
    for (int i = 0; i < blocks; ++i) {
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < 512; ++s) buf.setSample(ch, s, (rnd.nextFloat() * 2.f - 1.f) * 0.25f);
        proc.processBlock(buf, midi);
    }
}

void shot(SuzgecProcessor& proc, const juce::File& dir, const juce::String& name)
{
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(960, 720);
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
    auto file = dir.getChildFile(name + ".png");
    file.deleteFile();
    juce::FileOutputStream os(file);
    juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("wrote %s\n", file.getFullPathName().toRawUTF8());
}

void set(SuzgecProcessor& proc, const char* id, float normalised)
{
    proc.apvts.getParameter(id)->setValueNotifyingHost(normalised);
}
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir = argc > 1 ? juce::File(juce::String(argv[1])) : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    SuzgecProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    // 1) Trip + Ring Mod, birkaç kilit
    for (int b = 0; b < sz::kNumBands; ++b) {
        const float v = 0.5f + 0.45f * std::sin((float) b * 0.37f) * std::cos((float) b * 0.11f);
        proc.bandParam(b)->setValueNotifyingHost(v);
    }
    set(proc, "resonance", 0.55f);
    set(proc, "panOdd", 0.2f);
    set(proc, "panEven", 0.8f);
    set(proc, "tripOn", 1.f);
    set(proc, "tripIntensity", 0.7f);
    set(proc, "tripShape", 0.3f);
    set(proc, "tripToPan", 0.4f);
    set(proc, "ringOn", 1.f);
    proc.setRingMask((1ull << 7) | (1ull << 19) | (1ull << 31) | (1ull << 40));
    proc.setLocked("resonance", true);
    proc.setLocked("tripOn", true);
    proc.setPalette(0);
    proc.setCrt(true);
    feed(proc, 400);
    shot(proc, outDir, "suzgec_0");

    // 2) klavye modu, basılı notalar
    proc.lockAll(false);
    set(proc, "ringOn", 0.f);
    set(proc, "tripOn", 0.f);
    set(proc, "kbdMode", 1.f);
    set(proc, "resonance", 0.85f);
    feed(proc, 4);
    for (int n : { 45, 57, 64, 69, 76 }) proc.keyboardState.noteOn(1, n, 0.9f);
    feed(proc, 100);
    proc.setPalette(1);
    shot(proc, outDir, "suzgec_1");
    for (int n : { 45, 57, 64, 69, 76 }) proc.keyboardState.noteOff(1, n, 0.f);

    // 3) fabrika preseti, CRT kapalı
    proc.presets.loadFactory(1);
    proc.setOversampling(2); // 4x yolu da çalışsın
    feed(proc, 200);
    {
        juce::AudioBuffer<float> buf(2, 512);
        juce::MidiBuffer midi;
        double sum = 0;
        bool finite = true;
        for (int i = 0; i < 50; ++i) {
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < 512; ++s) buf.setSample(ch, s, std::sin((float) (i * 512 + s) * 0.05f) * 0.3f);
            proc.processBlock(buf, midi);
            for (int s = 0; s < 512; ++s) {
                finite &= std::isfinite(buf.getSample(0, s));
                sum += buf.getSample(0, s) * buf.getSample(0, s);
            }
        }
        std::printf("oversampling 4x: latency %d samples, rms %.4f, finite %d\n", proc.getLatencySamples(),
                    std::sqrt(sum / (50 * 512)), finite ? 1 : 0);
    }
    proc.setPalette(2);
    proc.setCrt(false);
    shot(proc, outDir, "suzgec_2");
    return 0;
}

// Arayüz ekran görüntüsü aracı (CI'da arayüzü kontrol etmek için)
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir = argc > 1 ? juce::File(juce::String(argv[1])) : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    SuzgecProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    // örnek bir spektral şekil
    for (int b = 0; b < sz::kNumBands; ++b) {
        const float v = 0.5f + 0.45f * std::sin((float) b * 0.37f) * std::cos((float) b * 0.11f);
        proc.bandParam(b)->setValueNotifyingHost(v);
    }
    proc.apvts.getParameter("resonance")->setValueNotifyingHost(0.55f);
    proc.apvts.getParameter("panOdd")->setValueNotifyingHost(0.2f);
    proc.apvts.getParameter("panEven")->setValueNotifyingHost(0.8f);

    // sayaçlar dolsun diye gürültü besle
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    juce::Random rnd(3);
    for (int i = 0; i < 400; ++i) {
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < 512; ++s) buf.setSample(ch, s, (rnd.nextFloat() * 2.f - 1.f) * 0.25f);
        proc.processBlock(buf, midi);
    }

    const int palettes[] = { 0, 1, 2 };
    for (int pal : palettes) {
        proc.setPalette(pal);
        proc.setCrt(pal != 2);
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        ed->setSize(960, 600);
        auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
        auto file = outDir.getChildFile("suzgec_" + juce::String(pal) + ".png");
        file.deleteFile();
        juce::FileOutputStream os(file);
        juce::PNGImageFormat().writeImageToStream(img, os);
        std::printf("wrote %s\n", file.getFullPathName().toRawUTF8());
    }
    return 0;
}

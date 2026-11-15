#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const juce::Identifier kPalette("palette"), kCrt("crt"), kUiWidth("uiWidth");
constexpr const char* kPresetPrefix = "SUZGEC1:";

juce::String dbText(float v, int) { return (v > 0.05f ? "+" : "") + juce::String(v, 1) + " dB"; }
juce::String pctText(float v, int) { return juce::String(juce::roundToInt(v)) + "%"; }
juce::String panText(float v, int)
{
    const int i = juce::roundToInt(v);
    if (i == 0) return "C";
    return (i < 0 ? "L " : "R ") + juce::String(std::abs(i));
}
} // namespace

juce::String SuzgecProcessor::bandParamId(int band)
{
    return "band" + juce::String(band + 1).paddedLeft('0', 2);
}

juce::AudioProcessorValueTreeState::ParameterLayout SuzgecProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto fl = [&](const char* id, const char* name, float lo, float hi, float def, auto toText, const char* unit) {
        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, String(CharPointer_UTF8(name)), NormalisableRange<float>(lo, hi, 0.01f), def,
            AudioParameterFloatAttributes().withLabel(unit).withStringFromValueFunction(toText)));
    };

    fl("input", "Input", -24.f, 24.f, 0.f, dbText, "dB");
    fl("output", "Output", -24.f, 24.f, 0.f, dbText, "dB");
    fl("mix", "Mix", 0.f, 100.f, 100.f, pctText, "%");
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "softclip", 1 }, "Soft Clip", false));
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "shift", 1 }, "Shift",
                                                      StringArray { "-1 ST", "OFF", "+1 ST" }, 1));
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "slope", 1 }, "Slope",
                                                      StringArray { "12 dB/oct", "24 dB/oct" }, 0));
    fl("resonance", "Resonance", 0.f, 100.f, 30.f, pctText, "%");
    fl("panEven", "Pan Even", -100.f, 100.f, 0.f, panText, "");
    fl("panOdd", "Pan Odd", -100.f, 100.f, 0.f, panText, "");

    for (int b = 0; b < sz::kNumBands; ++b) {
        const auto hz = sz::bandFrequency(b);
        const String name = "Band " + String(b + 1) + " (" + String(hz, hz < 1000 ? 0 : 0) + " Hz)";
        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID { bandParamId(b), 1 }, name, NormalisableRange<float>(0.f, 100.f, 0.01f), 75.f,
            AudioParameterFloatAttributes().withLabel("%").withStringFromValueFunction(pctText)));
    }
    return layout;
}

SuzgecProcessor::SuzgecProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SUZGEC", createLayout())
{
    pInput = apvts.getRawParameterValue("input");
    pOutput = apvts.getRawParameterValue("output");
    pMix = apvts.getRawParameterValue("mix");
    pSoftClip = apvts.getRawParameterValue("softclip");
    pShift = apvts.getRawParameterValue("shift");
    pSlope = apvts.getRawParameterValue("slope");
    pResonance = apvts.getRawParameterValue("resonance");
    pPanEven = apvts.getRawParameterValue("panEven");
    pPanOdd = apvts.getRawParameterValue("panOdd");
    for (int b = 0; b < sz::kNumBands; ++b) {
        pBands[(size_t) b] = apvts.getRawParameterValue(bandParamId(b));
        bandParams[(size_t) b] = apvts.getParameter(bandParamId(b));
    }
    if (!apvts.state.hasProperty(kPalette)) apvts.state.setProperty(kPalette, 0, nullptr);
    if (!apvts.state.hasProperty(kCrt)) apvts.state.setProperty(kCrt, true, nullptr);
    if (!apvts.state.hasProperty(kUiWidth)) apvts.state.setProperty(kUiWidth, 960, nullptr);
}

bool SuzgecProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

sz::Params SuzgecProcessor::readParams() const
{
    sz::Params p;
    p.inputDb = pInput->load();
    p.outputDb = pOutput->load();
    p.mix = pMix->load() * 0.01f;
    p.softClip = pSoftClip->load() > 0.5f;
    p.shift = juce::roundToInt(pShift->load()) - 1;
    p.slope24 = pSlope->load() > 0.5f;
    p.resonance = pResonance->load() * 0.01f;
    p.panEven = pPanEven->load() * 0.01f;
    p.panOdd = pPanOdd->load() * 0.01f;
    for (int b = 0; b < sz::kNumBands; ++b) p.level[(size_t) b] = pBands[(size_t) b]->load() * 0.01f;
    return p;
}

void SuzgecProcessor::prepareToPlay(double sampleRate, int)
{
    bank.setParams(readParams());
    bank.prepare(sampleRate);
}

void SuzgecProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear(); // MIDI 3. fazda

    const int numIn = getTotalNumInputChannels();
    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, buffer.getNumSamples());

    bank.setParams(readParams());

    if (buffer.getNumChannels() == 0) return;
    float* l = buffer.getWritePointer(0);
    float* r = buffer.getNumChannels() > 1 && numIn > 1 ? buffer.getWritePointer(1) : nullptr;
    bank.process(l, r, buffer.getNumSamples());
}

// ---------------------------------------------------------------- bantlar
float SuzgecProcessor::getBandLevel(int band) const
{
    return pBands[(size_t) band]->load() * 0.01f;
}

void SuzgecProcessor::flipBands()
{
    std::array<float, sz::kNumBands> now {};
    for (int b = 0; b < sz::kNumBands; ++b) now[(size_t) b] = bandParams[(size_t) b]->getValue();
    for (int b = 0; b < sz::kNumBands; ++b) {
        auto* prm = bandParams[(size_t) b];
        prm->beginChangeGesture();
        prm->setValueNotifyingHost(1.f - now[(size_t) b]);
        prm->endChangeGesture();
    }
}

// ---------------------------------------------------------------- arayüz ayarları
int SuzgecProcessor::getPalette() const { return (int) apvts.state.getProperty(kPalette, 0); }
void SuzgecProcessor::setPalette(int i) { apvts.state.setProperty(kPalette, i, nullptr); }
bool SuzgecProcessor::getCrt() const { return (bool) apvts.state.getProperty(kCrt, true); }
void SuzgecProcessor::setCrt(bool on) { apvts.state.setProperty(kCrt, on, nullptr); }
int SuzgecProcessor::getUiWidth() const { return (int) apvts.state.getProperty(kUiWidth, 960); }
void SuzgecProcessor::setUiWidth(int w) { apvts.state.setProperty(kUiWidth, w, nullptr); }

// ---------------------------------------------------------------- durum
void SuzgecProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void SuzgecProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::String SuzgecProcessor::presetToString()
{
    auto state = apvts.copyState();
    state.removeProperty(kPalette, nullptr);
    state.removeProperty(kCrt, nullptr);
    state.removeProperty(kUiWidth, nullptr);
    juce::MemoryOutputStream mo;
    state.writeToStream(mo);
    return kPresetPrefix + mo.getMemoryBlock().toBase64Encoding();
}

bool SuzgecProcessor::presetFromString(const juce::String& text)
{
    auto t = text.trim();
    if (!t.startsWith(kPresetPrefix)) return false;
    juce::MemoryBlock mb;
    if (!mb.fromBase64Encoding(t.fromFirstOccurrenceOf(kPresetPrefix, false, false))) return false;
    auto tree = juce::ValueTree::readFromData(mb.getData(), mb.getSize());
    if (!tree.isValid() || tree.getType() != apvts.state.getType()) return false;

    // arayüz ayarlarını koru, sadece parametreleri değiştir
    for (int i = 0; i < tree.getNumChildren(); ++i) {
        auto child = tree.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (auto* prm = apvts.getParameter(id)) {
            const float v = (float) child.getProperty("value");
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(v));
            prm->endChangeGesture();
        }
    }
    return true;
}

juce::AudioProcessorEditor* SuzgecProcessor::createEditor() { return new SuzgecEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SuzgecProcessor(); }

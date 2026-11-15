#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Text.h"

namespace {
const juce::Identifier kPalette("palette"), kCrt("crt"), kUiWidth("uiWidth"), kRingMask("ringMask"), kLocks("locks"),
    kOversampling("oversampling"), kBrightness("brightness"), kContrast("contrast"), kPresetName("presetName");
constexpr const char* kPresetPrefix = "SUZGEC1:";

juce::String dbText(float v, int)
{
    if (std::abs(v) < 0.05f) return "0.0 dB";
    return (v > 0.f ? "+" : "") + juce::String(v, 1) + " dB";
}
juce::String pctText(float v, int) { return juce::String(juce::roundToInt(v)) + "%"; }
juce::String hzText(float v, int)
{
    if (v >= 1000.f) return juce::String(v / 1000.f, 2) + " kHz";
    if (v >= 100.f) return juce::String(v, 0) + " Hz";
    if (v >= 10.f) return juce::String(v, 1) + " Hz";
    return juce::String(v, 2) + " Hz";
}
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

    // --- Faz 2: Trip, Ring Mod, Downsample
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "tripOn", 1 }, "Trip On", false));
    {
        constexpr float lo = 0.01f, hi = 2000.f;
        NormalisableRange<float> logRange(
            lo, hi, [](float a, float b, float t) { return a * std::pow(b / a, t); },
            [](float a, float b, float v) { return std::log(v / a) / std::log(b / a); });
        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID { "tripSpeed", 1 }, "Trip Speed", logRange, 0.5f,
            AudioParameterFloatAttributes().withLabel("Hz").withStringFromValueFunction(hzText)));
    }
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "tripSync", 1 }, "Trip Sync", false));
    {
        StringArray names;
        for (const auto& d : sz::syncDivisions()) names.add(String(CharPointer_UTF8(d.name.c_str())));
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "tripDiv", 1 }, "Trip Division", names,
                                                          sz::defaultSyncDivision()));
    }
    fl("tripIntensity", "Trip Intensity", 0.f, 100.f, 50.f, pctText, "%");
    fl("tripShape", "Trip Shape", 0.f, 100.f, 0.f, pctText, "%");
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "tripPhase", 1 }, "Trip Phase (Reverse)", false));
    fl("tripToPan", "Trip To Pan", 0.f, 100.f, 0.f, pctText, "%");
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "ringOn", 1 }, "Ring Mod", false));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "downsample", 1 }, "Downsample", false));

    // --- Faz 3: MIDI klavye modu
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "kbdMode", 1 }, "Keyboard Mode", false));
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
    pTripOn = apvts.getRawParameterValue("tripOn");
    pTripSpeed = apvts.getRawParameterValue("tripSpeed");
    pTripSync = apvts.getRawParameterValue("tripSync");
    pTripDiv = apvts.getRawParameterValue("tripDiv");
    pTripIntensity = apvts.getRawParameterValue("tripIntensity");
    pTripShape = apvts.getRawParameterValue("tripShape");
    pTripPhase = apvts.getRawParameterValue("tripPhase");
    pTripToPan = apvts.getRawParameterValue("tripToPan");
    pRingOn = apvts.getRawParameterValue("ringOn");
    pDownsample = apvts.getRawParameterValue("downsample");
    pKbdMode = apvts.getRawParameterValue("kbdMode");
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

    p.tripOn = pTripOn->load() > 0.5f;
    p.tripRateHz = pTripSpeed->load();
    p.tripIntensity = pTripIntensity->load() * 0.01f;
    p.tripShape = pTripShape->load() * 0.01f;
    p.tripReverse = pTripPhase->load() > 0.5f;
    p.tripToPan = pTripToPan->load() * 0.01f;
    p.ringOn = pRingOn->load() > 0.5f;
    p.ringMask = ringMask.load();
    p.downsample = pDownsample->load() > 0.5f;
    p.keyboardMode = pKbdMode->load() > 0.5f;
    return p;
}

void SuzgecProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate > 0 ? sampleRate : 48000.0;
    maxBlock = juce::jmax(16, samplesPerBlock);
    for (int i = 1; i < (int) oversamplers.size(); ++i) {
        oversamplers[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, (size_t) i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        oversamplers[(size_t) i]->initProcessing((size_t) maxBlock);
    }
    keyboardState.reset();
    osActive = -1;
    activateOversampling(osRequested.load());
}

void SuzgecProcessor::activateOversampling(int index)
{
    index = juce::jlimit(0, 4, index);
    osActive = index;
    bank.setParams(readParams());
    bank.prepare(hostRate * (double) (1 << index));
    int latency = 0;
    if (index > 0 && oversamplers[(size_t) index]) {
        oversamplers[(size_t) index]->reset();
        latency = (int) std::lround(oversamplers[(size_t) index]->getLatencyInSamples());
    }
    setLatencySamples(latency);
}

void SuzgecProcessor::handleMidi(const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        bank.noteOn(m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        bank.noteOff(m.getNoteNumber());
    else if (m.isPitchWheel())
        bank.setAttackSeconds(sz::FilterBank::wheelToSeconds((float) m.getPitchWheelValue() / 16383.f));
    else if (m.isControllerOfType(1))
        bank.setDecaySeconds(sz::FilterBank::wheelToSeconds((float) m.getControllerValue() / 127.f));
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        bank.allNotesOff();
}

void SuzgecProcessor::processChunk(juce::AudioBuffer<float>& buffer, int start, int num, const juce::MidiBuffer& midi)
{
    const int numCh = juce::jmin(buffer.getNumChannels(), getTotalNumInputChannels(), 2);
    if (numCh == 0) return;
    const int factor = 1 << osActive;

    float* l;
    float* r;
    juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) start, (size_t) num);
    juce::dsp::AudioBlock<float> up;
    if (osActive > 0) {
        up = oversamplers[(size_t) osActive]->processSamplesUp(block);
        l = up.getChannelPointer(0);
        r = numCh > 1 ? up.getChannelPointer(1) : nullptr;
    } else {
        l = buffer.getWritePointer(0, start);
        r = numCh > 1 ? buffer.getWritePointer(1, start) : nullptr;
    }

    // MIDI olaylarını örnek doğruluğunda uygula: olaylar arasında parça parça işle
    int pos = 0;
    for (const auto meta : midi) {
        const int at = meta.samplePosition - start;
        if (at < 0 || at >= num) continue;
        if (at > pos) {
            bank.process(l + pos * factor, r ? r + pos * factor : nullptr, (at - pos) * factor);
            pos = at;
        }
        handleMidi(meta.getMessage());
    }
    if (pos < num) bank.process(l + pos * factor, r ? r + pos * factor : nullptr, (num - pos) * factor);

    if (osActive > 0) oversamplers[(size_t) osActive]->processSamplesDown(block);
}

void SuzgecProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer(midi, 0, n, true);
    if (osRequested.load() != osActive) activateOversampling(osRequested.load());

    const int numIn = getTotalNumInputChannels();
    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, buffer.getNumSamples());

    auto p = readParams();

    // Trip senkronu: host temposuna ve çalma konumuna kilitlen
    if (p.tripOn && pTripSync->load() > 0.5f) {
        const auto& divs = sz::syncDivisions();
        const int di = juce::jlimit(0, (int) divs.size() - 1, juce::roundToInt(pTripDiv->load()));
        const double beats = divs[(size_t) di].beats;
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition()) {
                if (auto b = pos->getBpm()) bpm = *b;
                if (pos->getIsPlaying())
                    if (auto ppq = pos->getPpqPosition()) {
                        p.tripLocked = true;
                        p.tripPosition = (*ppq / beats) * (p.tripReverse ? -1.0 : 1.0);
                    }
            }
        p.tripRateHz = bpm / 60.0 / beats;
    }
    tripRateShown.store((float) p.tripRateHz, std::memory_order_relaxed);
    p.downsampleHold = 4 * (1 << osActive); // Downsample her zaman host hızının 1/4'ü
    bank.setParams(p);

    // host beklenenden büyük blok gönderirse parçala
    for (int start = 0; start < n; start += maxBlock) processChunk(buffer, start, juce::jmin(maxBlock, n - start), midi);
    midi.clear();
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

// ---------------------------------------------------------------- Ring Mod seçimi
void SuzgecProcessor::setRingMask(std::uint64_t mask)
{
    mask &= (1ull << sz::kNumBands) - 1;
    ringMask.store(mask);
    apvts.state.setProperty(kRingMask, juce::String::toHexString((juce::int64) mask), nullptr);
}

void SuzgecProcessor::toggleRingBand(int band)
{
    setRingMask(ringMask.load() ^ (1ull << band));
}

void SuzgecProcessor::randomOrClearRing()
{
    if (ringMask.load() != 0) {
        setRingMask(0);
        return;
    }
    auto& rnd = juce::Random::getSystemRandom();
    const int count = 3 + rnd.nextInt(3); // 3..5 bant
    std::uint64_t m = 0;
    for (int picked = 0; picked < count;) {
        const auto bit = 1ull << rnd.nextInt(sz::kNumBands);
        if (m & bit) continue;
        m |= bit;
        ++picked;
    }
    setRingMask(m);
}

void SuzgecProcessor::loadRingMaskFromState()
{
    const auto hex = apvts.state.getProperty(kRingMask, "0").toString();
    ringMask.store((std::uint64_t) hex.getHexValue64() & ((1ull << sz::kNumBands) - 1));
}

// ---------------------------------------------------------------- kilitler
juce::StringArray SuzgecProcessor::allParamIds() const
{
    juce::StringArray ids;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) ids.add(rp->getParameterID());
    return ids;
}

void SuzgecProcessor::setLocked(const juce::String& id, bool on)
{
    if (on)
        locked.insert(id);
    else
        locked.erase(id);
    storeLocks();
}

void SuzgecProcessor::setLockedMany(const juce::StringArray& ids, bool on)
{
    for (auto& id : ids) {
        if (on)
            locked.insert(id);
        else
            locked.erase(id);
    }
    storeLocks();
}

void SuzgecProcessor::lockAll(bool on) { setLockedMany(allParamIds(), on); }

void SuzgecProcessor::storeLocks()
{
    juce::StringArray a;
    for (auto& id : locked) a.add(id);
    apvts.state.setProperty(kLocks, a.joinIntoString(","), nullptr);
}

void SuzgecProcessor::loadLocksFromState()
{
    locked.clear();
    juce::StringArray a;
    a.addTokens(apvts.state.getProperty(kLocks, "").toString(), ",", "");
    for (auto& id : a)
        if (id.isNotEmpty()) locked.insert(id);
}

// ---------------------------------------------------------------- ayarlar
int SuzgecProcessor::getOversampling() const { return osRequested.load(); }
void SuzgecProcessor::setOversampling(int i)
{
    i = juce::jlimit(0, 4, i);
    osRequested.store(i);
    apvts.state.setProperty(kOversampling, i, nullptr);
}
float SuzgecProcessor::getBrightness() const { return (float) apvts.state.getProperty(kBrightness, 1.0); }
void SuzgecProcessor::setBrightness(float v) { apvts.state.setProperty(kBrightness, v, nullptr); }
float SuzgecProcessor::getContrast() const { return (float) apvts.state.getProperty(kContrast, 1.0); }
void SuzgecProcessor::setContrast(float v) { apvts.state.setProperty(kContrast, v, nullptr); }

void SuzgecProcessor::setCurrentProgram(int i)
{
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr
        && juce::MessageManager::getInstance()->isThisTheMessageThread())
        presets.loadFactory(i);
    else
        juce::MessageManager::callAsync([this, i] { presets.loadFactory(i); });
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
        if (xml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            loadRingMaskFromState();
            loadLocksFromState();
            osRequested.store(juce::jlimit(0, 4, (int) apvts.state.getProperty(kOversampling, 0)));
        }
}

juce::String SuzgecProcessor::presetToString()
{
    auto state = apvts.copyState();
    state.removeProperty(kPalette, nullptr);
    state.removeProperty(kCrt, nullptr);
    state.removeProperty(kUiWidth, nullptr);
    for (auto id : { kLocks, kOversampling, kBrightness, kContrast, kPresetName }) state.removeProperty(id, nullptr);
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
    PresetManager::Values v;
    std::uint64_t mask = 0;
    if (!PresetManager::treeToValues(tree, v, mask)) return false;
    presets.apply(v, &mask, TR("Panodan"));
    return true;
}

juce::AudioProcessorEditor* SuzgecProcessor::createEditor() { return new SuzgecEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SuzgecProcessor(); }

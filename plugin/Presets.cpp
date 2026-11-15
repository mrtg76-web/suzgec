#include "Presets.h"
#include "PluginProcessor.h"
#include "Text.h"

#include <cmath>

namespace {
const juce::Identifier kPresetName("presetName"), kRingMask("ringMask");

juce::String bandId(int b) { return SuzgecProcessor::bandParamId(b); }

// band şekli yardımcıları (değerler %)
void bands(PresetManager::Values& v, const std::function<float(int)>& f)
{
    for (int b = 0; b < sz::kNumBands; ++b) v[bandId(b)] = juce::jlimit(0.f, 100.f, f(b));
}

int divIndex(const char* name)
{
    const auto& d = sz::syncDivisions();
    for (size_t i = 0; i < d.size(); ++i)
        if (juce::String(juce::CharPointer_UTF8(d[i].name.c_str())) == TR(name)) return (int) i;
    return sz::defaultSyncDivision();
}

std::uint64_t maskOf(std::initializer_list<int> bandNumbers) // 1 tabanlı bant numaraları
{
    std::uint64_t m = 0;
    for (int n : bandNumbers) m |= 1ull << (n - 1);
    return m;
}
} // namespace

PresetManager::PresetManager(SuzgecProcessor& p) : proc(p)
{
    using V = Values;
    using M = std::uint64_t;
    factory = {
        { "Başlangıç", [](V&, M&) {} },
        { "Cam Çanlar",
          [](V& v, M&) {
              v["resonance"] = 86.f;
              v["slope"] = 1.f;
              bands(v, [](int b) { return (b % 4 == 2 && b > 14) ? 70.f + b * 0.6f : 0.f; });
              v["input"] = 4.f;
          } },
        { "Derin Gong",
          [](V& v, M&) {
              v["resonance"] = 95.f;
              v["slope"] = 1.f;
              bands(v, [](int b) { return (b == 1 || b == 4 || b == 7 || b == 11) ? 95.f : (b < 16 ? 8.f : 0.f); });
              v["input"] = 6.f;
              v["softclip"] = 1.f;
          } },
        { "Metal Tel",
          [](V& v, M& m) {
              v["resonance"] = 62.f;
              bands(v, [](int b) { return 55.f + 35.f * std::sin(b * 0.9f); });
              v["ringOn"] = 1.f;
              m = maskOf({ 9, 22, 31, 40 });
          } },
        { "Geniş Tek/Çift",
          [](V& v, M&) {
              v["resonance"] = 40.f;
              v["panOdd"] = -100.f;
              v["panEven"] = 100.f;
              bands(v, [](int) { return 80.f; });
          } },
        { "Nefes Alan Spektrum",
          [](V& v, M&) {
              v["resonance"] = 55.f;
              bands(v, [](int b) { return 50.f + 40.f * std::sin(b * 0.26f); });
              v["tripOn"] = 1.f;
              v["tripSpeed"] = 0.15f;
              v["tripIntensity"] = 80.f;
              v["tripShape"] = 0.f;
              v["tripToPan"] = 30.f;
          } },
        { "Titreşim",
          [](V& v, M&) {
              v["resonance"] = 45.f;
              bands(v, [](int) { return 85.f; });
              v["tripOn"] = 1.f;
              v["tripSync"] = 1.f;
              v["tripDiv"] = (float) divIndex("1/16");
              v["tripIntensity"] = 100.f;
              v["tripShape"] = 50.f;
          } },
        { "Karışık Damlalar",
          [](V& v, M&) {
              v["resonance"] = 82.f;
              v["slope"] = 1.f;
              bands(v, [](int b) { return b > 10 && b < 44 ? 90.f : 20.f; });
              v["tripOn"] = 1.f;
              v["tripSync"] = 1.f;
              v["tripDiv"] = (float) divIndex("1/8T");
              v["tripIntensity"] = 100.f;
              v["tripShape"] = 100.f;
              v["tripToPan"] = 60.f;
          } },
        { "Lo-Fi Radyo",
          [](V& v, M&) {
              v["resonance"] = 25.f;
              bands(v, [](int b) { return (b >= 12 && b <= 34) ? 95.f - std::abs(b - 23) * 2.f : 0.f; });
              v["downsample"] = 1.f;
              v["softclip"] = 1.f;
              v["input"] = 9.f;
              v["output"] = -3.f;
          } },
        { "Ters Tarak",
          [](V& v, M&) {
              v["resonance"] = 50.f;
              v["slope"] = 1.f;
              bands(v, [](int b) { return (b & 1) ? 100.f : 0.f; });
          } },
        { "Ses Hızında AM",
          [](V& v, M&) {
              v["resonance"] = 35.f;
              bands(v, [](int) { return 80.f; });
              v["tripOn"] = 1.f;
              v["tripSpeed"] = 220.f;
              v["tripIntensity"] = 70.f;
              v["tripShape"] = 30.f;
          } },
        { "Gürültü Klavyesi",
          [](V& v, M&) {
              v["resonance"] = 90.f;
              v["slope"] = 1.f;
              v["kbdMode"] = 1.f;
              bands(v, [](int) { return 75.f; });
          } },
    };
}

juce::String PresetManager::factoryName(int i) const
{
    if (i < 0 || i >= (int) factory.size()) return {};
    return TR(factory[(size_t) i].name);
}

juce::File PresetManager::userFolder()
{
    return juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Suzgec").getChildFile("Presets");
}

std::vector<PresetManager::Entry> PresetManager::allEntries() const
{
    std::vector<Entry> out;
    for (int i = 0; i < (int) factory.size(); ++i) out.push_back({ factoryName(i), true, i, {} });
    auto files = userFolder().findChildFiles(juce::File::findFiles, false, "*.szpreset");
    files.sort();
    for (auto& f : files) out.push_back({ f.getFileNameWithoutExtension(), false, -1, f });
    return out;
}

PresetManager::Values PresetManager::defaults() const
{
    Values v;
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            v[rp->getParameterID()] = rp->convertFrom0to1(rp->getDefaultValue());
    return v;
}

juce::String PresetManager::currentName() const
{
    return proc.apvts.state.getProperty(kPresetName, TR("Başlangıç")).toString();
}

void PresetManager::setCurrentName(const juce::String& n) { proc.apvts.state.setProperty(kPresetName, n, nullptr); }

void PresetManager::apply(const Values& values, const std::uint64_t* ringMask, const juce::String& name)
{
    for (const auto& [id, value] : values) {
        if (proc.isLocked(id)) continue;
        if (auto* prm = proc.apvts.getParameter(id)) {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(value));
            prm->endChangeGesture();
        }
    }
    if (ringMask && !proc.isLocked("ringOn")) proc.setRingMask(*ringMask);
    setCurrentName(name);
}

void PresetManager::loadFactory(int index)
{
    if (index < 0 || index >= (int) factory.size()) return;
    auto v = defaults();
    std::uint64_t mask = 0;
    factory[(size_t) index].build(v, mask);
    lastFactory = index;
    apply(v, &mask, factoryName(index));
}

bool PresetManager::treeToValues(const juce::ValueTree& tree, Values& out, std::uint64_t& ringMask)
{
    if (!tree.isValid()) return false;
    for (int i = 0; i < tree.getNumChildren(); ++i) {
        auto child = tree.getChild(i);
        if (child.hasProperty("id") && child.hasProperty("value"))
            out[child.getProperty("id").toString()] = (float) child.getProperty("value");
    }
    ringMask = (std::uint64_t) tree.getProperty(kRingMask, "0").toString().getHexValue64();
    return !out.empty();
}

bool PresetManager::loadFile(const juce::File& f)
{
    auto xml = juce::XmlDocument::parse(f);
    if (!xml) return false;
    auto tree = juce::ValueTree::fromXml(*xml);
    Values v;
    std::uint64_t mask = 0;
    if (!treeToValues(tree, v, mask)) return false;
    apply(v, &mask, f.getFileNameWithoutExtension());
    return true;
}

bool PresetManager::save(const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName(rawName.trim());
    if (name.isEmpty()) return false;
    auto dir = userFolder();
    if (!dir.createDirectory()) return false;
    juce::ValueTree t("SUZGEC");
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            juce::ValueTree c("PARAM");
            c.setProperty("id", rp->getParameterID(), nullptr);
            c.setProperty("value", rp->convertFrom0to1(rp->getValue()), nullptr);
            t.appendChild(c, nullptr);
        }
    t.setProperty(kRingMask, juce::String::toHexString((juce::int64) proc.getRingMask()), nullptr);
    auto xml = t.createXml();
    const auto file = dir.getChildFile(name + ".szpreset");
    if (!xml || !xml->writeTo(file)) return false;
    setCurrentName(name);
    return true;
}

void PresetManager::step(int delta)
{
    const auto list = allEntries();
    if (list.empty()) return;
    int cur = -1;
    const auto name = currentName();
    for (int i = 0; i < (int) list.size(); ++i)
        if (list[(size_t) i].name == name) cur = i;
    int next = cur < 0 ? 0 : (cur + delta + (int) list.size()) % (int) list.size();
    const auto& e = list[(size_t) next];
    if (e.factory)
        loadFactory(e.factoryIndex);
    else
        loadFile(e.file);
}

void PresetManager::randomize()
{
    auto& r = juce::Random::getSystemRandom();
    auto uni = [&](float a, float b) { return a + (b - a) * r.nextFloat(); };
    Values v;

    // bant şekli: birkaç farklı üreteçten biri
    const int kind = r.nextInt(5);
    const float f1 = uni(0.08f, 0.6f), f2 = uni(0.02f, 0.25f), ph1 = uni(0.f, 6.28f), ph2 = uni(0.f, 6.28f);
    const int every = 2 + r.nextInt(5);
    std::array<float, sz::kNumBands> peaks {};
    for (int i = 0, n = 3 + r.nextInt(6); i < n; ++i) peaks[(size_t) r.nextInt(sz::kNumBands)] = uni(60.f, 100.f);
    bands(v, [&](int b) {
        switch (kind) {
        case 0: return 50.f + 30.f * std::sin(b * f1 + ph1) + 20.f * std::sin(b * f2 + ph2); // dalgalı
        case 1: return (b % every == 0) ? uni(60.f, 100.f) : uni(0.f, 15.f);               // tarak
        case 2: return peaks[(size_t) b];                                                   // seyrek tepeler
        case 3: return juce::jmap((float) b, 0.f, 47.f, uni(0.f, 100.f), uni(0.f, 100.f)) + uni(-10.f, 10.f); // eğim
        default: return uni(0.f, 100.f);                                                    // gürültü
        }
    });

    const float s = r.nextFloat();
    v["shift"] = s < 0.6f ? 1.f : (s < 0.8f ? 0.f : 2.f);
    v["slope"] = r.nextBool() ? 1.f : 0.f;
    v["resonance"] = uni(10.f, 95.f);
    if (r.nextBool()) {
        v["panEven"] = uni(-100.f, 100.f);
        v["panOdd"] = uni(-100.f, 100.f);
    } else {
        v["panEven"] = 0.f;
        v["panOdd"] = 0.f;
    }

    v["tripOn"] = r.nextBool() ? 1.f : 0.f;
    v["tripSpeed"] = r.nextFloat() < 0.1f ? uni(50.f, 1000.f) : 0.05f * std::pow(400.f, r.nextFloat());
    v["tripSync"] = r.nextFloat() < 0.4f ? 1.f : 0.f;
    v["tripDiv"] = (float) (divIndex("4 ölçü") + r.nextInt(divIndex("1/16") - divIndex("4 ölçü") + 1));
    v["tripIntensity"] = uni(20.f, 100.f);
    v["tripShape"] = uni(0.f, 100.f);
    v["tripPhase"] = r.nextBool() ? 1.f : 0.f;
    v["tripToPan"] = r.nextBool() ? uni(0.f, 80.f) : 0.f;

    const bool ring = r.nextFloat() < 0.25f;
    v["ringOn"] = ring ? 1.f : 0.f;
    std::uint64_t mask = 0;
    if (ring)
        for (int i = 0, n = 3 + r.nextInt(3); i < n; ++i) mask |= 1ull << r.nextInt(sz::kNumBands);
    v["downsample"] = r.nextFloat() < 0.15f ? 1.f : 0.f;

    apply(v, &mask, TR("Rastgele"));
}

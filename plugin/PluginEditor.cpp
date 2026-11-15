#include "PluginEditor.h"
#include "Text.h"

#include <cmath>

// ---------------------------------------------------------------- palet
namespace {
const PaletteEntry kPalettes[] = {
    { "YEŞİL", 0xff3dff7a },   { "TURUNCU", 0xffffa53d }, { "MAVİ", 0xff4fc8ff }, { "KIRMIZI", 0xffff5252 },
    { "SARI", 0xffffe14d },    { "MOR", 0xffc98bff },     { "S/B", 0xffe9ece8 },
};
constexpr float kDefaultLevel = 0.75f;
} // namespace

int paletteCount() { return (int) (sizeof(kPalettes) / sizeof(kPalettes[0])); }
const PaletteEntry& paletteAt(int i)
{
    const int n = paletteCount();
    return kPalettes[((i % n) + n) % n];
}

// ---------------------------------------------------------------- görünüm
CrtLook::CrtLook() { setPhosphor(juce::Colour(kPalettes[0].argb)); }

juce::Font CrtLook::mono(float size, bool bold)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size,
                                        bold ? juce::Font::bold : juce::Font::plain));
}

void CrtLook::setPhosphor(juce::Colour c, float brightness, float contrast)
{
    ph = brightness <= 1.f ? c.withMultipliedBrightness(brightness) : c.brighter(brightness - 1.f);
    // fosfor rengine çok hafif bürünmüş siyah zemin; kontrast düştükçe zemin açılır
    const float lift = juce::jmax(0.f, 1.f - contrast) * 0.35f;
    bg = juce::Colour(0xff050706).interpolatedWith(c, 0.035f / juce::jmax(0.5f, contrast)).brighter(lift);
    pn = juce::Colour(0xff0b0f0d).interpolatedWith(c, 0.06f / juce::jmax(0.5f, contrast)).brighter(lift);
    if (contrast > 1.f) {
        bg = bg.darker(contrast - 1.f);
        pn = pn.darker((contrast - 1.f) * 0.6f);
    }
    setColour(juce::AlertWindow::backgroundColourId, pn);
    setColour(juce::AlertWindow::textColourId, ph);
    setColour(juce::AlertWindow::outlineColourId, ph.withAlpha(0.5f));

    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, ph);
    setColour(juce::Slider::textBoxTextColourId, ph);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, ph.withAlpha(0.3f));
    setColour(juce::TextEditor::textColourId, ph);
    setColour(juce::TextEditor::backgroundColourId, bg);
    setColour(juce::TextEditor::highlightColourId, ph.withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, ph);
    setColour(juce::TextEditor::outlineColourId, ph.withAlpha(0.5f));
    setColour(juce::TextEditor::focusedOutlineColourId, ph);
    setColour(juce::CaretComponent::caretColourId, ph);
    setColour(juce::PopupMenu::backgroundColourId, pn);
    setColour(juce::PopupMenu::textColourId, ph);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ph.withAlpha(0.85f));
    setColour(juce::PopupMenu::highlightedTextColourId, bg);
    setColour(juce::TextButton::textColourOffId, ph);
    setColour(juce::TextButton::textColourOnId, bg);
}

juce::Label* CrtLook::createSliderTextBox(juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setFont(mono(13.f));
    l->setJustificationType(juce::Justification::centred);
    return l;
}

void CrtLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
                               juce::Slider& s)
{
    const auto r = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
    const float radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
    const auto c = r.getCentre();
    const float angle = start + pos * (end - start);
    const bool bipolar = (bool) s.getProperties()["bipolar"];
    const float from = bipolar ? (start + end) * 0.5f : start;

    juce::Path track;
    track.addCentredArc(c.x, c.y, radius - 3.f, radius - 3.f, 0.f, start, end, true);
    g.setColour(ph.withAlpha(0.16f));
    g.strokePath(track, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc(c.x, c.y, radius - 3.f, radius - 3.f, 0.f, juce::jmin(from, angle), juce::jmax(from, angle),
                      true);
    g.setColour(ph.withAlpha(0.25f));
    g.strokePath(val, juce::PathStrokeType(9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ph);
    g.strokePath(val, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // gövde
    const float inner = radius - 10.f;
    g.setColour(pn.brighter(0.08f));
    g.fillEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f);
    g.setColour(ph.withAlpha(0.35f));
    g.drawEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f, 1.f);

    const auto tip = c.getPointOnCircumference(inner - 3.f, angle);
    const auto base = c.getPointOnCircumference(inner * 0.35f, angle);
    g.setColour(ph);
    g.drawLine({ base, tip }, 2.5f);
}

void CrtLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.f);
    const bool on = b.getToggleState();
    if (on) {
        g.setColour(ph.withAlpha(0.25f));
        g.fillRoundedRectangle(r.expanded(1.f), 4.f);
        g.setColour(ph.withAlpha(down ? 0.75f : 0.92f));
        g.fillRoundedRectangle(r, 3.f);
    } else {
        g.setColour(ph.withAlpha(down ? 0.22f : over ? 0.12f : 0.05f));
        g.fillRoundedRectangle(r, 3.f);
        g.setColour(ph.withAlpha(over ? 0.8f : 0.5f));
        g.drawRoundedRectangle(r, 3.f, 1.2f);
    }
}

void CrtLook::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont(mono(juce::jmin(14.f, b.getHeight() * 0.5f)));
    g.setColour(b.getToggleState() ? bg : ph);
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4, 2), juce::Justification::centred, 1);
}

// ---------------------------------------------------------------- bant ekranı
BandDisplay::BandDisplay(SuzgecProcessor& p, CrtLook& l) : proc(p), look(l)
{
    setRepaintsOnMouseActivity(false);
    startTimerHz(30);
}

void BandDisplay::timerCallback()
{
    if (analysis != Analysis::Off) {
        // gelen spektrum: en güçlü banda göre dB, 48 dB'lik aralık 0..1'e
        float mx = 0.f;
        std::array<float, sz::kNumBands> m {};
        for (int b = 0; b < sz::kNumBands; ++b) {
            m[(size_t) b] = proc.bank.inputMeter(b);
            mx = juce::jmax(mx, m[(size_t) b]);
        }
        if (mx > 3e-4f) { // yaklaşık -70 dBFS: kullanılabilir sinyal
            for (int b = 0; b < sz::kNumBands; ++b) {
                const float rel = m[(size_t) b] > 1e-9f ? 20.f * std::log10(m[(size_t) b] / mx) : -120.f;
                const float v = juce::jlimit(0.f, 1.f, 1.f + rel / 48.f);
                auto& a = analysed[(size_t) b];
                a = analysis == Analysis::Waiting ? v : a * 0.7f + v * 0.3f;
            }
            analysis = Analysis::Ready;
        }
    }
    repaint();
}

void BandDisplay::startAnalysis()
{
    analysed.fill(0.f);
    analysis = Analysis::Waiting;
}

void BandDisplay::applyAnalysis(bool inverted)
{
    if (analysis == Analysis::Ready)
        for (int b = 0; b < sz::kNumBands; ++b) {
            auto* prm = proc.bandParam(b);
            const float v = analysed[(size_t) b];
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(inverted ? 1.f - v : v);
            prm->endChangeGesture();
        }
    analysis = Analysis::Off;
}

BandDisplay::~BandDisplay() { endGestures(); }

juce::Rectangle<float> BandDisplay::plotArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft(10.f).withTrimmedRight(10.f).withTrimmedTop(20.f)
        .withTrimmedBottom(48.f);
}

int BandDisplay::bandAt(float x) const
{
    const auto p = plotArea();
    return juce::jlimit(0, sz::kNumBands - 1, (int) std::floor((x - p.getX()) / p.getWidth() * sz::kNumBands));
}

float BandDisplay::levelAt(float y) const
{
    const auto p = plotArea();
    return juce::jlimit(0.f, 1.f, (p.getBottom() - y) / p.getHeight());
}

void BandDisplay::setBand(int b, float level)
{
    auto* prm = proc.bandParam(b);
    if (!touched[(size_t) b]) {
        prm->beginChangeGesture();
        touched[(size_t) b] = true;
    }
    prm->setValueNotifyingHost(level); // aralık 0..100, normalize değer 0..1
}

void BandDisplay::endGestures()
{
    for (int b = 0; b < sz::kNumBands; ++b)
        if (touched[(size_t) b]) {
            proc.bandParam(b)->endChangeGesture();
            touched[(size_t) b] = false;
        }
}

void BandDisplay::drawTo(juce::Point<float> pt)
{
    const auto p = plotArea();
    const float bw = p.getWidth() / sz::kNumBands;
    const int b0 = bandAt(lastPt.x), b1 = bandAt(pt.x);
    if (b0 == b1) {
        setBand(b1, levelAt(pt.y));
    } else {
        const int dir = b1 > b0 ? 1 : -1;
        for (int b = b0; b != b1 + dir; b += dir) {
            const float cx = p.getX() + (b + 0.5f) * bw;
            float t = (cx - lastPt.x) / (pt.x - lastPt.x);
            t = juce::jlimit(0.f, 1.f, t);
            setBand(b, levelAt(lastPt.y + (pt.y - lastPt.y) * t));
        }
    }
    lastPt = pt;
    hoverBand = b1;
}

void BandDisplay::mouseMove(const juce::MouseEvent& e)
{
    hoverBand = plotArea().expanded(0.f, 20.f).contains(e.position) ? bandAt(e.position.x) : -1;
}

void BandDisplay::mouseExit(const juce::MouseEvent&)
{
    if (!dragging) hoverBand = -1;
}

void BandDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) {
        if (onRightClick) onRightClick();
        return;
    }
    // çizmek analizi uygulamadan durdurur
    if (analysis != Analysis::Off) analysis = Analysis::Off;
    dragging = true;
    lastPt = downPt = e.position;
    pendingClick = false;
    if (e.mods.isAltDown() || e.getNumberOfClicks() > 1) {
        setBand(bandAt(e.position.x), kDefaultLevel); // alt/çift tık: varsayılan seviye
        return;
    }
    if (proc.apvts.getRawParameterValue("ringOn")->load() > 0.5f) {
        pendingClick = true; // Ring Mod açıkken: tık = seç, sürükle = çiz
        hoverBand = bandAt(e.position.x);
        return;
    }
    drawTo(e.position);
}

void BandDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isPopupMenu() || !dragging) return;
    if (pendingClick) {
        if (e.position.getDistanceFrom(downPt) < 4.f) return;
        pendingClick = false;
        lastPt = downPt;
    }
    drawTo(e.position);
}

void BandDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (!dragging) return;
    if (pendingClick) {
        proc.toggleRingBand(bandAt(downPt.x));
        pendingClick = false;
    }
    dragging = false;
    endGestures();
    if (!getLocalBounds().contains(e.getPosition())) hoverBand = -1;
}

void BandDisplay::paint(juce::Graphics& g)
{
    const auto ph = look.phosphor();
    const auto bounds = getLocalBounds().toFloat();
    const auto p = plotArea();
    const float bw = p.getWidth() / sz::kNumBands;

    // ekran camı
    g.setColour(look.background().darker(0.4f));
    g.fillRoundedRectangle(bounds, 8.f);
    if (crt) {
        juce::ColourGradient glow(ph.withAlpha(0.07f), bounds.getCentreX(), bounds.getCentreY(),
                                  juce::Colours::transparentBlack, bounds.getX(), bounds.getY(), true);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(bounds, 8.f);
    }

    // ızgara
    g.setColour(ph.withAlpha(0.10f));
    for (int i = 1; i <= 4; ++i) {
        const float y = p.getBottom() - p.getHeight() * i / 4.f;
        for (float x = p.getX(); x < p.getRight(); x += 6.f) g.fillRect(x, y, 2.f, 1.f);
    }
    for (int b = 0; b <= sz::kNumBands; b += 6) {
        const float x = p.getX() + b * bw;
        for (float y = p.getY(); y < p.getBottom(); y += 6.f) g.fillRect(x, y, 1.f, 2.f);
    }

    // bantlar
    const bool tripOn = proc.apvts.getRawParameterValue("tripOn")->load() > 0.5f;
    const bool ringOn = proc.apvts.getRawParameterValue("ringOn")->load() > 0.5f;
    const bool kbd = proc.apvts.getRawParameterValue("kbdMode")->load() > 0.5f;
    const bool analysing = analysis != Analysis::Off;
    const auto mask = proc.getRingMask();
    bool anyMidi = kbd;
    for (int b = 0; b < sz::kNumBands; ++b) {
        const float lvl = proc.getBandLevel(b);
        const bool midiBand = proc.bank.bandMidiDriven(b);
        anyMidi |= midiBand;
        // dolgu: canlı seviye (Trip ve MIDI dahil); ince çizgi: kayıtlı (çizilen) seviye
        float shown = midiBand ? proc.bank.effectiveLevel(b) : lvl;
        if (tripOn) shown *= proc.bank.tripFactor(b);
        if (analysing) shown = analysed[(size_t) b];
        const bool showStored = !analysing && (tripOn || midiBand);
        const float x = p.getX() + b * bw;
        const auto col = juce::Rectangle<float>(x, p.getY(), bw, p.getHeight());
        const bool selected = ((mask >> b) & 1u) != 0;

        if (b == hoverBand) {
            g.setColour(ph.withAlpha(0.07f));
            g.fillRect(col);
        }

        if (selected) {
            // Ring Mod seçimi: üstte elmas, sütun çerçevesi
            const float cx = col.getCentreX(), cy = p.getY() - 10.f, d = juce::jmin(5.f, bw * 0.3f);
            juce::Path dia;
            dia.addQuadrilateral(cx, cy - d, cx + d, cy, cx, cy + d, cx - d, cy);
            g.setColour(ph.withAlpha(ringOn ? 1.f : 0.35f));
            g.fillPath(dia);
            g.setColour(ph.withAlpha(ringOn ? 0.45f : 0.15f));
            g.drawRect(col.reduced(1.f, 0.f), 1.f);
        }

        const auto bar = col.reduced(bw * 0.14f, 0.f).withTop(p.getBottom() - shown * p.getHeight());
        g.setColour(ph.withAlpha(analysing ? 0.45f : 0.20f));
        g.fillRect(bar);
        if (crt) {
            g.setColour(ph.withAlpha(0.22f));
            g.fillRect(bar.withHeight(7.f).translated(0.f, -2.5f).expanded(1.5f, 0.f));
        }
        g.setColour(ph);
        g.fillRect(bar.withHeight(2.5f));
        if (showStored) {
            g.setColour(ph.withAlpha(0.5f));
            g.fillRect(col.reduced(bw * 0.14f, 0.f).withTop(p.getBottom() - lvl * p.getHeight()).withHeight(1.f));
        }

        // canlı çıkış seviyesi (-60..0 dB), LED blokları
        const float m = proc.bank.outputMeter(b);
        const float db = m > 1e-6f ? 20.f * std::log10(m) : -120.f;
        const float norm = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
        auto& mShown = meterShown[(size_t) b];
        mShown = juce::jmax(norm, mShown * 0.88f);
        const float mh = mShown * p.getHeight();
        const auto meterCol = col.reduced(bw * 0.32f, 0.f);
        g.setColour(ph.withAlpha(0.75f));
        for (float y = p.getBottom() - 4.f; y > p.getBottom() - mh; y -= 6.f)
            g.fillRect(meterCol.getX(), y, meterCol.getWidth(), 4.f);
    }

    // frekans etiketleri (her 6 bant ≈ bir oktav)
    g.setFont(CrtLook::mono(11.f, false));
    g.setColour(ph.withAlpha(0.6f));
    for (int b = 0; b < sz::kNumBands; b += 6) {
        const double hz = sz::bandFrequency(b);
        const juce::String t = hz >= 1000 ? juce::String(hz / 1000.0, 1) + "k" : juce::String(juce::roundToInt(hz));
        g.drawText(t, juce::Rectangle<float>(p.getX() + (b + 0.5f) * bw - 20.f, p.getBottom() + 3.f, 40.f, 14.f),
                   juce::Justification::centred);
    }

    // durum satırı
    const auto status = juce::Rectangle<float>(p.getX(), bounds.getBottom() - 26.f, p.getWidth(), 20.f);
    g.setColour(ph.withAlpha(0.18f));
    g.fillRect(status.getX(), status.getY() - 4.f, status.getWidth(), 1.f);
    g.setFont(CrtLook::mono(13.f));
    g.setColour(ph);
    juce::String left;
    if (analysis == Analysis::Waiting) {
        left = TR("ANALİZ: ses bekleniyor… (sinyal Süzgeç'ten geçmeli)");
    } else if (analysis == Analysis::Ready && hoverBand < 0) {
        left = TR("ANALİZ: istediğin an UYGULA  ·  TERS ÇEVİR = tersini uygula");
    } else if (hoverBand >= 0) {
        const double hz = proc.bank.bandFrequencyNow(hoverBand);
        const juce::String f = hz >= 1000 ? juce::String(hz / 1000.0, 2) + " kHz" : juce::String(hz, 1) + " Hz";
        left = TR("BANT ") + juce::String(hoverBand + 1).paddedLeft('0', 2) + "   " + f + "   "
             + juce::String(sz::noteName(hz)) + "   %"
             + juce::String(juce::roundToInt(proc.getBandLevel(hoverBand) * 100.f));
        if ((mask >> hoverBand) & 1u) left += TR("   ◆ RING");
    } else if (ringOn) {
        left = TR("RING MOD: tık = bandı seç/bırak  ·  sürükle = çiz");
    } else {
        left = TR("ekranda sürükleyerek çiz  ·  alt+tık: bandı %75'e döndür");
    }

    const float res = proc.apvts.getRawParameterValue("resonance")->load();
    juce::String right = "Q " + juce::String(sz::resonanceToQ(res * 0.01f), 1) + "   "
                       + (proc.apvts.getRawParameterValue("slope")->load() > 0.5f ? "24" : "12") + " dB/oct";
    if (tripOn) {
        const float hz = proc.getTripRateHz();
        right = "TRIP " + (hz < 10.f ? juce::String(hz, 2) : juce::String(hz, 0)) + " Hz   " + right;
    }
    if (anyMidi) {
        auto ms = [](float s) { return s < 1.f ? juce::String(juce::roundToInt(s * 1000.f)) + "ms" : juce::String(s, 2) + "s"; };
        right = (kbd ? TR("KLAVYE  ") : TR("MIDI  ")) + TR("ATAK ") + ms(proc.bank.attackSeconds()) + TR("  SÖNÜM ")
              + ms(proc.bank.decaySeconds()) + "   " + right;
    }
    const float rightW = juce::GlyphArrangement::getStringWidth(CrtLook::mono(13.f), right);
    g.drawFittedText(left, status.withTrimmedRight(rightW + 16.f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour(ph.withAlpha(0.6f));
    g.drawText(right, status, juce::Justification::centredRight);

    g.setColour(ph.withAlpha(0.35f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.f, 1.2f);
}

// ---------------------------------------------------------------- Trip deseni
TripPattern::TripPattern(SuzgecProcessor& p, CrtLook& l) : proc(p), look(l) { startTimerHz(30); }

void TripPattern::paint(juce::Graphics& g)
{
    const auto ph = look.phosphor();
    const auto r = getLocalBounds().toFloat();
    g.setColour(look.background().darker(0.4f));
    g.fillRoundedRectangle(r, 6.f);
    g.setColour(ph.withAlpha(0.3f));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.f, 1.f);

    const auto a = r.reduced(10.f, 8.f).withTrimmedBottom(14.f);
    const bool on = proc.apvts.getRawParameterValue("tripOn")->load() > 0.5f;
    const float shape = proc.apvts.getRawParameterValue("tripShape")->load() * 0.01f;
    const float bw = a.getWidth() / sz::kNumBands;

    // her bant için seviye çarpanı (trip açıkken canlı)
    for (int b = 0; b < sz::kNumBands; ++b) {
        const float off = sz::FilterBank::tripOffset(b, shape);
        const float fac = on ? proc.bank.tripFactor(b) : 1.f;
        const float x = a.getX() + b * bw;
        // faz kayması: nokta
        g.setColour(ph.withAlpha(0.35f));
        g.fillEllipse(x + bw * 0.5f - 1.5f, a.getY() + (off - std::floor(off)) * a.getHeight() - 1.5f, 3.f, 3.f);
        // çarpan: alttan çubuk
        g.setColour(ph.withAlpha(on ? 0.85f : 0.25f));
        const float h = fac * a.getHeight() * 0.35f;
        g.fillRect(x + bw * 0.2f, a.getBottom() - h, bw * 0.6f, h);
    }
    if (on) {
        // o anki faz: yatay çizgi
        const float y = a.getY() + proc.bank.tripPhaseNow() * a.getHeight();
        g.setColour(ph.withAlpha(0.7f));
        g.fillRect(a.getX(), y, a.getWidth(), 1.f);
    }
    g.setFont(CrtLook::mono(11.f, false));
    g.setColour(ph.withAlpha(0.6f));
    const juce::String name = shape < 0.17f ? TR("SIRALI") : shape < 0.34f ? TR("SIRALI → TEK/ÇİFT")
                            : shape < 0.67f ? TR("TEK/ÇİFT") : shape < 0.84f ? TR("TEK/ÇİFT → KARIŞIK")
                                                                              : TR("KARIŞIK");
    g.drawText(TR("DESEN: ") + name, r.reduced(10.f, 4.f), juce::Justification::bottomLeft);
}

// ---------------------------------------------------------------- düğme
Knob::Knob(SuzgecProcessor& p, const juce::String& id, const juce::String& label, bool bipolar)
    : paramId(id), attachment(p.apvts, id, slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 18);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.getProperties().set("bipolar", bipolar);
    slider.setDoubleClickReturnValue(true, p.apvts.getParameterRange(id).convertFrom0to1(
                                               p.apvts.getParameter(id)->getDefaultValue()));
    slider.setPopupMenuEnabled(false);
    caption.setText(label, juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(CrtLook::mono(12.f));
    caption.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(slider);
    addAndMakeVisible(caption);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    caption.setBounds(r.removeFromTop(16));
    slider.setBounds(r);
}

// ---------------------------------------------------------------- seçim düğmeleri
Segmented::Segmented(juce::RangedAudioParameter& prm, const juce::StringArray& labels, const juce::String& cap)
    : attachment(prm, [this](float v) { update(juce::roundToInt(v)); })
{
    caption.setText(cap, juce::dontSendNotification);
    caption.setFont(CrtLook::mono(12.f));
    caption.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(caption);
    for (int i = 0; i < labels.size(); ++i) {
        auto* b = buttons.add(new RClickButton());
        b->setButtonText(labels[i]);
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture((float) i); };
        addAndMakeVisible(b);
    }
    attachment.sendInitialUpdate();
}

void Segmented::setRightClick(std::function<void()> fn)
{
    for (auto* b : buttons) b->onRightClick = fn;
}

void Segmented::update(int index)
{
    for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState(i == index, juce::dontSendNotification);
}

void Segmented::resized()
{
    auto r = getLocalBounds();
    caption.setBounds(r.removeFromTop(16));
    r.removeFromTop(2);
    const int w = r.getWidth() / juce::jmax(1, buttons.size());
    for (auto* b : buttons) b->setBounds(r.removeFromLeft(w).reduced(2, 0));
}

// ---------------------------------------------------------------- içerik
Content::Content(SuzgecProcessor& p, CrtLook& l)
    : proc(p), look(l), display(p, l), keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      ringAtt(p.apvts, "ringOn", ringBtn), downsampleAtt(p.apvts, "downsample", downsampleBtn),
      kbdAtt(p.apvts, "kbdMode", kbdBtn), input(p, "input", TR("GİRİŞ")), output(p, "output", TR("ÇIKIŞ")),
      mix(p, "mix", TR("KARIŞIM")), resonance(p, "resonance", TR("REZONANS")),
      panEven(p, "panEven", TR("ÇİFT"), true), panOdd(p, "panOdd", TR("TEK"), true),
      softClipAtt(p.apvts, "softclip", softClipBtn),
      shift(*p.apvts.getParameter("shift"), { "-1 ST", "KAPALI", "+1 ST" }, TR("KAYDIR")),
      slope(*p.apvts.getParameter("slope"), { "12 dB", "24 dB" }, TR("EĞİM")),
      tripOnAtt(p.apvts, "tripOn", tripOnBtn), tripSyncAtt(p.apvts, "tripSync", tripSyncBtn),
      tripPhaseAtt(p.apvts, "tripPhase", tripPhaseBtn), tripSpeed(p, "tripSpeed", TR("HIZ")),
      tripDiv(p, "tripDiv", TR("HIZ (SENK)")), tripIntensity(p, "tripIntensity", TR("YOĞUNLUK")),
      tripShape(p, "tripShape", TR("ŞEKİL")), tripToPan(p, "tripToPan", TR("PAN'A")), tripPattern(p, l),
      syncWatch(*p.apvts.getParameter("tripSync"),
                [this](float v) {
                    tripSpeed.setVisible(v < 0.5f);
                    tripDiv.setVisible(v >= 0.5f);
                }),
      kbdWatch(*p.apvts.getParameter("kbdMode"), [this](float v) {
          const bool on = v >= 0.5f;
          keyboard.setVisible(on);
          shift.setEnabled(!on); // klavye modunda Kaydır devre dışı
          shift.setAlpha(on ? 0.4f : 1.f);
          if (on && display.analysisState() != BandDisplay::Analysis::Off) display.stopAnalysis();
          layoutDisplay();
      })
{
    addAndMakeVisible(display);
    display.onRightClick = [this] {
        juce::StringArray ids;
        for (int b = 0; b < sz::kNumBands; ++b) ids.add(SuzgecProcessor::bandParamId(b));
        showLockMenu(ids, TR("48 bant seviyesi"));
    };

    keyboard.setAvailableRange(21, 117); // A0 .. A8
    keyboard.setLowestVisibleKey(21);
    keyboard.setWantsKeyboardFocus(false);
    keyboard.setScrollButtonsVisible(false);
    keyboard.setOctaveForMiddleC(4);
    addChildComponent(keyboard);

    // --- araç satırı
    flipBtn.setButtonText(TR("TERS ÇEVİR"));
    flipBtn.setTooltip(TR("Bütün bant seviyelerini ters çevir (%90 -> %10). Analiz sırasında: gösterilenin tersini uygula."));
    flipBtn.onClick = [this] {
        if (display.analysisState() == BandDisplay::Analysis::Ready) {
            display.applyAnalysis(true);
            return;
        }
        display.stopAnalysis();
        proc.flipBands();
    };
    addAndMakeVisible(flipBtn);

    ringBtn.setButtonText("RING MOD");
    ringBtn.setClickingTogglesState(true);
    ringBtn.setTooltip(TR("Açıkken ekranda banda tıklayarak taşıyıcı bantları seç"));
    addAndMakeVisible(ringBtn);
    ringRandomBtn.onClick = [this] {
        proc.randomOrClearRing();
        timerCallback();
    };
    addAndMakeVisible(ringRandomBtn);
    downsampleBtn.setButtonText(TR("DOWNSAMPLE 1/4"));
    downsampleBtn.setClickingTogglesState(true);
    downsampleBtn.setTooltip(TR("İşlenmiş sesi host örnekleme hızının dörtte birine düşürür"));
    addAndMakeVisible(downsampleBtn);

    analyzeBtn.setTooltip(TR("Gelen sesin spektrumunu oku, sonra UYGULA ile bant seviyelerine yaz"));
    analyzeBtn.onClick = [this] {
        switch (display.analysisState()) {
        case BandDisplay::Analysis::Off: display.startAnalysis(); break;
        case BandDisplay::Analysis::Waiting: display.stopAnalysis(); break;
        case BandDisplay::Analysis::Ready: display.applyAnalysis(false); break;
        }
        timerCallback();
    };
    addAndMakeVisible(analyzeBtn);

    kbdBtn.setButtonText(TR("KLAVYE"));
    kbdBtn.setClickingTogglesState(true);
    kbdBtn.setTooltip(TR("Klavye modu: MIDI notaları en yakın bandı tam nota frekansına akort eder"));
    addAndMakeVisible(kbdBtn);

    // --- preset çubuğu
    prevBtn.setButtonText("<");
    nextBtn.setButtonText(">");
    prevBtn.onClick = [this] { proc.presets.step(-1); timerCallback(); };
    nextBtn.onClick = [this] { proc.presets.step(1); timerCallback(); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    saveBtn.setButtonText(TR("KAYDET"));
    saveBtn.onClick = [this] { savePresetDialog(); };
    diceBtn.setButtonText(TR("ZAR"));
    diceBtn.setTooltip(TR("Rastgele ses (kilitli parametreler değişmez)"));
    diceBtn.onClick = [this] { proc.presets.randomize(); timerCallback(); };
    for (auto* b : { &prevBtn, &presetBtn, &nextBtn, &saveBtn, &diceBtn }) addAndMakeVisible(b);

    // --- trip
    tripOnBtn.setButtonText(TR("AÇIK"));
    tripSyncBtn.setButtonText(TR("SENK"));
    tripPhaseBtn.setButtonText(TR("TERS YÖN"));
    for (auto* b : { &tripOnBtn, &tripSyncBtn, &tripPhaseBtn }) {
        b->setClickingTogglesState(true);
        addAndMakeVisible(b);
    }
    for (juce::Component* c : std::initializer_list<juce::Component*> { &tripSpeed, &tripDiv, &tripIntensity,
                                                                         &tripShape, &tripToPan, &tripPattern })
        addAndMakeVisible(c);

    // --- başlık
    paletteBtn.onClick = [this] {
        proc.setPalette(proc.getPalette() + 1);
        applyPalette();
    };
    paletteBtn.onRightClick = [this] {
        proc.setPalette(proc.getPalette() - 1 + paletteCount());
        applyPalette();
    };
    addAndMakeVisible(paletteBtn);

    crtBtn.setButtonText("CRT");
    crtBtn.setClickingTogglesState(true);
    crtBtn.setToggleState(proc.getCrt(), juce::dontSendNotification);
    crtBtn.onClick = [this] {
        proc.setCrt(crtBtn.getToggleState());
        display.crt = crtBtn.getToggleState();
        repaint();
    };
    addAndMakeVisible(crtBtn);

    menuBtn.setButtonText(TR("MENÜ"));
    menuBtn.onClick = [this] { showMenu(); };
    addAndMakeVisible(menuBtn);

    softClipBtn.setButtonText(TR("YUMUŞAK KIRP"));
    softClipBtn.setClickingTogglesState(true);
    addAndMakeVisible(softClipBtn);

    for (juce::Component* c : std::initializer_list<juce::Component*> { &input, &output, &mix, &resonance, &panEven,
                                                                         &panOdd, &shift, &slope })
        addAndMakeVisible(c);

    // --- parametre kilidi (sağ tık)
    for (auto* k : { &input, &output, &mix, &resonance, &panEven, &panOdd, &tripSpeed, &tripDiv, &tripIntensity,
                     &tripShape, &tripToPan }) {
        k->slider.onRightClick = [this, k] { showLockMenu({ k->paramId }, k->paramId, k); };
        registerLockable(*k, { k->paramId });
    }
    const std::pair<RClickButton*, const char*> toggles[] = {
        { &softClipBtn, "softclip" }, { &ringBtn, "ringOn" },     { &downsampleBtn, "downsample" },
        { &kbdBtn, "kbdMode" },       { &tripOnBtn, "tripOn" },   { &tripSyncBtn, "tripSync" },
        { &tripPhaseBtn, "tripPhase" },
    };
    for (auto [b, id] : toggles) {
        const juce::String pid(id);
        b->onRightClick = [this, pid] { showLockMenu({ pid }, pid); };
        registerLockable(*b, { pid });
    }
    shift.setRightClick([this] { showLockMenu({ "shift" }, "shift"); });
    slope.setRightClick([this] { showLockMenu({ "slope" }, "slope"); });
    registerLockable(shift, { "shift" });
    registerLockable(slope, { "slope" });

    display.crt = proc.getCrt();
    syncWatch.sendInitialUpdate();
    setSize(SuzgecEditor::kBaseW, SuzgecEditor::kBaseH);
    kbdWatch.sendInitialUpdate();
    applyPalette();
    timerCallback();
    startTimerHz(10);
}

void Content::registerLockable(juce::Component& c, const juce::StringArray& ids) { lockables.push_back({ &c, ids }); }

void Content::timerCallback()
{
    const auto t = proc.getRingMask() != 0 ? TR("TEMİZLE") : TR("RASTGELE");
    if (ringRandomBtn.getButtonText() != t) ringRandomBtn.setButtonText(t);

    juce::String a;
    switch (display.analysisState()) {
    case BandDisplay::Analysis::Off: a = TR("ANALİZ"); break;
    case BandDisplay::Analysis::Waiting: a = TR("ANALİZ…"); break;
    case BandDisplay::Analysis::Ready: a = TR("UYGULA"); break;
    }
    if (analyzeBtn.getButtonText() != a) analyzeBtn.setButtonText(a);
    analyzeBtn.setToggleState(display.analysisState() != BandDisplay::Analysis::Off, juce::dontSendNotification);

    const auto name = proc.presets.currentName();
    if (presetBtn.getButtonText() != name) presetBtn.setButtonText(name);
}

void Content::applyPalette()
{
    const auto& pe = paletteAt(proc.getPalette());
    look.setPhosphor(juce::Colour(pe.argb), proc.getBrightness(), proc.getContrast());
    paletteBtn.setButtonText(TR(pe.name));

    const auto ph = look.phosphor(), bg = look.background();
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, bg.interpolatedWith(ph, 0.12f));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, bg.darker(0.5f));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, ph.withAlpha(0.35f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ph.withAlpha(0.25f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, ph.withAlpha(0.85f));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, ph.withAlpha(0.7f));

    sendLookAndFeelChange();
    repaint();
}

void Content::showLockMenu(const juce::StringArray& ids, const juce::String& title, Knob* knob)
{
    bool all = true;
    for (auto& id : ids) all &= proc.isLocked(id);
    juce::String name = title;
    if (ids.size() == 1)
        if (auto* prm = proc.apvts.getParameter(ids[0])) name = prm->getName(40);

    juce::PopupMenu m;
    m.addSectionHeader(name);
    m.addItem(all ? TR("Kilidi aç") : TR("Kilitle"), [this, ids, all] {
        proc.setLockedMany(ids, !all);
        repaint();
    });
    if (knob != nullptr) m.addItem(TR("Değer gir…  (Shift+tık)"), [knob] { knob->slider.showTextBox(); });
    m.addSeparator();
    m.addItem(TR("Hepsini kilitle"), [this] {
        proc.lockAll(true);
        repaint();
    });
    m.addItem(TR("Bütün kilitleri aç"), [this] {
        proc.lockAll(false);
        repaint();
    });
    m.showMenuAsync(juce::PopupMenu::Options());
}

void Content::showPresetMenu()
{
    juce::PopupMenu m;
    const auto current = proc.presets.currentName();
    m.addSectionHeader(TR("FABRİKA"));
    for (auto& e : proc.presets.allEntries()) {
        if (!e.factory) continue;
        const int fi = e.factoryIndex;
        m.addItem(e.name, true, e.name == current, [this, fi] {
            proc.presets.loadFactory(fi);
            timerCallback();
        });
    }
    bool anyUser = false;
    for (auto& e : proc.presets.allEntries()) {
        if (e.factory) continue;
        if (!anyUser) {
            m.addSeparator();
            m.addSectionHeader(TR("KULLANICI"));
            anyUser = true;
        }
        const auto f = e.file;
        m.addItem(e.name, true, e.name == current, [this, f] {
            proc.presets.loadFile(f);
            timerCallback();
        });
    }
    m.addSeparator();
    m.addItem(TR("Kaydet…"), [this] { savePresetDialog(); });
    m.addItem(TR("Preset klasörünü aç"), [] {
        auto dir = PresetManager::userFolder();
        dir.createDirectory();
        dir.revealToUser();
    });
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBtn));
}

void Content::savePresetDialog()
{
    auto* w = new juce::AlertWindow(TR("Preset kaydet"), TR("Preset adı:"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.presets.currentName());
    w->addButton(TR("KAYDET"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(TR("VAZGEÇ"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Content> safe(this);
    w->enterModalState(true, juce::ModalCallbackFunction::create([safe, w](int result) {
                           if (result != 1 || safe == nullptr) return;
                           const auto name = w->getTextEditorContents("name");
                           if (!safe->proc.presets.save(name))
                               juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                                      TR("Süzgeç"), TR("Preset kaydedilemedi."));
                           safe->timerCallback();
                       }),
                       true);
}

void Content::showMenu()
{
    juce::PopupMenu sizes;
    const int w = proc.getUiWidth();
    sizes.addItem(TR("Küçük"), true, w == 720, [this] { if (onSizeChosen) onSizeChosen(720); });
    sizes.addItem(TR("Standart"), true, w == 960, [this] { if (onSizeChosen) onSizeChosen(960); });
    sizes.addItem(TR("Büyük"), true, w == 1280, [this] { if (onSizeChosen) onSizeChosen(1280); });

    juce::PopupMenu os;
    const char* osNames[] = { "Kapalı", "2x", "4x", "8x", "16x" };
    for (int i = 0; i < 5; ++i)
        os.addItem(TR(osNames[i]), true, proc.getOversampling() == i, [this, i] { proc.setOversampling(i); });

    auto stepMenu = [this](float current, std::function<void(float)> set) {
        juce::PopupMenu sm;
        for (float v : { 0.7f, 0.85f, 1.f, 1.15f, 1.3f })
            sm.addItem("%" + juce::String(juce::roundToInt(v * 100.f)), true, std::abs(current - v) < 0.01f, [this, v, set] {
                set(v);
                applyPalette();
            });
        return sm;
    };

    juce::PopupMenu m;
    m.addSubMenu(TR("Oversampling"), os);
    m.addSubMenu(TR("Pencere boyutu"), sizes);
    m.addSubMenu(TR("Parlaklık"), stepMenu(proc.getBrightness(), [this](float v) { proc.setBrightness(v); }));
    m.addSubMenu(TR("Kontrast"), stepMenu(proc.getContrast(), [this](float v) { proc.setContrast(v); }));
    m.addSeparator();
    m.addItem(TR("Preseti panoya kopyala"), [this] { juce::SystemClipboard::copyTextToClipboard(proc.presetToString()); });
    m.addItem(TR("Panodan preset yapıştır"), [this] {
        if (!proc.presetFromString(juce::SystemClipboard::getTextFromClipboard()))
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, TR("Süzgeç"),
                                                   TR("Panoda geçerli bir Süzgeç preseti yok."));
        timerCallback();
    });
    m.addSeparator();
    m.addItem(TR("Bütün bantları %75'e sıfırla"), [this] {
        for (int b = 0; b < sz::kNumBands; ++b) {
            auto* prm = proc.bandParam(b);
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(kDefaultLevel);
            prm->endChangeGesture();
        }
    });
    m.addItem(TR("Bütün kilitleri aç"), [this] {
        proc.lockAll(false);
        repaint();
    });
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuBtn));
}

void Content::layoutDisplay()
{
    const int W = getWidth();
    if (keyboard.isVisible()) {
        display.setBounds(20, 92, W - 40, 222);
        keyboard.setBounds(20, 318, W - 40, 54);
        keyboard.setKeyWidth((float) (W - 40) / 50.f); // A0..A8: 50 beyaz tuş
    } else {
        display.setBounds(20, 92, W - 40, 280);
    }
}

void Content::resized()
{
    const int W = getWidth();
    // başlık
    menuBtn.setBounds(W - 20 - 64, 12, 64, 28);
    crtBtn.setBounds(menuBtn.getX() - 8 - 60, 12, 60, 28);
    paletteBtn.setBounds(crtBtn.getX() - 8 - 110, 12, 110, 28);
    prevBtn.setBounds(150, 12, 30, 28);
    presetBtn.setBounds(184, 12, 236, 28);
    nextBtn.setBounds(424, 12, 30, 28);
    saveBtn.setBounds(462, 12, 84, 28);
    diceBtn.setBounds(552, 12, 60, 28);

    // araç satırı + ekran
    flipBtn.setBounds(20, 56, 130, 28);
    ringBtn.setBounds(166, 56, 110, 28);
    ringRandomBtn.setBounds(282, 56, 100, 28);
    downsampleBtn.setBounds(398, 56, 140, 28);
    analyzeBtn.setBounds(W - 20 - 110 - 8 - 120, 56, 120, 28);
    kbdBtn.setBounds(W - 20 - 110, 56, 110, 28);
    layoutDisplay();

    // alt paneller
    const int top = 388, h = 150;
    masterPanel = { 20, top, 400, h };
    filterPanel = { 432, top, 300, h };
    panPanel = { 744, top, W - 20 - 744, h };
    tripPanel = { 20, top + h + 12, W - 40, h };

    auto knobRow = [](juce::Rectangle<int> r) { return r.withTrimmedTop(30).withTrimmedBottom(8); };

    auto mr = knobRow(masterPanel).reduced(8, 0);
    input.setBounds(mr.removeFromLeft(90));
    output.setBounds(mr.removeFromLeft(90));
    mix.setBounds(mr.removeFromLeft(90));
    softClipBtn.setBounds(mr.reduced(4, 0).withSizeKeepingCentre(mr.getWidth() - 8, 30));

    auto fr = knobRow(filterPanel).reduced(10, 0);
    auto seg = fr.removeFromLeft(180);
    shift.setBounds(seg.removeFromTop(50));
    seg.removeFromTop(6);
    slope.setBounds(seg.removeFromTop(50).withTrimmedRight(60));
    resonance.setBounds(fr);

    auto pr = knobRow(panPanel).reduced(8, 0);
    panOdd.setBounds(pr.removeFromLeft(pr.getWidth() / 2));
    panEven.setBounds(pr);

    auto tr = knobRow(tripPanel).reduced(12, 0);
    auto toggleCol = tr.removeFromLeft(120);
    tripOnBtn.setBounds(toggleCol.removeFromTop(32).reduced(0, 2));
    tripSyncBtn.setBounds(toggleCol.removeFromTop(38).reduced(0, 5));
    tripPhaseBtn.setBounds(toggleCol.removeFromTop(32).reduced(0, 2));
    tr.removeFromLeft(12);
    const auto speedArea = tr.removeFromLeft(100);
    tripSpeed.setBounds(speedArea);
    tripDiv.setBounds(speedArea);
    tripIntensity.setBounds(tr.removeFromLeft(100));
    tripShape.setBounds(tr.removeFromLeft(100));
    tripToPan.setBounds(tr.removeFromLeft(100));
    tr.removeFromLeft(12);
    tripPattern.setBounds(tr);

    buildOverlay();
}

void Content::paint(juce::Graphics& g)
{
    const auto ph = look.phosphor();
    g.fillAll(look.background());

    // başlık
    g.setColour(ph);
    g.setFont(CrtLook::mono(26.f));
    g.drawText(TR("SÜZGEÇ"), 20, 10, 200, 32, juce::Justification::centredLeft);

    auto drawPanel = [&](juce::Rectangle<int> r, const juce::String& title) {
        const auto rf = r.toFloat();
        g.setColour(look.panel());
        g.fillRoundedRectangle(rf, 8.f);
        g.setColour(ph.withAlpha(0.3f));
        g.drawRoundedRectangle(rf.reduced(0.5f), 8.f, 1.f);
        g.setColour(ph);
        g.setFont(CrtLook::mono(13.f));
        g.drawText(title, r.getX() + 12, r.getY() + 6, r.getWidth() - 24, 18, juce::Justification::centredLeft);
        g.setColour(ph.withAlpha(0.2f));
        g.fillRect(r.getX() + 12, r.getY() + 26, r.getWidth() - 24, 1);
    };
    drawPanel(masterPanel, TR("ANA"));
    drawPanel(filterPanel, TR("FİLTRE"));
    drawPanel(panPanel, TR("PAN"));
    drawPanel(tripPanel, TR("TRIP  ·  bant seviyelerini ve stereo alanı hareketlendirir"));
}

void Content::buildOverlay()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    // tarama çizgileri + köşe kararması; ölçeklenmiş çizim için 2x çözünürlük
    const int s = 2;
    overlay = juce::Image(juce::Image::ARGB, getWidth() * s, getHeight() * s, true);
    juce::Graphics g(overlay);
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    for (int y = 0; y < overlay.getHeight(); y += 3 * s) g.fillRect(0, y, overlay.getWidth(), s);
    juce::ColourGradient vig(juce::Colours::transparentBlack, overlay.getWidth() * 0.5f, overlay.getHeight() * 0.5f,
                             juce::Colours::black.withAlpha(0.5f), 0.f, 0.f, true);
    vig.addColour(0.6, juce::Colours::transparentBlack);
    g.setGradientFill(vig);
    g.fillAll();
}

void Content::paintOverChildren(juce::Graphics& g)
{
    // kilit simgeleri
    const auto ph = look.phosphor();
    auto padlock = [&](float x, float y) {
        g.setColour(look.background());
        g.fillRoundedRectangle(x - 2.f, y - 2.f, 14.f, 15.f, 3.f);
        g.setColour(ph);
        juce::Path shackle;
        shackle.addCentredArc(x + 5.f, y + 5.f, 3.2f, 3.5f, 0.f, -juce::MathConstants<float>::halfPi,
                              juce::MathConstants<float>::halfPi, true);
        g.strokePath(shackle, juce::PathStrokeType(1.5f));
        g.fillRoundedRectangle(x + 0.5f, y + 5.f, 9.f, 6.5f, 1.5f);
    };
    for (auto& [comp, ids] : lockables) {
        if (!comp->isVisible()) continue;
        bool all = true;
        for (auto& id : ids) all &= proc.isLocked(id);
        if (!all) continue;
        const auto b = comp->getBounds().toFloat();
        padlock(b.getRight() - 14.f, b.getY() + 1.f);
    }
    int bandLocks = 0;
    for (int b = 0; b < sz::kNumBands; ++b) bandLocks += proc.isLocked(SuzgecProcessor::bandParamId(b)) ? 1 : 0;
    if (bandLocks > 0) {
        const auto d = display.getBounds().toFloat();
        padlock(d.getRight() - 22.f, d.getY() + 6.f);
    }

    if (!proc.getCrt() || !overlay.isValid()) return;
    g.drawImage(overlay, getLocalBounds().toFloat());
}

// ---------------------------------------------------------------- editör
SuzgecEditor::SuzgecEditor(SuzgecProcessor& p) : AudioProcessorEditor(&p), proc(p), content(p, look)
{
    setLookAndFeel(&look);
    addAndMakeVisible(content);
    content.onSizeChosen = [this](int w) { setSize(w, w * kBaseH / kBaseW); };

    setResizable(true, true);
    setResizeLimits(720, 540, 1600, 1200);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio((double) kBaseW / kBaseH);
    const int w = juce::jlimit(720, 1600, proc.getUiWidth());
    setSize(w, w * kBaseH / kBaseW);
}

SuzgecEditor::~SuzgecEditor() { setLookAndFeel(nullptr); }

void SuzgecEditor::resized()
{
    const float s = (float) getWidth() / (float) kBaseW;
    content.setBounds(0, 0, kBaseW, kBaseH);
    content.setTransform(juce::AffineTransform::scale(s));
    proc.setUiWidth(getWidth());
}

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

void CrtLook::setPhosphor(juce::Colour c)
{
    ph = c;
    // fosfor rengine çok hafif bürünmüş siyah zemin
    bg = juce::Colour(0xff050706).interpolatedWith(c, 0.035f);
    pn = juce::Colour(0xff0b0f0d).interpolatedWith(c, 0.06f);

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

BandDisplay::~BandDisplay() { endGestures(); }

juce::Rectangle<float> BandDisplay::plotArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft(10.f).withTrimmedRight(10.f).withTrimmedTop(12.f)
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
    dragging = true;
    lastPt = e.position;
    if (e.mods.isAltDown() || e.getNumberOfClicks() > 1) {
        setBand(bandAt(e.position.x), kDefaultLevel); // alt/çift tık: varsayılan seviye
        return;
    }
    drawTo(e.position);
}

void BandDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isAltDown()) return;
    drawTo(e.position);
}

void BandDisplay::mouseUp(const juce::MouseEvent& e)
{
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
    for (int b = 0; b < sz::kNumBands; ++b) {
        const float lvl = proc.getBandLevel(b);
        const float x = p.getX() + b * bw;
        const auto col = juce::Rectangle<float>(x, p.getY(), bw, p.getHeight());

        if (b == hoverBand) {
            g.setColour(ph.withAlpha(0.07f));
            g.fillRect(col);
        }

        const auto bar = col.reduced(bw * 0.14f, 0.f).withTop(p.getBottom() - lvl * p.getHeight());
        g.setColour(ph.withAlpha(0.20f));
        g.fillRect(bar);
        if (crt) {
            g.setColour(ph.withAlpha(0.22f));
            g.fillRect(bar.withHeight(7.f).translated(0.f, -2.5f).expanded(1.5f, 0.f));
        }
        g.setColour(ph);
        g.fillRect(bar.withHeight(2.5f));

        // canlı çıkış seviyesi (-60..0 dB), LED blokları
        const float m = proc.bank.outputMeter(b);
        const float db = m > 1e-6f ? 20.f * std::log10(m) : -120.f;
        const float norm = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
        auto& shown = meterShown[(size_t) b];
        shown = juce::jmax(norm, shown * 0.88f);
        const float mh = shown * p.getHeight();
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
        g.drawText(t, juce::Rectangle<float>(p.getX() + b * bw - 10.f, p.getBottom() + 3.f, 40.f, 14.f),
                   juce::Justification::centredLeft);
    }

    // durum satırı
    const auto status = juce::Rectangle<float>(p.getX(), bounds.getBottom() - 26.f, p.getWidth(), 20.f);
    g.setColour(ph.withAlpha(0.18f));
    g.fillRect(status.getX(), status.getY() - 4.f, status.getWidth(), 1.f);
    g.setFont(CrtLook::mono(13.f));
    g.setColour(ph);
    juce::String left;
    if (hoverBand >= 0) {
        const int shift = proc.apvts.getRawParameterValue("shift")->load() > 1.5f    ? 1
                          : proc.apvts.getRawParameterValue("shift")->load() < 0.5f ? -1
                                                                                     : 0;
        const double hz = sz::bandFrequency(hoverBand, shift);
        const juce::String f = hz >= 1000 ? juce::String(hz / 1000.0, 2) + " kHz" : juce::String(hz, 1) + " Hz";
        left = TR("BANT ") + juce::String(hoverBand + 1).paddedLeft('0', 2) + "   " + f + "   "
             + juce::String(sz::noteName(hz)) + "   %"
             + juce::String(juce::roundToInt(proc.getBandLevel(hoverBand) * 100.f));
    } else {
        left = TR("ekranda sürükleyerek çiz  ·  alt+tık: bandı %75'e döndür");
    }
    g.drawText(left, status, juce::Justification::centredLeft);

    const float res = proc.apvts.getRawParameterValue("resonance")->load();
    const juce::String right = "Q " + juce::String(sz::resonanceToQ(res * 0.01f), 1) + "   "
                             + (proc.apvts.getRawParameterValue("slope")->load() > 0.5f ? "24" : "12") + " dB/oct";
    g.setColour(ph.withAlpha(0.6f));
    g.drawText(right, status, juce::Justification::centredRight);

    g.setColour(ph.withAlpha(0.35f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.f, 1.2f);
}

// ---------------------------------------------------------------- düğme
Knob::Knob(SuzgecProcessor& p, const juce::String& id, const juce::String& label, bool bipolar)
    : attachment(p.apvts, id, slider)
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
        auto* b = buttons.add(new juce::TextButton(labels[i]));
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture((float) i); };
        addAndMakeVisible(b);
    }
    attachment.sendInitialUpdate();
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
    : proc(p), look(l), display(p, l), input(p, "input", TR("GİRİŞ")), output(p, "output", TR("ÇIKIŞ")),
      mix(p, "mix", TR("KARIŞIM")), resonance(p, "resonance", TR("REZONANS")),
      panEven(p, "panEven", TR("ÇİFT"), true), panOdd(p, "panOdd", TR("TEK"), true),
      softClipAtt(p.apvts, "softclip", softClipBtn),
      shift(*p.apvts.getParameter("shift"), { "-1 ST", "KAPALI", "+1 ST" }, TR("KAYDIR")),
      slope(*p.apvts.getParameter("slope"), { "12 dB", "24 dB" }, TR("EĞİM"))
{
    addAndMakeVisible(display);

    flipBtn.setButtonText(TR("TERS ÇEVİR"));
    flipBtn.setTooltip(TR("Bütün bant seviyelerini ters çevir (%90 -> %10)"));
    flipBtn.onClick = [this] { proc.flipBands(); };
    addAndMakeVisible(flipBtn);

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

    display.crt = proc.getCrt();
    setSize(SuzgecEditor::kBaseW, SuzgecEditor::kBaseH);
    applyPalette();
}

void Content::applyPalette()
{
    const auto& pe = paletteAt(proc.getPalette());
    look.setPhosphor(juce::Colour(pe.argb));
    paletteBtn.setButtonText(TR(pe.name));
    sendLookAndFeelChange();
    repaint();
}

void Content::showMenu()
{
    juce::PopupMenu sizes;
    const int w = proc.getUiWidth();
    sizes.addItem(TR("Küçük"), true, w == 720, [this] { if (onSizeChosen) onSizeChosen(720); });
    sizes.addItem(TR("Standart"), true, w == 960, [this] { if (onSizeChosen) onSizeChosen(960); });
    sizes.addItem(TR("Büyük"), true, w == 1280, [this] { if (onSizeChosen) onSizeChosen(1280); });

    juce::PopupMenu m;
    m.addSubMenu(TR("Pencere boyutu"), sizes);
    m.addSeparator();
    m.addItem(TR("Preseti panoya kopyala"), [this] { juce::SystemClipboard::copyTextToClipboard(proc.presetToString()); });
    m.addItem(TR("Panodan preset yapıştır"), [this] {
        if (!proc.presetFromString(juce::SystemClipboard::getTextFromClipboard()))
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, TR("Süzgeç"),
                                                   TR("Panoda geçerli bir Süzgeç preseti yok."));
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
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuBtn));
}

void Content::resized()
{
    const int W = getWidth();
    // başlık
    menuBtn.setBounds(W - 20 - 64, 12, 64, 28);
    crtBtn.setBounds(menuBtn.getX() - 8 - 60, 12, 60, 28);
    paletteBtn.setBounds(crtBtn.getX() - 8 - 110, 12, 110, 28);

    // araç satırı + ekran
    flipBtn.setBounds(20, 56, 130, 28);
    display.setBounds(20, 92, W - 40, 312);

    // alt paneller
    const int top = 420, h = 164;
    masterPanel = { 20, top, 400, h };
    filterPanel = { 432, top, 300, h };
    panPanel = { 744, top, W - 20 - 744, h };

    auto knobRow = [](juce::Rectangle<int> r) { return r.withTrimmedTop(30).withTrimmedBottom(8); };

    auto mr = knobRow(masterPanel).reduced(8, 0);
    input.setBounds(mr.removeFromLeft(90));
    output.setBounds(mr.removeFromLeft(90));
    mix.setBounds(mr.removeFromLeft(90));
    softClipBtn.setBounds(mr.reduced(4, 0).withSizeKeepingCentre(mr.getWidth() - 8, 30));

    auto fr = knobRow(filterPanel).reduced(10, 0);
    auto seg = fr.removeFromLeft(180);
    shift.setBounds(seg.removeFromTop(56).withTrimmedTop(4));
    seg.removeFromTop(10);
    slope.setBounds(seg.removeFromTop(56).withTrimmedRight(60));
    resonance.setBounds(fr);

    auto pr = knobRow(panPanel).reduced(8, 0);
    panOdd.setBounds(pr.removeFromLeft(pr.getWidth() / 2));
    panEven.setBounds(pr);

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
    g.setColour(ph.withAlpha(0.6f));
    g.setFont(CrtLook::mono(12.f, false));
    g.drawText(TR("48 BANT  ·  9:8 SPEKTRAL FİLTRE"), 150, 18, 320, 20, juce::Justification::centredLeft);
    g.drawText(TR("55 Hz – 13.9 kHz  ·  bant aralığı tam ses (203.9 sent)"), 160, 60, getWidth() - 180, 20,
               juce::Justification::centredRight);

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
    drawPanel(panPanel, TR("PAN  ·  TEK / ÇİFT BANTLAR"));
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
    setResizeLimits(720, 450, 1600, 1000);
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

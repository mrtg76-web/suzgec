#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

// ---------------------------------------------------------------- renk paleti
struct PaletteEntry {
    const char* name;
    juce::uint32 argb;
};
int paletteCount();
const PaletteEntry& paletteAt(int index);

// ---------------------------------------------------------------- görünüm
class CrtLook : public juce::LookAndFeel_V4 {
public:
    CrtLook();
    void setPhosphor(juce::Colour c, float brightness = 1.f, float contrast = 1.f);
    juce::Colour phosphor() const { return ph; }
    juce::Colour background() const { return bg; }
    juce::Colour panel() const { return pn; }

    static juce::Font mono(float size, bool bold = true);

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getLabelFont(juce::Label&) override { return mono(13.f); }
    juce::Font getPopupMenuFont() override { return mono(14.f, false); }
    juce::Font getAlertWindowMessageFont() override { return mono(14.f, false); }
    juce::Font getAlertWindowTitleFont() override { return mono(16.f); }
    juce::Label* createSliderTextBox(juce::Slider&) override;

private:
    juce::Colour ph, bg, pn;
};

// sağ tıklamayı da yakalayan düğme
class RClickButton : public juce::TextButton {
public:
    std::function<void()> onRightClick;
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) {
            if (onRightClick) onRightClick();
            return;
        }
        juce::TextButton::mouseUp(e);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!e.mods.isPopupMenu()) juce::TextButton::mouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!e.mods.isPopupMenu()) juce::TextButton::mouseDrag(e);
    }
};

// sağ tık menüsü ve Shift+tık ile değer girişi olan döner düğme
class ParamSlider : public juce::Slider {
public:
    std::function<void()> onRightClick;
    void mouseDown(const juce::MouseEvent& e) override
    {
        ignoreDrag = false;
        if (e.mods.isPopupMenu()) {
            ignoreDrag = true;
            if (onRightClick) onRightClick();
            return;
        }
        if (e.mods.isShiftDown()) {
            ignoreDrag = true;
            showTextBox();
            return;
        }
        juce::Slider::mouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!ignoreDrag) juce::Slider::mouseDrag(e);
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!ignoreDrag) juce::Slider::mouseUp(e);
        ignoreDrag = false;
    }

private:
    bool ignoreDrag = false;
};

// ---------------------------------------------------------------- bant ekranı
class BandDisplay : public juce::Component, private juce::Timer {
public:
    BandDisplay(SuzgecProcessor&, CrtLook&);
    ~BandDisplay() override;

    void paint(juce::Graphics&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    // Spektral yakalama
    enum class Analysis { Off, Waiting, Ready };
    Analysis analysisState() const { return analysis; }
    void startAnalysis();
    void stopAnalysis() { analysis = Analysis::Off; }
    void applyAnalysis(bool inverted); // bant seviyelerine yaz ve analizi bitir

    std::function<void()> onRightClick;
    bool crt = true;

private:
    void timerCallback() override;
    juce::Rectangle<float> plotArea() const;
    int bandAt(float x) const;
    float levelAt(float y) const;
    void drawTo(juce::Point<float> p);
    void setBand(int band, float level);
    void endGestures();

    SuzgecProcessor& proc;
    CrtLook& look;
    int hoverBand = -1;
    bool dragging = false;
    bool pendingClick = false; // Ring Mod: tık mı sürükleme mi henüz belli değil
    juce::Point<float> lastPt, downPt;
    std::array<bool, sz::kNumBands> touched {};
    std::array<float, sz::kNumBands> meterShown {};
    Analysis analysis = Analysis::Off;
    std::array<float, sz::kNumBands> analysed {};
};

// ---------------------------------------------------------------- Trip deseni (küçük ekran)
class TripPattern : public juce::Component, private juce::Timer {
public:
    TripPattern(SuzgecProcessor&, CrtLook&);
    void paint(juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }
    SuzgecProcessor& proc;
    CrtLook& look;
};

// ---------------------------------------------------------------- düğme
class Knob : public juce::Component {
public:
    Knob(SuzgecProcessor&, const juce::String& paramId, const juce::String& label, bool bipolar = false);
    void resized() override;
    ParamSlider slider;
    const juce::String paramId;

private:
    juce::Label caption;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// ---------------------------------------------------------------- seçim düğmeleri (Shift, Slope)
class Segmented : public juce::Component {
public:
    Segmented(juce::RangedAudioParameter&, const juce::StringArray& labels, const juce::String& caption);
    void resized() override;
    void setRightClick(std::function<void()> fn);

private:
    void update(int index);
    juce::Label caption;
    juce::OwnedArray<RClickButton> buttons;
    juce::ParameterAttachment attachment;
};

// ---------------------------------------------------------------- içerik (sabit 960x720, ölçeklenir)
class Content : public juce::Component, private juce::Timer {
public:
    Content(SuzgecProcessor&, CrtLook&);
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void applyPalette();

    std::function<void(int)> onSizeChosen;

private:
    void showMenu();
    void showPresetMenu();
    void savePresetDialog();
    void showLockMenu(const juce::StringArray& ids, const juce::String& title, Knob* knob = nullptr);
    void registerLockable(juce::Component& c, const juce::StringArray& ids);
    void buildOverlay();
    void timerCallback() override;
    void layoutDisplay();

    SuzgecProcessor& proc;
    CrtLook& look;

    BandDisplay display;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton flipBtn, ringRandomBtn, analyzeBtn;
    RClickButton ringBtn, downsampleBtn, kbdBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment ringAtt, downsampleAtt, kbdAtt;
    RClickButton paletteBtn;
    juce::TextButton crtBtn, menuBtn;

    // preset çubuğu
    juce::TextButton prevBtn, presetBtn, nextBtn, saveBtn, diceBtn;

    Knob input, output, mix, resonance, panEven, panOdd;
    RClickButton softClipBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment softClipAtt;
    Segmented shift, slope;

    // Trip
    RClickButton tripOnBtn, tripSyncBtn, tripPhaseBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment tripOnAtt, tripSyncAtt, tripPhaseAtt;
    Knob tripSpeed, tripDiv, tripIntensity, tripShape, tripToPan;
    TripPattern tripPattern;
    juce::ParameterAttachment syncWatch, kbdWatch;

    std::vector<std::pair<juce::Component*, juce::StringArray>> lockables;
    juce::Image overlay;
    juce::Rectangle<int> masterPanel, filterPanel, panPanel, tripPanel;
};

class SuzgecEditor : public juce::AudioProcessorEditor {
public:
    explicit SuzgecEditor(SuzgecProcessor&);
    ~SuzgecEditor() override;
    void resized() override;

    static constexpr int kBaseW = 960, kBaseH = 720;

private:
    SuzgecProcessor& proc;
    CrtLook look;
    Content content;
};

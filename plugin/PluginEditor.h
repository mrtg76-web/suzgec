#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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
    void setPhosphor(juce::Colour c);
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
    juce::Label* createSliderTextBox(juce::Slider&) override;

private:
    juce::Colour ph, bg, pn;
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

    bool crt = true;

private:
    void timerCallback() override { repaint(); }
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
    juce::Point<float> lastPt;
    std::array<bool, sz::kNumBands> touched {};
    std::array<float, sz::kNumBands> meterShown {};
};

// ---------------------------------------------------------------- düğme
class Knob : public juce::Component {
public:
    Knob(SuzgecProcessor&, const juce::String& paramId, const juce::String& label, bool bipolar = false);
    void resized() override;

private:
    juce::Slider slider;
    juce::Label caption;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// ---------------------------------------------------------------- seçim düğmeleri (Shift, Slope)
class Segmented : public juce::Component {
public:
    Segmented(juce::RangedAudioParameter&, const juce::StringArray& labels, const juce::String& caption);
    void resized() override;

private:
    void update(int index);
    juce::Label caption;
    juce::OwnedArray<juce::TextButton> buttons;
    juce::ParameterAttachment attachment;
};

// sağ tıklamayı da yakalayan düğme
class PaletteButton : public juce::TextButton {
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
};

// ---------------------------------------------------------------- içerik (sabit 960x600, ölçeklenir)
class Content : public juce::Component {
public:
    Content(SuzgecProcessor&, CrtLook&);
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void applyPalette();

    std::function<void(int)> onSizeChosen;

private:
    void showMenu();
    void buildOverlay();

    SuzgecProcessor& proc;
    CrtLook& look;

    BandDisplay display;
    juce::TextButton flipBtn;
    PaletteButton paletteBtn;
    juce::TextButton crtBtn, menuBtn;

    Knob input, output, mix, resonance, panEven, panOdd;
    juce::TextButton softClipBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment softClipAtt;
    Segmented shift, slope;

    juce::Image overlay;
    juce::Rectangle<int> masterPanel, filterPanel, panPanel;
};

class SuzgecEditor : public juce::AudioProcessorEditor {
public:
    explicit SuzgecEditor(SuzgecProcessor&);
    ~SuzgecEditor() override;
    void resized() override;

    static constexpr int kBaseW = 960, kBaseH = 600;

private:
    SuzgecProcessor& proc;
    CrtLook look;
    Content content;
};

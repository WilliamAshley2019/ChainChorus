#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
// Colour palette — "BBD Bench Dark"
//==============================================================================
namespace ChainColour
{
    inline const juce::Colour BG{ 0xFF0E1116 };
    inline const juce::Colour Panel{ 0xFF1A1E26 };
    inline const juce::Colour PanelBdr{ 0xFF2A3040 };
    inline const juce::Colour Header{ 0xFF080A0E };

    inline const juce::Colour Teal{ 0xFF00D4C8 };
    inline const juce::Colour TealDim{ 0xFF005A55 };
    inline const juce::Colour Amber{ 0xFFFFB300 };
    inline const juce::Colour Magenta{ 0xFFFF0066 };
    inline const juce::Colour Phosphor{ 0xFF39FF14 };
    inline const juce::Colour IvoryText{ 0xFFD8DDE8 };
    inline const juce::Colour DimText{ 0xFF6A7080 };
    inline const juce::Colour White{ 0xFFFFFFFF };
}

//==============================================================================
// LookAndFeel
//==============================================================================
class ChainLAF : public juce::LookAndFeel_V4
{
public:
    ChainLAF();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
        float sliderPos, float startAng, float endAng,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
        bool highlight, bool down) override;

    static juce::Font monoFont(float size);
    static juce::Font displayFont(float size);
};

//==============================================================================
// Labelled knob
//==============================================================================
class ChainKnob : public juce::Component
{
public:
    ChainKnob(const juce::String& label, const juce::String& paramID,
        juce::AudioProcessorValueTreeState& apvts,
        juce::Colour accent = ChainColour::Teal);

    void resized() override;
    void paint(juce::Graphics&) override;

    juce::Slider slider;

private:
    juce::Label nameLabel, valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String paramID;
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainKnob)
};

//==============================================================================
// Mode selector pill row
//==============================================================================
class ModeSelector : public juce::Component,
    private juce::Timer
{
public:
    ModeSelector(juce::AudioProcessorValueTreeState& apvts);
    ~ModeSelector() override;
    void resized() override;
    void paint(juce::Graphics&) override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    std::array<juce::TextButton, 4> buttons;
    juce::StringArray labels{ "OFF", "I", "II", "I+II" };
    int lastMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModeSelector)
};

//==============================================================================
// Output level meter
//==============================================================================
class LevelMeter : public juce::Component,
    private juce::Timer
{
public:
    LevelMeter(ChainChorusAudioProcessor& p);
    ~LevelMeter() override;
    void paint(juce::Graphics&) override;
    void timerCallback() override;

private:
    ChainChorusAudioProcessor& proc;
    float dispL = 0.f, dispR = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

//==============================================================================
// Main editor
//==============================================================================
class ChainChorusAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer          // <-- CHANGE 1
{
public:
    ChainChorusAudioProcessorEditor(ChainChorusAudioProcessor&);
    ~ChainChorusAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;                                    // <-- CHANGE 2

private:
    ChainChorusAudioProcessor& audioProcessor;
    ChainLAF laf;

    // Controls
    std::unique_ptr<ModeSelector> modeSelector;
    std::unique_ptr<ChainKnob> rateKnob;
    std::unique_ptr<ChainKnob> depthKnob;
    std::unique_ptr<ChainKnob> widthKnob;
    std::unique_ptr<ChainKnob> mixKnob;
    std::unique_ptr<ChainKnob> wetHpfKnob;
    std::unique_ptr<ChainKnob> analogKnob;
    std::unique_ptr<ChainKnob> mainsKnob;
    std::unique_ptr<ChainKnob> clockKnob;
    std::unique_ptr<ChainKnob> bbdDriveKnob;
    std::unique_ptr<ChainKnob> outputKnob;

    juce::ToggleButton pinkBtn{ "PINK" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pinkAttach;

    juce::ToggleButton bypassBtn{ "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    juce::TextButton abButton{ "A/B SWAP" };
    juce::TextButton captureBtn{ "CAPTURE A" };

    // ===== Preset bar =====                                          
    juce::TextButton    prevPresetButton{ "<" };                       
    juce::TextButton    nextPresetButton{ ">" };                       
    juce::ComboBox      presetSelector;                               
    juce::TextButton    savePresetButton{ "SAVE" };                     
    juce::TextButton    deletePresetButton{ "DEL" };                   
    juce::TextButton    exportPresetButton{ "EXP" };                   
    juce::TextButton    openFolderButton{ "DIR" };                     

    bool updatingPresetSelector = false;                                
    int  lastKnownPresetIndex = -1;                                     

    std::unique_ptr<LevelMeter> meter;

    void drawHeader(juce::Graphics&, juce::Rectangle<int>);

    // Preset helper functions                                           
    void refreshPresetList();                                           
    void onPresetSelected(int comboItemId);                             
    void onProcessorPresetChanged(int newIndex);                        
    void onSavePresetClicked();                                         
    void onDeletePresetClicked();                                       
    void onExportPresetClicked();                                       
    void onOpenFolderClicked();                                         

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainChorusAudioProcessorEditor)
};
#pragma once
#include <JuceHeader.h>

//==============================================================================
// ChainChorus — Preset Manager
//
// Owns the factory preset table, the user preset folder, and the logic to
// move preset data in and out of the APVTS. All preset I/O happens on the
// message thread only — nothing in this class touches the audio thread.
//
// File format: single-file XML, one .chainchorus file per user preset.
// Factory presets are baked into the binary as C++ data (see PresetManager.cpp).
//==============================================================================

class PresetManager
{
public:
    //==========================================================================
    // One entry in the available-presets list.
    //==========================================================================
    struct PresetInfo
    {
        juce::String  name;         // display name, e.g. "Gentle Thickening"
        juce::String  category;     // e.g. "Subtle", "Vintage", "Creative"
        juce::String  author;       // e.g. "Factory" or a user name
        bool          isFactory = true;
        int           factoryIndex = -1;  // valid iff isFactory
        juce::File    file;               // valid iff !isFactory
    };

    //==========================================================================
    // Factory preset table entry.
    //==========================================================================
    struct FactoryPreset
    {
        juce::String name;
        juce::String category;
        juce::String author;
        std::vector<std::pair<juce::String, float>> params;
    };

    //==========================================================================
    // Construction / lifecycle
    //==========================================================================
    PresetManager(juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager() = default;

    // Rescans the user preset folder and rebuilds the available-presets
    // list. Call this once at startup, and again after any save/delete so
    // the list stays in sync with the disk.
    void refresh();

    //==========================================================================
    // Queries
    //==========================================================================
    int                            getNumPresets() const;
    const PresetInfo& getPresetInfo(int index) const;
    const std::vector<PresetInfo>& getAllPresets() const { return presets; }
    const std::vector<PresetInfo>& listAll()       const { return presets; }

    // Best-effort match against the current APVTS state. Returns -1 if no
    // factory preset matches exactly.
    int getCurrentPresetIndex() const;

    // Expose the factory table.
    static const std::vector<FactoryPreset>& getFactoryPresets();

    //==========================================================================
    // Loading
    //==========================================================================
    bool loadPreset(int index);
    bool loadPresetFromFile(const juce::File& file);

    //==========================================================================
    // Saving
    //==========================================================================
    juce::File saveUserPreset(const juce::String& name,
        const juce::String& category,
        const juce::String& author,
        bool allowOverwrite = false);

    bool exportPresetToFile(const juce::File& destination,
        const juce::String& name,
        const juce::String& category,
        const juce::String& author);

    //==========================================================================
    // Deleting
    //==========================================================================
    bool deletePreset(int index);

    //==========================================================================
    // Utility
    //==========================================================================
    juce::File getUserPresetDirectory() const;
    bool applyPresetXml(const juce::XmlElement& xml);

    std::unique_ptr<juce::XmlElement> writePresetXml(const juce::String& name,
        const juce::String& category,
        const juce::String& author) const;

private:
    void writeParametersToXml(juce::XmlElement& parent) const;
    void applyParametersFromXml(const juce::XmlElement& parent);
    void scanUserPresets();

    bool readUserPresetMetadata(const juce::File& file,
        juce::String& name,
        juce::String& category,
        juce::String& author) const;

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetInfo> presets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
#include "PresetManager.h"
#include "PluginProcessor.h"

//==============================================================================
// Factory preset bank
//
// Categories are ordered: Subtle → Classic → Instrument → Creative.
// Each preset is a small list of {paramID, naturalValue} pairs. Any
// parameter not listed will keep its current value at load time — this
// lets a preset be a "delta" on top of a known baseline. For a full
// preset you should list every parameter, but the convenience of
// partial presets is worth the tradeoff for factory banks.
//==============================================================================
namespace
{
    using P = std::pair<const char*, float>;

    // Convenience: preset builder
    PresetManager::FactoryPreset makePreset(const char* name,
        const char* category,
        std::initializer_list<P> pairs)
    {
        PresetManager::FactoryPreset fp;
        fp.name = name;
        fp.category = category;
        fp.author = "Factory";
        for (auto& p : pairs)
            fp.params.push_back({ juce::String(p.first), p.second });
        return fp;
    }
}

const std::vector<PresetManager::FactoryPreset>&
PresetManager::getFactoryPresets()
{
    using namespace ChainChorusParams;

    static const std::vector<FactoryPreset> presets = []()
        {
            std::vector<FactoryPreset> v;

            // ===========================================================
            // SUBTLE / MIX-READY
            // ===========================================================
            v.push_back(makePreset("Gentle Thickening", "Subtle", {
                { MODE, 1.f },{ RATE, 0.65f },{ DEPTH, 0.35f },{ WIDTH, 0.25f },
                { MIX, 0.20f },{ WET_HPF, 100.f },{ ANALOG, 0.30f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 25.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Vocal Double", "Subtle", {
                { MODE, 1.f },{ RATE, 0.60f },{ DEPTH, 0.35f },{ WIDTH, 0.40f },
                { MIX, 0.18f },{ WET_HPF, 140.f },{ ANALOG, 0.30f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 25.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Bass-Safe Chorus", "Subtle", {
                { MODE, 1.f },{ RATE, 0.55f },{ DEPTH, 0.25f },{ WIDTH, 0.20f },
                { MIX, 0.18f },{ WET_HPF, 180.f },{ ANALOG, 0.25f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 20.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Slow Dimension", "Subtle", {
                { MODE, 3.f },{ RATE, 0.35f },{ DEPTH, 0.30f },{ WIDTH, 0.65f },
                { MIX, 0.30f },{ WET_HPF, 100.f },{ ANALOG, 0.45f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 30.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            // ===========================================================
            // CLASSIC / ENSEMBLE
            // ===========================================================
            v.push_back(makePreset("Electric Piano Silk", "Classic", {
                { MODE, 3.f },{ RATE, 0.65f },{ DEPTH, 0.60f },{ WIDTH, 0.55f },
                { MIX, 0.30f },{ WET_HPF, 80.f },{ ANALOG, 0.45f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 35.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Clean Guitar Shimmer", "Classic", {
                { MODE, 1.f },{ RATE, 0.80f },{ DEPTH, 0.55f },{ WIDTH, 0.60f },
                { MIX, 0.28f },{ WET_HPF, 120.f },{ ANALOG, 0.35f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 30.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Warm Synth Pad", "Classic", {
                { MODE, 3.f },{ RATE, 0.45f },{ DEPTH, 0.65f },{ WIDTH, 0.75f },
                { MIX, 0.35f },{ WET_HPF, 100.f },{ ANALOG, 0.60f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 45.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Vintage Ensemble", "Classic", {
                { MODE, 2.f },{ RATE, 0.65f },{ DEPTH, 0.75f },{ WIDTH, 0.70f },
                { MIX, 0.38f },{ WET_HPF, 90.f },{ ANALOG, 0.65f },
                { MAINS, 0.55f },{ CLOCK, 0.55f },{ BBD_DRIVE, 50.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Juno 106 Classic", "Classic", {
                { MODE, 1.f },{ RATE, 1.00f },{ DEPTH, 1.00f },{ WIDTH, 0.50f },
                { MIX, 1.00f },{ WET_HPF, 0.f },{ ANALOG, 0.50f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 50.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            // ===========================================================
            // INSTRUMENT-SPECIFIC
            // ===========================================================
            v.push_back(makePreset("Wide Rhythm Guitar", "Instrument", {
                { MODE, 3.f },{ RATE, 0.90f },{ DEPTH, 0.70f },{ WIDTH, 0.85f },
                { MIX, 0.35f },{ WET_HPF, 120.f },{ ANALOG, 0.45f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 40.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Backing Vocal Spread", "Instrument", {
                { MODE, 3.f },{ RATE, 0.75f },{ DEPTH, 0.50f },{ WIDTH, 0.80f },
                { MIX, 0.26f },{ WET_HPF, 160.f },{ ANALOG, 0.40f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 35.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Stereo Keys", "Instrument", {
                { MODE, 3.f },{ RATE, 0.55f },{ DEPTH, 0.50f },{ WIDTH, 0.90f },
                { MIX, 0.30f },{ WET_HPF, 110.f },{ ANALOG, 0.50f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 40.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Chorus Bass", "Instrument", {
                { MODE, 1.f },{ RATE, 0.50f },{ DEPTH, 0.40f },{ WIDTH, 0.35f },
                { MIX, 0.22f },{ WET_HPF, 220.f },{ ANALOG, 0.40f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 35.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            // ===========================================================
            // CREATIVE / SOUND DESIGN
            // ===========================================================
            v.push_back(makePreset("Fast Modulation", "Creative", {
                { MODE, 3.f },{ RATE, 1.60f },{ DEPTH, 0.65f },{ WIDTH, 0.65f },
                { MIX, 0.30f },{ WET_HPF, 120.f },{ ANALOG, 0.40f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 35.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Deep Swirl", "Creative", {
                { MODE, 3.f },{ RATE, 1.20f },{ DEPTH, 1.35f },{ WIDTH, 0.90f },
                { MIX, 0.50f },{ WET_HPF, 80.f },{ ANALOG, 0.65f },
                { MAINS, 0.55f },{ CLOCK, 0.60f },{ BBD_DRIVE, 60.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Vibrato-Like Motion", "Creative", {
                { MODE, 2.f },{ RATE, 1.40f },{ DEPTH, 1.60f },{ WIDTH, 0.45f },
                { MIX, 0.55f },{ WET_HPF, 120.f },{ ANALOG, 0.50f },
                { MAINS, 0.50f },{ CLOCK, 0.50f },{ BBD_DRIVE, 45.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Lo-Fi BBD Texture", "Creative", {
                { MODE, 2.f },{ RATE, 0.70f },{ DEPTH, 0.75f },{ WIDTH, 0.55f },
                { MIX, 0.35f },{ WET_HPF, 220.f },{ ANALOG, 0.85f },
                { MAINS, 0.70f },{ CLOCK, 0.75f },{ BBD_DRIVE, 75.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Noisy Vintage Machine", "Creative", {
                { MODE, 2.f },{ RATE, 0.60f },{ DEPTH, 0.55f },{ WIDTH, 0.60f },
                { MIX, 0.30f },{ WET_HPF, 200.f },{ ANALOG, 0.90f },
                { MAINS, 0.80f },{ CLOCK, 0.85f },{ BBD_DRIVE, 65.f },
                { PINK, 1.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            v.push_back(makePreset("Extreme Stereo Wash", "Creative", {
                { MODE, 3.f },{ RATE, 0.85f },{ DEPTH, 1.25f },{ WIDTH, 1.00f },
                { MIX, 0.55f },{ WET_HPF, 150.f },{ ANALOG, 0.65f },
                { MAINS, 0.55f },{ CLOCK, 0.55f },{ BBD_DRIVE, 60.f },
                { PINK, 0.f },{ OUTPUT, 0.f },{ BYPASS, 0.f }
                }));

            return v;
        }();

    return presets;
}

//==============================================================================
// Constructor
//==============================================================================
PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a)
    : apvts(a)
{
    refresh();
}

//==============================================================================
// refresh
//==============================================================================
void PresetManager::refresh()
{
    presets.clear();

    // Factory presets first
    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int)factory.size(); ++i)
    {
        PresetInfo info;
        info.name = factory[(size_t)i].name;
        info.category = factory[(size_t)i].category;
        info.author = factory[(size_t)i].author;
        info.isFactory = true;
        info.factoryIndex = i;
        presets.push_back(info);
    }

    // Then user presets from disk
    scanUserPresets();
}

//==============================================================================
// scanUserPresets
//==============================================================================
void PresetManager::scanUserPresets()
{
    auto dir = getUserPresetDirectory();
    if (!dir.isDirectory()) return;

    juce::Array<juce::File> files;
    dir.findChildFiles(files, juce::File::findFiles, false, "*.chainchorus");

    // Sort alphabetically
    std::sort(files.begin(), files.end(),
        [](const juce::File& a, const juce::File& b)
        {
            return a.getFileNameWithoutExtension().compareNatural(
                b.getFileNameWithoutExtension()) < 0;
        });

    for (auto& f : files)
    {
        juce::String name, category, author;
        if (readUserPresetMetadata(f, name, category, author))
        {
            PresetInfo info;
            info.name = name;
            info.category = category;
            info.author = author;
            info.isFactory = false;
            info.factoryIndex = -1;
            info.file = f;
            presets.push_back(info);
        }
    }
}

//==============================================================================
// Query helpers
//==============================================================================
int PresetManager::getNumPresets() const
{
    return (int)presets.size();
}

const PresetManager::PresetInfo& PresetManager::getPresetInfo(int index) const
{
    jassert(index >= 0 && index < (int)presets.size());
    return presets[(size_t)index];
}

//==============================================================================
// getCurrentPresetIndex — best-effort match against current APVTS state
//==============================================================================
int PresetManager::getCurrentPresetIndex() const
{
    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int)factory.size(); ++i)
    {
        bool match = true;
        for (auto& kv : factory[(size_t)i].params)
        {
            auto* param = apvts.getParameter(kv.first);
            if (!param) { match = false; break; }
            const float natural = param->getNormalisableRange().convertFrom0to1(
                param->getValue());
            if (std::abs(natural - kv.second) > 1e-3f)
            {
                match = false;
                break;
            }
        }
        if (match) return i;
    }
    return -1;
}

//==============================================================================
// loadPreset
//==============================================================================
bool PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= (int)presets.size()) return false;
    const auto& info = presets[(size_t)index];

    if (info.isFactory)
    {
        const auto& factory = getFactoryPresets();
        if (info.factoryIndex < 0 || info.factoryIndex >= (int)factory.size())
            return false;

        for (auto& kv : factory[(size_t)info.factoryIndex].params)
        {
            if (auto* p = apvts.getParameter(kv.first))
                p->setValueNotifyingHost(
                    p->getNormalisableRange().convertTo0to1(kv.second));
        }
        return true;
    }
    else
    {
        return loadPresetFromFile(info.file);
    }
}

//==============================================================================
// loadPresetFromFile
//==============================================================================
bool PresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr) return false;
    return applyPresetXml(*xml);
}

//==============================================================================
// saveUserPreset
//==============================================================================
juce::File PresetManager::saveUserPreset(const juce::String& name,
    const juce::String& category,
    const juce::String& author,
    bool allowOverwrite)
{
    auto dir = getUserPresetDirectory();
    if (!dir.isDirectory())
        dir.createDirectory();

    auto safeName = juce::File::createLegalFileName(name);
    auto target = dir.getChildFile(safeName + ".chainchorus");

    if (target.existsAsFile() && !allowOverwrite)
        return {};

    if (auto xml = writePresetXml(name, category, author))
        xml->writeTo(target);

    refresh();
    return target;
}

//==============================================================================
// exportPresetToFile
//==============================================================================
bool PresetManager::exportPresetToFile(const juce::File& destination,
    const juce::String& name,
    const juce::String& category,
    const juce::String& author)
{
    if (auto xml = writePresetXml(name, category, author))
        return xml->writeTo(destination);
    return false;
}

//==============================================================================
// deletePreset
//==============================================================================
bool PresetManager::deletePreset(int index)
{
    if (index < 0 || index >= (int)presets.size()) return false;
    const auto& info = presets[(size_t)index];
    if (info.isFactory) return false;

    if (info.file.existsAsFile())
    {
        info.file.deleteFile();
        refresh();
        return true;
    }
    return false;
}

//==============================================================================
// getUserPresetDirectory
//==============================================================================
juce::File PresetManager::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ChainChorus")
        .getChildFile("Presets");
}

//==============================================================================
// XML serialization
//==============================================================================
void PresetManager::writeParametersToXml(juce::XmlElement& parent) const
{
    auto* paramsXml = parent.createNewChildElement("PARAMETERS");
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            auto* pXml = paramsXml->createNewChildElement("PARAM");
            pXml->setAttribute("id", p->getParameterID());
            pXml->setAttribute("value", (double)p->getValue());
        }
    }
}

void PresetManager::applyParametersFromXml(const juce::XmlElement& parent)
{
    if (auto* paramsXml = parent.getChildByName("PARAMETERS"))
    {
        for (auto* pXml : paramsXml->getChildWithTagNameIterator("PARAM"))
        {
            auto id = pXml->getStringAttribute("id");
            auto val = (float)pXml->getDoubleAttribute("value");
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(val);
        }
    }
}

//==============================================================================
// XML write / read
//==============================================================================
std::unique_ptr<juce::XmlElement> PresetManager::writePresetXml(
    const juce::String& name,
    const juce::String& category,
    const juce::String& author) const
{
    auto xml = std::make_unique<juce::XmlElement>("CHAINCHORUS_PRESET");
    xml->setAttribute("version", 1);
    xml->setAttribute("name", name);
    xml->setAttribute("category", category);
    xml->setAttribute("author", author);
    writeParametersToXml(*xml);
    return xml;
}

bool PresetManager::applyPresetXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName("CHAINCHORUS_PRESET")) return false;
    applyParametersFromXml(xml);
    return true;
}

bool PresetManager::readUserPresetMetadata(const juce::File& file,
    juce::String& name,
    juce::String& category,
    juce::String& author) const
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr) return false;
    if (!xml->hasTagName("CHAINCHORUS_PRESET")) return false;

    name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
    category = xml->getStringAttribute("category", "User");
    author = xml->getStringAttribute("author", "User");
    return true;
}
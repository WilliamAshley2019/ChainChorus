#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetManager.h"
//==============================================================================
// Parameter layout
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChainChorusAudioProcessor::createParameterLayout()
{
    using namespace ChainChorusParams;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ MODE, 1 }, "Mode",
        juce::StringArray{ "Off", "I", "II", "I+II" }, 1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ RATE, 1 }, "Rate",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.001f, 0.5f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ DEPTH, 1 }, "Depth",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ WIDTH, 1 }, "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ MIX, 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ WET_HPF, 1 }, "Wet HPF",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.4f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ANALOG, 1 }, "Analog",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ MAINS, 1 }, "Mains",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ CLOCK, 1 }, "Clock",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ BBD_DRIVE, 1 }, "BBD Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PINK, 1 }, "Pink Noise", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ OUTPUT, 1 }, "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ BYPASS, 1 }, "Bypass", false));

    return layout;
}

//==============================================================================
// Constructor / destructor
//==============================================================================
ChainChorusAudioProcessor::ChainChorusAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "ChainChorusState", createParameterLayout())
{
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.addParameterListener(p->getParameterID(), this);
    }
    presetManager = std::make_unique<PresetManager>(apvts);
}

ChainChorusAudioProcessor::~ChainChorusAudioProcessor()
{
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.removeParameterListener(p->getParameterID(), this);
    }
    presetManager.reset();
}

//==============================================================================
// prepareToPlay / releaseResources
//==============================================================================
void ChainChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jlimit(8000.0, 384000.0, sampleRate);
    currentBlockSize = juce::jmax(1, samplesPerBlock);

    chorus.Init((float)currentSampleRate);

    smoothedMix.reset(currentSampleRate, 0.02);
    smoothedOutput.reset(currentSampleRate, 0.02);

    // Initial state push
    updateChorusParams();
    updateMode((int)apvts.getRawParameterValue(ChainChorusParams::MODE)->load());

    levelPeakL.store(0.0f);
    levelPeakR.store(0.0f);

    isPrepared = true;
}

void ChainChorusAudioProcessor::releaseResources() { isPrepared = false; }

bool ChainChorusAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    if (in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono())
        return false;
    return true;
}

//==============================================================================
// parameterChanged
//==============================================================================
void ChainChorusAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    using namespace ChainChorusParams;
    if (paramID == MODE)
        updateMode((int)newValue);
    else
        updateChorusParams();
}

//==============================================================================
// Param sync into chorus
//==============================================================================
void ChainChorusAudioProcessor::updateMode(int mode)
{
    chorus.SetMode(juce::jlimit(0, 3, mode));
}

void ChainChorusAudioProcessor::updateChorusParams()
{
    using namespace ChainChorusParams;

    const float rate = apvts.getRawParameterValue(RATE)->load();
    const float depth = apvts.getRawParameterValue(DEPTH)->load();
    const float width = apvts.getRawParameterValue(WIDTH)->load();
    const float analog = apvts.getRawParameterValue(ANALOG)->load();
    const float mains = apvts.getRawParameterValue(MAINS)->load();
    const float clock = apvts.getRawParameterValue(CLOCK)->load();
    const float bbdDrv = apvts.getRawParameterValue(BBD_DRIVE)->load();
    const bool pink = apvts.getRawParameterValue(PINK)->load() > 0.5f;
    const float wetHpf = apvts.getRawParameterValue(WET_HPF)->load();

    chorus.mUserRateMul = juce::jlimit(0.25f, 4.0f, rate);
    chorus.mUserDepthMul = depth;
    chorus.mUserWidth = width;

    // Analog noise scale (0..1 -> ~0..2x dry character)
    chorus.mAnalogMul = 0.25f + analog * 1.75f;
    chorus.mMainsMul = 0.1f + mains * 1.9f;
    chorus.mClockMul = 0.1f + clock * 1.9f;

    chorus.mBbdDriveUser = bbdDrv;
    chorus.updateBbdDrive();

    chorus.mWetPink0.mPinkEnabled = pink;
    chorus.mWetPink1.mPinkEnabled = pink;

    // Wet HPF
    if (wetHpf <= 1.0f)
        wetHpfCoeff = 0.f;
    else
        wetHpfCoeff = 1.f - std::exp(-2.f * (float)juce::MathConstants<double>::pi *
            std::min(wetHpf, (float)currentSampleRate * 0.45f) / (float)currentSampleRate);

    // Refresh LFO rate (mode configure uses mUserRateMul)
    const int currentMode = (int)apvts.getRawParameterValue(MODE)->load();
    if (currentMode > 0)
        chorus.SetMode(currentMode);
}

//==============================================================================
// processBlock
//==============================================================================
void ChainChorusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (!isPrepared) return;

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples <= 0 || numCh < 1) return;

    using namespace ChainChorusParams;

    const bool bypassed = apvts.getRawParameterValue(BYPASS)->load() > 0.5f;
    const int mode = (int)apvts.getRawParameterValue(MODE)->load();
    const float mix = apvts.getRawParameterValue(MIX)->load();
    const float outDb = apvts.getRawParameterValue(OUTPUT)->load();

    // Per-block updates
    updateChorusParams();
    chorus.mUserMix = mix;
    chorus.SetWetHpf(apvts.getRawParameterValue(WET_HPF)->load(), (float)currentSampleRate);

    smoothedMix.setTargetValue(mix);
    smoothedOutput.setTargetValue(juce::Decibels::decibelsToGain(outDb));

    float* left = buffer.getWritePointer(0);
    float* right = (numCh >= 2) ? buffer.getWritePointer(1) : left;

    // If bypassed or mode == Off, just apply output gain + meter
    if (bypassed || mode == 0)
    {
        const float g = smoothedOutput.getNextValue();
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] *= g;
            if (numCh >= 2) right[i] *= g;
        }
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = (numCh >= 2) ? right[i] : inL;

            // Process mono-summed input into the BBD chorus, but feed left/right
            // separately so the chorus has full stereo context.
            float outL = inL, outR = inR;
            chorus.Process(inL, outL, outR);

            const float g = smoothedOutput.getNextValue();
            left[i] = outL * g;
            if (numCh >= 2) right[i] = outR * g;
        }
    }

    // Meter
    float peakL = 0.0f, peakR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        peakL = juce::jmax(peakL, std::abs(left[i]));
        if (numCh >= 2) peakR = juce::jmax(peakR, std::abs(right[i]));
    }
    levelPeakL.store(peakL);
    levelPeakR.store(peakR);
}

//==============================================================================
// A/B snapshot
//==============================================================================
void ChainChorusAudioProcessor::captureAState()
{
    aState = apvts.copyState();
    aStateValid = true;
}

void ChainChorusAudioProcessor::swapAB()
{
    if (!aStateValid) return;
    auto current = apvts.copyState();
    apvts.replaceState(aState);
    aState = current;
}

//==============================================================================
// State
//==============================================================================
void ChainChorusAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChainChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// Editor
//==============================================================================
juce::AudioProcessorEditor* ChainChorusAudioProcessor::createEditor()
{
    return new ChainChorusAudioProcessorEditor(*this);
}
//==============================================================================
// Preset navigation
//==============================================================================
void ChainChorusAudioProcessor::loadPresetByIndex(int index)
{
    if (!presetManager) return;
    if (index < 0 || index >= presetManager->getNumPresets()) return;

    presetManager->loadPreset(index);
    currentPresetIndex = index;

    if (onPresetChanged) onPresetChanged(index);
}

void ChainChorusAudioProcessor::loadPreviousPreset()
{
    if (!presetManager) return;
    int n = presetManager->getNumPresets();
    if (n == 0) return;
    int idx = (currentPresetIndex <= 0) ? n - 1 : currentPresetIndex - 1;
    loadPresetByIndex(idx);
}

void ChainChorusAudioProcessor::loadNextPreset()
{
    if (!presetManager) return;
    int n = presetManager->getNumPresets();
    if (n == 0) return;
    int idx = (currentPresetIndex + 1) % n;
    loadPresetByIndex(idx);
}
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChainChorusAudioProcessor();
}
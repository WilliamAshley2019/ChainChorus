#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

//==============================================================================
// ChainLAF
//==============================================================================
ChainLAF::ChainLAF()
{
    setColour(juce::Slider::thumbColourId, ChainColour::Teal);
    setColour(juce::Slider::rotarySliderFillColourId, ChainColour::Teal);
    setColour(juce::Slider::trackColourId, ChainColour::TealDim);
    setColour(juce::Label::textColourId, ChainColour::IvoryText);
    setColour(juce::ComboBox::backgroundColourId, ChainColour::Panel);
    setColour(juce::ComboBox::textColourId, ChainColour::IvoryText);
    setColour(juce::ComboBox::outlineColourId, ChainColour::PanelBdr);
    setColour(juce::ComboBox::arrowColourId, ChainColour::Teal);
    setColour(juce::PopupMenu::backgroundColourId, ChainColour::Panel);
    setColour(juce::PopupMenu::textColourId, ChainColour::IvoryText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ChainColour::TealDim);
    setColour(juce::TextButton::buttonColourId, ChainColour::Panel);
    setColour(juce::TextButton::textColourOffId, ChainColour::Teal);
    setColour(juce::TextButton::textColourOnId, ChainColour::BG);
    setColour(juce::ToggleButton::textColourId, ChainColour::IvoryText);
    setColour(juce::ToggleButton::tickDisabledColourId, ChainColour::DimText);
    setColour(juce::ToggleButton::tickColourId, ChainColour::Teal);
}

juce::Font ChainLAF::monoFont(float size)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
        size, juce::Font::plain));
}

juce::Font ChainLAF::displayFont(float size)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
        size, juce::Font::bold));
}

void ChainLAF::drawRotarySlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float sliderPos, float startAng, float endAng,
    juce::Slider& slider)
{
    const float radius = (float)juce::jmin(w, h) * 0.5f - 4.0f;
    const float centreX = (float)x + (float)w * 0.5f;
    const float centreY = (float)y + (float)h * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = startAng + sliderPos * (endAng - startAng);

    g.setColour(ChainColour::BG);
    g.fillEllipse(rx, ry, rw, rw);
    g.setColour(ChainColour::PanelBdr);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);

    const juce::Colour accentCol = slider.findColour(juce::Slider::rotarySliderFillColourId);
    {
        juce::Path trackArc;
        trackArc.addArc(rx + 3.0f, ry + 3.0f, rw - 6.0f, rw - 6.0f,
            startAng, endAng, true);
        g.setColour(accentCol.darker(1.4f));
        g.strokePath(trackArc, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    {
        juce::Path fillArc;
        fillArc.addArc(rx + 3.0f, ry + 3.0f, rw - 6.0f, rw - 6.0f,
            startAng, angle, true);
        g.setColour(accentCol);
        g.strokePath(fillArc, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    {
        juce::Path pointer;
        const float pointerLen = radius * 0.55f;
        const float pointerW = 2.0f;
        pointer.addRectangle(-pointerW * 0.5f, -radius + 4.0f, pointerW, pointerLen);
        pointer.applyTransform(juce::AffineTransform::rotation(angle)
            .translated(centreX, centreY));
        g.setColour(ChainColour::White);
        g.fillPath(pointer);
    }
    g.setColour(accentCol);
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}

void ChainLAF::drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
    bool, bool)
{
    const bool isOn = btn.getToggleState();
    const auto bounds = btn.getLocalBounds().toFloat();

    const float rx = 4.0f, h = bounds.getHeight() - 8.0f;
    juce::Rectangle<float> pill(rx, 4.0f, h * 2.0f, h);
    g.setColour(isOn ? ChainColour::Teal : ChainColour::PanelBdr);
    g.fillRoundedRectangle(pill, h * 0.5f);

    const float thumbX = isOn ? pill.getRight() - h - 2.0f : pill.getX() + 2.0f;
    g.setColour(isOn ? ChainColour::BG : ChainColour::DimText);
    g.fillEllipse(thumbX, pill.getY() + 2.0f, h - 4.0f, h - 4.0f);

    g.setColour(isOn ? ChainColour::IvoryText : ChainColour::DimText);
    g.setFont(monoFont(11.0f));
    g.drawText(btn.getButtonText(),
        (int)pill.getRight() + 6, 0,
        btn.getWidth() - (int)pill.getRight() - 6, btn.getHeight(),
        juce::Justification::centredLeft, true);
}

//==============================================================================
// ChainKnob
//==============================================================================
ChainKnob::ChainKnob(const juce::String& labelStr, const juce::String& pid,
    juce::AudioProcessorValueTreeState& apvts, juce::Colour accentCol)
    : paramID(pid), apvtsRef(apvts), accent(accentCol)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, false, nullptr);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accentCol);
    slider.setColour(juce::Slider::thumbColourId, accentCol);
    addAndMakeVisible(slider);

    nameLabel.setText(labelStr, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, ChainColour::DimText);
    nameLabel.setFont(ChainLAF::monoFont(10.0f));
    addAndMakeVisible(nameLabel);

    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, accentCol);
    valueLabel.setFont(ChainLAF::monoFont(10.0f));
    addAndMakeVisible(valueLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, pid, slider);

    slider.onValueChange = [this]()
        {
            if (auto* param = apvtsRef.getParameter(paramID))
                valueLabel.setText(param->getText(param->getValue(), 6),
                    juce::dontSendNotification);
        };
    slider.onValueChange();
}

void ChainKnob::resized()
{
    auto b = getLocalBounds();
    const int labelH = 14;
    nameLabel.setBounds(b.removeFromTop(labelH));
    valueLabel.setBounds(b.removeFromBottom(labelH));
    slider.setBounds(b.reduced(2));
}

void ChainKnob::paint(juce::Graphics&) {}

//==============================================================================
// ModeSelector
//==============================================================================
ModeSelector::ModeSelector(juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef(apvts)
{
    for (int i = 0; i < 4; ++i)
    {
        buttons[(size_t)i].setButtonText(labels[i]);
        buttons[(size_t)i].setClickingTogglesState(false);
        buttons[(size_t)i].onClick = [this, i]()
            {
                if (auto* p = apvtsRef.getParameter(ChainChorusParams::MODE))
                    p->setValueNotifyingHost(
                        p->getNormalisableRange().convertTo0to1((float)i));
                repaint();
            };
        addAndMakeVisible(buttons[(size_t)i]);
    }
    startTimerHz(15);
}

ModeSelector::~ModeSelector()
{
    stopTimer();
}

void ModeSelector::resized()
{
    auto b = getLocalBounds().reduced(2);
    const int w = b.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        buttons[(size_t)i].setBounds(b.removeFromLeft(w).reduced(1));
}

void ModeSelector::timerCallback()
{
    const int current = (int)apvtsRef
        .getRawParameterValue(ChainChorusParams::MODE)->load();
    if (current != lastMode)
    {
        lastMode = current;
        repaint();
    }
}

void ModeSelector::paint(juce::Graphics& g)
{
    const int current = (int)apvtsRef
        .getRawParameterValue(ChainChorusParams::MODE)->load();
    lastMode = current;

    for (int i = 0; i < 4; ++i)
    {
        const bool on = (i == current);
        buttons[(size_t)i].setColour(juce::TextButton::buttonColourId,
            on ? ChainColour::TealDim : ChainColour::Panel);
        buttons[(size_t)i].setColour(juce::TextButton::textColourOffId,
            on ? ChainColour::White : ChainColour::DimText);
    }
    g.setColour(ChainColour::PanelBdr);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.0f);
}

//==============================================================================
// LevelMeter
//==============================================================================
LevelMeter::LevelMeter(ChainChorusAudioProcessor& p) : proc(p)
{
    startTimerHz(30);
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

void LevelMeter::timerCallback()
{
    const float inL = proc.levelPeakL.load();
    const float inR = proc.levelPeakR.load();
    dispL = juce::jmax(dispL * 0.85f, inL);
    dispR = juce::jmax(dispR * 0.85f, inR);
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(ChainColour::BG);
    g.fillRoundedRectangle(b, 3.0f);
    g.setColour(ChainColour::PanelBdr);
    g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);

    const int w = getWidth() / 2;
    auto drawBar = [&](int x, float lin, juce::Colour col)
        {
            const float db = 20.0f * std::log10(juce::jmax(1e-6f, lin));
            const float norm = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
            const int h = getHeight() - 4;
            const int filled = (int)(norm * (float)h);
            const int top = 2 + h - filled;
            g.setColour(col.darker(1.6f));
            g.fillRect(x + 2, 2, w - 4, h);
            g.setColour(col);
            g.fillRect(x + 2, top, w - 4, filled);
        };
    drawBar(0, dispL, ChainColour::Teal);
    drawBar(w, dispR, ChainColour::Teal.brighter(0.25f));

    g.setFont(ChainLAF::monoFont(8.0f));
    g.setColour(ChainColour::DimText);
    g.drawText("L", 0, getHeight() - 10, w, 10, juce::Justification::centred);
    g.drawText("R", w, getHeight() - 10, w, 10, juce::Justification::centred);
}

//==============================================================================
// Editor
//==============================================================================
ChainChorusAudioProcessorEditor::ChainChorusAudioProcessorEditor(
    ChainChorusAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&laf);

    using namespace ChainChorusParams;
    auto& a = p.apvts;

    // ===== Mode selector row =====
    modeSelector = std::make_unique<ModeSelector>(a);
    addAndMakeVisible(*modeSelector);

    // ===== Knobs =====
    rateKnob = std::make_unique<ChainKnob>("RATE", RATE, a, ChainColour::Teal);
    depthKnob = std::make_unique<ChainKnob>("DEPTH", DEPTH, a, ChainColour::Teal);
    widthKnob = std::make_unique<ChainKnob>("WIDTH", WIDTH, a, ChainColour::Teal);
    mixKnob = std::make_unique<ChainKnob>("MIX", MIX, a, ChainColour::Teal);
    wetHpfKnob = std::make_unique<ChainKnob>("WET HPF", WET_HPF, a, ChainColour::Amber);
    analogKnob = std::make_unique<ChainKnob>("ANALOG", ANALOG, a, ChainColour::Phosphor);
    mainsKnob = std::make_unique<ChainKnob>("MAINS", MAINS, a, ChainColour::Phosphor);
    clockKnob = std::make_unique<ChainKnob>("CLOCK", CLOCK, a, ChainColour::Phosphor);
    bbdDriveKnob = std::make_unique<ChainKnob>("BBD DRV", BBD_DRIVE, a, ChainColour::Magenta);
    outputKnob = std::make_unique<ChainKnob>("OUTPUT", OUTPUT, a, ChainColour::IvoryText);

    addAndMakeVisible(*rateKnob);
    addAndMakeVisible(*depthKnob);
    addAndMakeVisible(*widthKnob);
    addAndMakeVisible(*mixKnob);
    addAndMakeVisible(*wetHpfKnob);
    addAndMakeVisible(*analogKnob);
    addAndMakeVisible(*mainsKnob);
    addAndMakeVisible(*clockKnob);
    addAndMakeVisible(*bbdDriveKnob);
    addAndMakeVisible(*outputKnob);

    // ===== Toggles =====
    pinkAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        a, PINK, pinkBtn);
    addAndMakeVisible(pinkBtn);

    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        a, BYPASS, bypassBtn);
    addAndMakeVisible(bypassBtn);

    // ===== A/B buttons =====
    abButton.onClick = [this]() { audioProcessor.swapAB(); };
    captureBtn.onClick = [this]() { audioProcessor.captureAState(); };
    addAndMakeVisible(abButton);
    addAndMakeVisible(captureBtn);

    // ===== Meter =====
    meter = std::make_unique<LevelMeter>(audioProcessor);
    addAndMakeVisible(*meter);

    // ===== Preset bar =====
    presetSelector.setLookAndFeel(&laf);
    presetSelector.setTextWhenNothingSelected("SELECT PRESET");

    auto styleBtn = [this](juce::TextButton& b)
        {
            b.setLookAndFeel(&laf);
            b.setColour(juce::TextButton::buttonColourId, ChainColour::Panel);
            b.setColour(juce::TextButton::textColourOffId, ChainColour::Teal);
        };
    styleBtn(prevPresetButton);
    styleBtn(nextPresetButton);
    styleBtn(savePresetButton);
    styleBtn(deletePresetButton);
    styleBtn(exportPresetButton);
    styleBtn(openFolderButton);

    addAndMakeVisible(prevPresetButton);
    addAndMakeVisible(nextPresetButton);
    addAndMakeVisible(presetSelector);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(deletePresetButton);
    addAndMakeVisible(exportPresetButton);
    addAndMakeVisible(openFolderButton);

    refreshPresetList();

    prevPresetButton.onClick = [this] { audioProcessor.loadPreviousPreset(); };
    nextPresetButton.onClick = [this] { audioProcessor.loadNextPreset(); };
    presetSelector.onChange = [this]
        {
            if (updatingPresetSelector) return;
            onPresetSelected(presetSelector.getSelectedId());
        };

    savePresetButton.onClick = [this] { onSavePresetClicked(); };
    deletePresetButton.onClick = [this] { onDeletePresetClicked(); };
    exportPresetButton.onClick = [this] { onExportPresetClicked(); };
    openFolderButton.onClick = [this] { onOpenFolderClicked(); };

    // Hook the processor's onPresetChanged so arrows/loads update the combo
    audioProcessor.onPresetChanged = [this](int newIndex) {
        onProcessorPresetChanged(newIndex);
        };

    // Poll for external preset index changes
    startTimerHz(10);

    setSize(920, 500);
    setResizable(false, false);
}

ChainChorusAudioProcessorEditor::~ChainChorusAudioProcessorEditor()
{
    stopTimer();

    // Clear the processor's callback before we tear down
    audioProcessor.onPresetChanged = nullptr;

    presetSelector.setLookAndFeel(nullptr);
    prevPresetButton.setLookAndFeel(nullptr);
    nextPresetButton.setLookAndFeel(nullptr);
    savePresetButton.setLookAndFeel(nullptr);
    deletePresetButton.setLookAndFeel(nullptr);
    exportPresetButton.setLookAndFeel(nullptr);
    openFolderButton.setLookAndFeel(nullptr);

    setLookAndFeel(nullptr);
}

void ChainChorusAudioProcessorEditor::drawHeader(juce::Graphics& g,
    juce::Rectangle<int> area)
{
    g.setColour(ChainColour::Header);
    g.fillRect(area);

    // Bottom accent line
    juce::ColourGradient accent(
        ChainColour::Teal.withAlpha(0.8f), 0.0f, (float)(area.getBottom() - 1),
        ChainColour::TealDim.withAlpha(0.4f), (float)area.getWidth(), (float)(area.getBottom() - 1),
        false);
    g.setGradientFill(accent);
    g.fillRect(0, area.getBottom() - 2, area.getWidth(), 2);

    // Title
    g.setFont(ChainLAF::displayFont(20.0f));
    g.setColour(ChainColour::IvoryText);
    g.drawText("CHAINCHORUS", area.getX() + 16, area.getY(),
        220, area.getHeight(), juce::Justification::centredLeft);

    // Subtitle
    g.setFont(ChainLAF::monoFont(9.0f));
    g.setColour(ChainColour::DimText);
    g.drawText("BBD Ensemble / Analog Character / Mains & Clock Drift",
        area.getX() + 216, area.getY(),
        320, area.getHeight(), juce::Justification::centredLeft);

    // Version
    g.setFont(ChainLAF::monoFont(9.0f));
    g.setColour(ChainColour::DimText);
    g.drawText("v1.0.0", area.getRight() - 60, area.getY(),
        54, area.getHeight(), juce::Justification::centredRight);
}

void ChainChorusAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ChainColour::BG);
    drawHeader(g, getLocalBounds().removeFromTop(44));
}

void ChainChorusAudioProcessorEditor::resized()
{
    const int headerH = 44;
    const int pad = 10;
    const int barH = 24;

    auto full = getLocalBounds();
    auto header = full.removeFromTop(headerH);
    auto body = full.reduced(pad);

    // -------------------------------------------------------------------
    // Header layout
    //
    // Left   : [title area drawn in paint(), ~200px]
    // Middle : [<] [preset combo] [>] [SAVE] [DEL] [EXP] [DIR]
    // Right  : [CAPTURE A] [A/B SWAP] [PINK] [BYPASS] [meter]
    // -------------------------------------------------------------------
    {
        auto top = header.reduced(pad, (headerH - barH) / 2);

        // --- Right cluster (from right to left) ---
        meter->setBounds(top.removeFromRight(46).reduced(0, 2));
        top.removeFromRight(8);

        bypassBtn.setBounds(top.removeFromRight(92).reduced(0, 2));
        top.removeFromRight(4);

        pinkBtn.setBounds(top.removeFromRight(70).reduced(0, 2));
        top.removeFromRight(10);

        abButton.setBounds(top.removeFromRight(84));
        top.removeFromRight(4);

        captureBtn.setBounds(top.removeFromRight(96));

        // --- Left cluster (preset bar) ---
        // Skip the title area drawn in paint()
        top.removeFromLeft(200);

        // Preset navigation
        prevPresetButton.setBounds(top.removeFromLeft(26));
        top.removeFromLeft(2);
        presetSelector.setBounds(top.removeFromLeft(220));
        top.removeFromLeft(2);
        nextPresetButton.setBounds(top.removeFromLeft(26));
        top.removeFromLeft(10);

        // Preset file ops
        savePresetButton.setBounds(top.removeFromLeft(52));
        top.removeFromLeft(4);
        deletePresetButton.setBounds(top.removeFromLeft(46));
        top.removeFromLeft(4);
        exportPresetButton.setBounds(top.removeFromLeft(46));
        top.removeFromLeft(4);
        openFolderButton.setBounds(top.removeFromLeft(46));
    }

    // -------------------------------------------------------------------
    // Mode selector row (own row, full width)
    // -------------------------------------------------------------------
    auto modeRow = body.removeFromTop(34);
    modeSelector->setBounds(modeRow);
    body.removeFromTop(pad);

    // -------------------------------------------------------------------
    // Two rows of knobs
    //
    // Row 1: Rate, Depth, Width, Mix, Wet HPF, Output  (6 knobs)
    // Row 2: Analog, Mains, Clock, BBD Drive          (4 knobs)
    //
    // Both rows use uniform slot widths so nothing overlaps regardless of
    // window size. Any unused slot space is distributed across the row.
    // -------------------------------------------------------------------
    const int rowH = 130;

    // Row 1 — six knobs, evenly spaced
    {
        auto row = body.removeFromTop(rowH);
        const int n = 6;
        const int slotW = row.getWidth() / n;
        auto place = [&](std::unique_ptr<ChainKnob>& k)
            {
                k->setBounds(row.removeFromLeft(slotW));
            };
        place(rateKnob);
        place(depthKnob);
        place(widthKnob);
        place(mixKnob);
        place(wetHpfKnob);
        place(outputKnob);
    }

    body.removeFromTop(pad);

    // Row 2 — four knobs, evenly spaced
    {
        auto row = body.removeFromTop(rowH);
        const int n = 4;
        const int slotW = row.getWidth() / n;
        auto place = [&](std::unique_ptr<ChainKnob>& k)
            {
                k->setBounds(row.removeFromLeft(slotW));
            };
        place(analogKnob);
        place(mainsKnob);
        place(clockKnob);
        place(bbdDriveKnob);
    }
}

//==============================================================================
// Preset management
//==============================================================================
void ChainChorusAudioProcessorEditor::refreshPresetList()
{
    updatingPresetSelector = true;
    presetSelector.clear(juce::dontSendNotification);

    auto& manager = audioProcessor.getPresetManager();
    const auto& all = manager.getAllPresets();

    for (size_t i = 0; i < all.size(); ++i)
    {
        const auto& info = all[i];
        juce::String display;
        if (!info.isFactory)
            display << juce::String::charToString((juce::juce_wchar)0x2022) << " ";
        display << info.name;
        if (info.category.isNotEmpty())
            display << "   [" << info.category << "]";
        presetSelector.addItem(display, (int)i + 1);
    }

    int currentIdx = audioProcessor.getCurrentPresetIndex();
    int comboId = (currentIdx >= 0 && currentIdx < presetSelector.getNumItems())
        ? currentIdx + 1
        : (presetSelector.getNumItems() > 0 ? 1 : 0);

    if (comboId > 0)
        presetSelector.setSelectedId(comboId, juce::dontSendNotification);

    lastKnownPresetIndex = currentIdx;
    updatingPresetSelector = false;
}

void ChainChorusAudioProcessorEditor::onPresetSelected(int comboItemId)
{
    if (comboItemId <= 0) return;
    const int managerIndex = comboItemId - 1;
    audioProcessor.loadPresetByIndex(managerIndex);
}

void ChainChorusAudioProcessorEditor::onProcessorPresetChanged(int newIndex)
{
    updatingPresetSelector = true;
    if (newIndex >= 0 && newIndex < presetSelector.getNumItems())
        presetSelector.setSelectedId(newIndex + 1, juce::dontSendNotification);
    updatingPresetSelector = false;
    lastKnownPresetIndex = newIndex;
}

void ChainChorusAudioProcessorEditor::timerCallback()
{
    int current = audioProcessor.getCurrentPresetIndex();
    if (current != lastKnownPresetIndex)
        onProcessorPresetChanged(current);
}

void ChainChorusAudioProcessorEditor::onSavePresetClicked()
{
    auto* window = new juce::AlertWindow("Save Preset",
        "Enter a name for the new preset:",
        juce::MessageBoxIconType::NoIcon);
    window->addTextEditor("name", "", "Name:");

    const int currentIdx = presetSelector.getSelectedId() - 1;
    if (currentIdx >= 0)
    {
        auto& manager = audioProcessor.getPresetManager();
        if (currentIdx < manager.getNumPresets())
        {
            const auto& info = manager.getPresetInfo(currentIdx);
            window->getTextEditor("name")->setText(info.name + " (copy)",
                juce::dontSendNotification);
            window->getTextEditor("name")->selectAll();
        }
    }

    window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true,
        juce::ModalCallbackFunction::create([this, window](int result)
            {
                juce::String name = window->getTextEditorContents("name").trim();
                if (result == 0 || name.isEmpty()) return;
                auto& manager = audioProcessor.getPresetManager();
                manager.saveUserPreset(name, "User", "", true);
                refreshPresetList();
            }), true);
}

void ChainChorusAudioProcessorEditor::onDeletePresetClicked()
{
    const int currentIdx = presetSelector.getSelectedId() - 1;
    if (currentIdx < 0) return;
    auto& manager = audioProcessor.getPresetManager();
    const auto& info = manager.getPresetInfo(currentIdx);
    if (info.isFactory)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Cannot Delete", "Factory presets cannot be deleted.");
        return;
    }
    manager.deletePreset(currentIdx);
    refreshPresetList();
}

void ChainChorusAudioProcessorEditor::onExportPresetClicked()
{
    juce::String suggestedName = "Preset";
    const int currentIdx = presetSelector.getSelectedId() - 1;
    if (currentIdx >= 0)
    {
        auto& manager = audioProcessor.getPresetManager();
        if (currentIdx < manager.getNumPresets())
            suggestedName = manager.getPresetInfo(currentIdx).name;
    }

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Preset As...",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(suggestedName + ".chainchorus"),
        "*.chainchorus");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser, suggestedName](const juce::FileChooser& fc)
        {
            auto chosen = fc.getResult();
            if (chosen == juce::File()) return;
            if (!chosen.hasFileExtension("chainchorus"))
                chosen = chosen.withFileExtension("chainchorus");
            audioProcessor.getPresetManager().exportPresetToFile(chosen,
                suggestedName, "User", "");
        });
}

void ChainChorusAudioProcessorEditor::onOpenFolderClicked()
{
    audioProcessor.getPresetManager().getUserPresetDirectory().revealToUser();
}
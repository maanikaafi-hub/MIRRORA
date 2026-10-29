/*
    MIRRORA — editor.

    The whole interface is drawn in a fixed 960 x 570 "design space" and then
    scaled with a transform, so it looks identical at every window size and on
    every display / DPI.
*/
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MirroraControls.h"
#include "WaveformDisplay.h"

//==============================================================================
/** Paints the static chrome: frame, header, captions, wordmark. */
class PanelBackground final : public juce::Component
{
public:
    PanelBackground() = default;
    ~PanelBackground() override = default;

    void paint (juce::Graphics&) override;

    int zoomPercent { 100 };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelBackground)
};

//==============================================================================
/** Simple "about" card, shown when the gear icon is clicked. */
class AboutOverlay final : public juce::Component
{
public:
    AboutOverlay();
    ~AboutOverlay() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
};

//==============================================================================
class MIRRORAEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MIRRORAEditor (MIRRORAProcessor&);
    ~MIRRORAEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void layoutContent();
    void setZoom (float newScale);
    float currentScale() const;

    MIRRORAProcessor& processor;

    juce::Component content;
    PanelBackground background;

    mirrora::Knob mixKnob, lengthKnob, outputKnob;

    mirrora::ModeButton simpleButton   { "SIMPLE",   mirrora::ModeButton::Glyph::reverse };
    mirrora::ModeButton rhythmicButton { "RHYTHMIC", mirrora::ModeButton::Glyph::rhythmic };

    mirrora::PillToggle syncButton { "BPM SYNC" };

    mirrora::IconButton zoomOutButton { mirrora::IconType::minus };
    mirrora::IconButton zoomInButton  { mirrora::IconType::plus };
    mirrora::IconButton infoButton    { mirrora::IconType::gear };

    mirrora::StepperRow lengthStepper, outputStepper;

    WaveformDisplay waveform;
    AboutOverlay about;

    std::unique_ptr<juce::ParameterAttachment> modeAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;

    std::unique_ptr<juce::SliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> lengthAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> outputAttachment;

    juce::TooltipWindow tooltipWindow { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIRRORAEditor)
};

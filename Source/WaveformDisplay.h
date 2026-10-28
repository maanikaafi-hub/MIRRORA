/*
    MIRRORA — the animated scope inside the dark rounded panel.
*/
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MirroraTheme.h"

class WaveformDisplay final : public juce::Component,
                              private juce::Timer
{
public:
    explicit WaveformDisplay (MIRRORAProcessor& processorToUse);
    ~WaveformDisplay() override = default;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    MIRRORAProcessor& processor;

    std::array<float, MIRRORAProcessor::scopeBins> incoming {};
    std::array<float, MIRRORAProcessor::scopeBins> displayed {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};

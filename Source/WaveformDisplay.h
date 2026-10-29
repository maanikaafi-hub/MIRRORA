/*
    MIRRORA — the animated scope inside the dark rounded panel.

    Drag from it to drop the reversed audio (as a WAV file) into the DAW,
    the desktop, a sampler, a chat window, …
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

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Writes the reversed window to a temporary WAV and starts a native drag. */
    void beginExport();

private:
    void timerCallback() override;

    /** Returns the export file, or an invalid file when nothing was captured. */
    juce::File createExportFile();

    MIRRORAProcessor& processor;

    std::array<float, MIRRORAProcessor::scopeBins> incoming {};
    std::array<float, MIRRORAProcessor::scopeBins> displayed {};

    juce::Point<int> dragStart;
    bool dragging { false };
    bool hovering { false };
    int  hintFrames { 0 };
    juce::String hintText { "DRAG TO EXPORT THE REVERSED AUDIO" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};

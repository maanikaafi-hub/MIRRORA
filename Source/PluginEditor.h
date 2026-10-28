#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class MIRRORAEditor final:public juce::AudioProcessorEditor{
public: explicit MIRRORAEditor(MIRRORAProcessor&);void paint(juce::Graphics&)override;void resized()override;
private:
 MIRRORAProcessor& proc;juce::TextButton simple{"SIMPLE REVERSE"},rhythmic{"RHYTHMIC REVERSE"};
 juce::Slider mix,length,bpm,output;juce::ToggleButton sync{"BPM SYNC"};
 juce::Label title,brand,mixL,lengthL,bpmL,outL,about;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ma,la,ba,oa;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sa;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIRRORAEditor)
};

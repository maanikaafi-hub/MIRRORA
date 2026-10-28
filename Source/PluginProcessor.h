#pragma once
#include <JuceHeader.h>
class MIRRORAProcessor final : public juce::AudioProcessor {
public:
 MIRRORAProcessor(); ~MIRRORAProcessor() override = default;
 void prepareToPlay(double, int) override; void releaseResources() override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override { return true; }
 const juce::String getName() const override { return JucePlugin_Name; }
 bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; } double getTailLengthSeconds() const override { return 10.0; }
 int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {} const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int, const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*, int) override;
 juce::AudioProcessorValueTreeState parameters;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
 double sr=44100.0; int windowSamples=22050, writePos=0, readPos=-1, active=0;
 std::array<juce::AudioBuffer<float>,2> buffers;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIRRORAProcessor)
};

/*
    MIRRORA — audio processor.

    A true "reverse" effect: the incoming signal is recorded into a window and,
    as soon as the window is complete, played back backwards (while the next
    window is being recorded).

    RHYTHMIC mode locks those windows to the musical grid reported by the host
    (PPQ), so every reversed chunk starts exactly on a 1/16, 1/8, 1/4, 1/2 …
    or triplet boundary. SIMPLE mode simply uses a fixed duration.
*/
#pragma once

#include <JuceHeader.h>

class MIRRORAProcessor final : public juce::AudioProcessor
{
public:
    MIRRORAProcessor();
    ~MIRRORAProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlockExpected) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram (int) override                {}
    const juce::String getProgramName (int) override     { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState& getState() noexcept { return parameters; }

    /** Note-value labels of the LENGTH parameter ("1/16" … "2 Bars"). */
    static juce::StringArray getLengthChoices();

    /** Number of points in the GUI waveform scope. */
    static constexpr int scopeBins = 512;

    /** Copies the scope, oldest sample first. */
    void readScope (float* destination) const noexcept;

    /** Copies the reversed audio of the current window (for drag & drop export).
        Message thread only; returns false if nothing has been captured yet.
    */
    bool copyReversedWindow (juce::AudioBuffer<float>& destination) const;

    juce::String describeExportName() const;

private:
    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void resetEngine();
    bool startNewWindow() noexcept;
    void updateScope (const juce::AudioBuffer<float>& buffer) noexcept;

    void cacheBufferPointers() noexcept;

    int lengthIndexInUse() const noexcept;
    double beatsPerWindow() const noexcept;
    double secondsPerWindow() const noexcept;

    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* modeValue   = nullptr;   // 0 = simple, 1 = rhythmic
    std::atomic<float>* mixValue    = nullptr;   // 0 .. 100 %
    std::atomic<float>* lengthValue = nullptr;   // note-value index
    std::atomic<float>* syncValue   = nullptr;   // bool
    std::atomic<float>* bpmValue    = nullptr;   // fallback tempo
    std::atomic<float>* outputValue = nullptr;   // dB

    //---- engine state -------------------------------------------------------
    double sampleRate { 44100.0 };

    std::array<juce::AudioBuffer<float>, 2> windowBuffers;
    int captureIndex { 0 };
    int playIndex    { 1 };

    float* capturePointer[2] { nullptr, nullptr };
    const float* playPointer[2] { nullptr, nullptr };

    int capturePosition { 0 };
    int captureLength   { 0 };

    int playPosition { 0 };
    int playLength   { 0 };
    double playLengthF { 0.0 };
    bool playing     { false };
    int fadeLength   { 0 };

    float* fades = nullptr;         // pre-computed cosine fade curve
    int fadeSizeInSamples { 0 };
    bool flushPending { false };

    bool rhythmicMode { false };

    // grid tracking for RHYTHMIC mode
    bool hasLastGridPosition { false };
    double lastPpq { 0.0 };

    juce::SmoothedValue<float> mixSmoothed  { 1.0f };
    juce::SmoothedValue<float> gainSmoothed { 1.0f };

    //---- drag & drop export -------------------------------------------------
    mutable juce::CriticalSection windowLock;

    //---- GUI scope ---------------------------------------------------------
    std::array<std::atomic<float>, scopeBins> scopeData {};
    std::atomic<int> scopeWritePosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIRRORAProcessor)
};

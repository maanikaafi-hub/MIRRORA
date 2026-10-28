#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    // Window length in beats for the RHYTHMIC mode ("1/4", "1/2", "1 Bar", "2 Bars").
    constexpr double kBeatLengths[] { 1.0, 2.0, 4.0, 8.0 };

    // Window length in seconds for the SIMPLE mode.
    constexpr double kPlainLengths[] { 0.25, 0.5, 1.0, 2.0 };

    // Longest window we allow (keeps the audio buffers small and predictable).
    constexpr double kMaxWindowSeconds = 8.0;
    constexpr double kBufferSeconds    = kMaxWindowSeconds + 1.0;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MIRRORAProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                                                              StringArray { "Simple", "Rhythmic" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float value, int) { return String (roundToInt (value)) + " %"; })));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "length", 1 }, "Length",
        StringArray { "1/4", "1/2", "1 Bar", "2 Bars" }, 1));

    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "BPM Sync", true));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "bpm", 1 }, "BPM",
        NormalisableRange<float> (40.0f, 240.0f, 0.1f), 120.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float value, int) { return String (value, 1) + " BPM"; })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float value, int) { return String (value, 1) + " dB"; })));

    return { params.begin(), params.end() };
}

juce::StringArray MIRRORAProcessor::getLengthChoices()
{
    return { "1/4", "1/2", "1 Bar", "2 Bars" };
}

//==============================================================================
MIRRORAProcessor::MIRRORAProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    modeValue   = parameters.getRawParameterValue ("mode");
    mixValue    = parameters.getRawParameterValue ("mix");
    lengthValue = parameters.getRawParameterValue ("length");
    syncValue   = parameters.getRawParameterValue ("sync");
    bpmValue    = parameters.getRawParameterValue ("bpm");
    outputValue = parameters.getRawParameterValue ("output");

    for (auto& value : scopeData)
        value.store (0.0f, std::memory_order_relaxed);
}

//==============================================================================
void MIRRORAProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

    const int capacity = juce::jmax (4096, (int) std::ceil (sampleRate * kBufferSeconds));

    for (auto& buffer : windowBuffers)
    {
        buffer.setSize (2, capacity, false, true, false);
        buffer.clear();
    }

    captureLength = juce::jlimit (16, capacity - 8, (int) std::round (sampleRate * 0.5));

    resetEngine();

    mixSmoothed.reset (sampleRate, 0.02);
    gainSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, mixValue->load() * 0.01f));
    gainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputValue->load(), -60.0f));

    for (auto& value : scopeData)
        value.store (0.0f, std::memory_order_relaxed);

    scopeWritePosition.store (0, std::memory_order_release);
}

void MIRRORAProcessor::releaseResources()
{
    for (auto& buffer : windowBuffers)
        buffer.setSize (0, 0);
}

void MIRRORAProcessor::resetEngine()
{
    captureIndex = 0;
    playIndex    = 1;

    capturePosition = 0;
    playPosition    = 0;
    playLength      = 0;
    playLengthF     = 0.0;
    playing         = false;
    fadeLength      = 0;
    pendingLengthF  = (double) captureLength;

    for (auto& buffer : windowBuffers)
        buffer.clear();

    cacheBufferPointers();
}

void MIRRORAProcessor::cacheBufferPointers() noexcept
{
    auto& captureBuffer = windowBuffers[(size_t) captureIndex];
    auto& playBuffer    = windowBuffers[(size_t) playIndex];

    capturePointer[0] = captureBuffer.getWritePointer (0);
    capturePointer[1] = captureBuffer.getWritePointer (1);

    playPointer[0] = playBuffer.getReadPointer (0);
    playPointer[1] = playBuffer.getReadPointer (1);
}

void MIRRORAProcessor::startNewWindow() noexcept
{
    if (capturePosition < 8)
        return;

    std::swap (captureIndex, playIndex);

    playLength  = capturePosition;
    playLengthF = (double) playLength;
    playPosition = 0;
    playing = true;

    // A short fade at both edges of the reversed window removes the click that a
    // hard wrap would otherwise produce. RHYTHMIC stays tighter than SIMPLE.
    const double fadeSeconds = rhythmicMode ? 0.0025 : 0.010;
    fadeLength = juce::jlimit (2, juce::jmax (2, playLength / 4),
                               (int) std::round (fadeSeconds * sampleRate));

    capturePosition = 0;
    cacheBufferPointers();
}

//==============================================================================
bool MIRRORAProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void MIRRORAProcessor::updateScope (const juce::AudioBuffer<float>& buffer) noexcept
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numSamples <= 0 || numChannels <= 0)
        return;

    constexpr int chunksPerBlock = 8;
    int writePosition = scopeWritePosition.load (std::memory_order_relaxed);

    for (int chunk = 0; chunk < chunksPerBlock; ++chunk)
    {
        const int start = chunk * numSamples / chunksPerBlock;
        const int end   = (chunk + 1) * numSamples / chunksPerBlock;

        float peak = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* data = buffer.getReadPointer (channel);

            for (int n = start; n < end; ++n)
                peak = juce::jmax (peak, std::abs (data[n]));
        }

        scopeData[(size_t) writePosition].store (juce::jmin (1.0f, peak), std::memory_order_relaxed);
        writePosition = (writePosition + 1) % scopeBins;
    }

    scopeWritePosition.store (writePosition, std::memory_order_release);
}

void MIRRORAProcessor::readScope (float* destination) const noexcept
{
    const int writePosition = scopeWritePosition.load (std::memory_order_acquire);

    for (int i = 0; i < scopeBins; ++i)
        destination[i] = scopeData[(size_t) ((writePosition + i) % scopeBins)].load (std::memory_order_relaxed);
}

//==============================================================================
void MIRRORAProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int totalChans  = buffer.getNumChannels();
    const int numChannels = juce::jmin (2, totalChans);

    for (int channel = numChannels; channel < totalChans; ++channel)
        buffer.clear (channel, 0, numSamples);

    if (numSamples == 0 || numChannels == 0 || windowBuffers[0].getNumSamples() < 64)
    {
        buffer.clear();
        return;
    }

    updateScope (buffer);

    //---- parameters ---------------------------------------------------------
    rhythmicMode = modeValue->load() > 0.5f;

    const bool synced      = syncValue->load() > 0.5f;
    const int  lengthIndex = juce::jlimit (0, 3, juce::roundToInt (lengthValue->load()));
    const float mix        = juce::jlimit (0.0f, 1.0f, mixValue->load() * 0.01f);
    const float gain       = juce::Decibels::decibelsToGain (outputValue->load(), -60.0f);

    double bpm = (double) bpmValue->load();

    if (synced)
    {
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition())
            {
                if (const auto hostBpm = position->getBpm())
                {
                    if (*hostBpm > 1.0)
                        bpm = *hostBpm;
                }
            }
        }
    }

    bpm = juce::jlimit (20.0, 400.0, bpm);

    const double wantedSeconds = juce::jlimit (0.01, kMaxWindowSeconds,
                                               rhythmicMode ? (60.0 / bpm) * kBeatLengths[lengthIndex]
                                                            : kPlainLengths[lengthIndex]);

    const double maxSamples = (double) windowBuffers[(size_t) captureIndex].getNumSamples() - 8.0;
    const int wantedLength  = juce::jlimit (16, (int) maxSamples,
                                            (int) std::round (wantedSeconds * sampleRate));

    captureLength = wantedLength;

    if (capturePosition >= captureLength)
        startNewWindow();

    mixSmoothed.setTargetValue (mix);
    gainSmoothed.setTargetValue (gain);

    //---- audio --------------------------------------------------------------
    const float* input[2] { buffer.getReadPointer (0),
                            buffer.getReadPointer (numChannels > 1 ? 1 : 0) };

    float* output[2] { buffer.getWritePointer (0),
                       numChannels > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = input[0][n];
        const float dryR = input[1][n];

        // record the incoming audio
        capturePointer[0][capturePosition] = dryL;
        capturePointer[1][capturePosition] = dryR;
        ++capturePosition;

        // play back the previous window, backwards
        float wetL = 0.0f;
        float wetR = 0.0f;

        if (playing)
        {
            const double readPosition = playLengthF - 1.0 - (double) playPosition;

            if (readPosition >= 0.0)
            {
                const int index0     = (int) readPosition;
                const float fraction = (float) (readPosition - (double) index0);
                const int index1     = juce::jmin (playLength - 1, index0 + 1);

                wetL = playPointer[0][index0] + fraction * (playPointer[0][index1] - playPointer[0][index0]);
                wetR = playPointer[1][index0] + fraction * (playPointer[1][index1] - playPointer[1][index0]);

                if (fadeLength > 0)
                {
                    float fade = 1.0f;

                    if (playPosition < fadeLength)
                        fade = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi
                                                       * (float) playPosition / (float) fadeLength);
                    else if (playPosition > playLength - fadeLength)
                        fade = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi
                                                       * (float) (playLength - playPosition) / (float) fadeLength);

                    wetL *= fade;
                    wetR *= fade;
                }
            }

            if (++playPosition >= playLength)
                playing = false;
        }

        const float currentMix  = mixSmoothed.getNextValue();
        const float currentGain = gainSmoothed.getNextValue();

        output[0][n] = ((1.0f - currentMix) * dryL + currentMix * wetL) * currentGain;

        if (output[1] != nullptr)
            output[1][n] = ((1.0f - currentMix) * dryR + currentMix * wetR) * currentGain;

        if (capturePosition >= captureLength)
            startNewWindow();
    }
}

//==============================================================================
juce::AudioProcessorEditor* MIRRORAProcessor::createEditor()
{
    return new MIRRORAEditor (*this);
}

void MIRRORAProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MIRRORAProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIRRORAProcessor();
}

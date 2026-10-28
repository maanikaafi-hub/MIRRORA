#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    struct LengthDefinition
    {
        const char* label;
        double beats;     // quarter-note beats (4/4)
        double seconds;   // SIMPLE mode duration
    };

    // ordered from shortest to longest, triplets included
    constexpr LengthDefinition kLengths[]
    {
        { "1/16",   0.25,        0.125 },   // 0
        { "1/8T",   1.0 / 3.0,   1.0 / 6.0 }, // 1
        { "1/8",    0.5,         0.25 },    // 2
        { "1/4T",   2.0 / 3.0,   1.0 / 3.0 }, // 3
        { "1/4",    1.0,         0.5 },     // 4  (default)
        { "1/2T",   4.0 / 3.0,   2.0 / 3.0 }, // 5
        { "1/2",    2.0,         1.0 },     // 6
        { "1 Bar",  4.0,         2.0 },     // 7
        { "2 Bars", 8.0,         4.0 }      // 8
    };

    constexpr int numLengths = (int) (sizeof (kLengths) / sizeof (kLengths[0]));
    constexpr int defaultLengthIndex = 4;   // "1/4"

    // Longest window we allow (keeps the audio buffers small and predictable).
    constexpr double kMaxWindowSeconds = 8.0;
    constexpr double kBufferSeconds    = kMaxWindowSeconds + 1.0;

    int clampLengthIndex (int index) noexcept
    {
        return juce::jlimit (0, numLengths - 1, index);
    }
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

    StringArray lengthChoices;

    for (const auto& definition : kLengths)
        lengthChoices.add (definition.label);

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "length", 1 }, "Length",
                                                              lengthChoices, defaultLengthIndex));

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
    juce::StringArray choices;

    for (const auto& definition : kLengths)
        choices.add (definition.label);

    return choices;
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
int MIRRORAProcessor::lengthIndexInUse() const noexcept
{
    return clampLengthIndex (juce::roundToInt (lengthValue->load()));
}

double MIRRORAProcessor::beatsPerWindow() const noexcept
{
    return kLengths[lengthIndexInUse()].beats;
}

double MIRRORAProcessor::secondsPerWindow() const noexcept
{
    return kLengths[lengthIndexInUse()].seconds;
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

    // pre-computed cosine fade-up / fade-down curve
    {
        delete[] fades;
        fades = nullptr;

        const int fadeSize = juce::jmax (2, (int) std::round (sampleRate * 0.020));

        fades = new float[(size_t) fadeSize];
        fadeSizeInSamples = fadeSize;

        for (int i = 0; i < fadeSize; ++i)
            fades[i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) i / (float) fadeSize);
    }

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

    delete[] fades;
    fades = nullptr;
    fadeSizeInSamples = 0;
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

    hasLastGridPosition = false;
    flushPending = false;

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

bool MIRRORAProcessor::startNewWindow() noexcept
{
    if (capturePosition < 8)
        return false;

    // The editor may be copying the play buffer for drag & drop export; if so,
    // defer the swap by one block instead of stalling the audio thread.
    const juce::ScopedTryLock deferredSwap (windowLock);

    if (! deferredSwap.isLocked())
        return false;

    std::swap (captureIndex, playIndex);

    playLength   = capturePosition;
    playLengthF  = (double) playLength;
    playPosition = 0;
    playing      = true;

    // A short fade at both edges of the reversed window removes the click that a
    // hard wrap would otherwise produce. RHYTHMIC stays tighter than SIMPLE.
    const int fadeSamples = rhythmicMode ? (int) std::round (sampleRate * 0.0025)
                                         : (int) std::round (sampleRate * 0.010);

    fadeLength = juce::jlimit (2, juce::jmax (2, playLength / 3),
                               juce::jmin (fadeSamples, fadeSizeInSamples));

    capturePosition = 0;
    cacheBufferPointers();
    return true;
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
bool MIRRORAProcessor::copyReversedWindow (juce::AudioBuffer<float>& destination) const
{
    const juce::ScopedLock snapshot (windowLock);

    const int length = playLength;
    const int channels = juce::jmax (1, juce::jmin (2, windowBuffers[(size_t) playIndex].getNumChannels()));

    if (length < 8 || playPointer[0] == nullptr)
        return false;

    destination.setSize (channels, length, false, true, false);

    for (int channel = 0; channel < channels; ++channel)
        for (int i = 0; i < length; ++i)
            destination.setSample (channel, i, playPointer[channel][length - 1 - i]);

    return true;
}

juce::String MIRRORAProcessor::describeExportName() const
{
    return "MIRRORA_" + juce::String (kLengths[lengthIndexInUse()].label).replace ("/", "-")
                                 .replace (" ", "") + "_reverse";
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

    const bool synced = syncValue->load() > 0.5f;
    const float mix   = juce::jlimit (0.0f, 1.0f, mixValue->load() * 0.01f);
    const float gain  = juce::Decibels::decibelsToGain (outputValue->load(), -60.0f);

    double bpm = (double) bpmValue->load();
    double ppq = 0.0;
    bool gridLocked = false;

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto hostBpm = position->getBpm())
                if (*hostBpm > 1.0)
                    bpm = *hostBpm;

            if (synced && rhythmicMode)
            {
                if (const auto hostPpq = position->getPpqPosition())
                {
                    ppq = *hostPpq;
                    gridLocked = position->getIsPlaying();
                }
            }
        }
    }

    bpm = juce::jlimit (20.0, 400.0, bpm);

    const double beats    = beatsPerWindow();
    const double samplesPerBeat = sampleRate * 60.0 / bpm;
    const double maxSamples    = (double) windowBuffers[(size_t) captureIndex].getNumSamples() - 8.0;

    // free-running window length (SIMPLE mode, or RHYTHMIC without a grid)
    const double wantedSeconds = juce::jlimit (0.01, kMaxWindowSeconds,
                                               rhythmicMode ? beats * 60.0 / bpm
                                                            : secondsPerWindow());

    captureLength = juce::jlimit (16, (int) maxSamples, (int) std::round (wantedSeconds * sampleRate));

    //---- grid alignment (RHYTHMIC) -----------------------------------------
    // Work out, sample-accurately, where the next note-value boundary falls
    // inside this block so the reversed chunks always start on the grid.
    int boundarySample = -1;

    if (gridLocked && beats > 0.0)
    {
        const double beatsToNext = (std::floor (ppq / beats + 1.0e-9) + 1.0) * beats - ppq;
        const double samplesToNext = beatsToNext * samplesPerBeat;

        if (samplesToNext >= 0.0 && samplesToNext < (double) numSamples)
            boundarySample = juce::jlimit (0, numSamples - 1, (int) std::round (samplesToNext));

        // transport jumped (loop, seek, …) => drop the stale window
        const double expectedAdvance = (double) numSamples / samplesPerBeat * 1.5 + 0.05;

        if (hasLastGridPosition && (ppq < lastPpq - 1.0e-6 || ppq - lastPpq > expectedAdvance))
            flushPending = true;

        lastPpq = ppq;
        hasLastGridPosition = true;
    }
    else
    {
        hasLastGridPosition = false;

        if (capturePosition >= captureLength)
            flushPending = true;
    }

    if (flushPending)
        flushPending = ! startNewWindow();

    mixSmoothed.setTargetValue (mix);
    gainSmoothed.setTargetValue (gain);

    //---- audio --------------------------------------------------------------
    const float* input[2] { buffer.getReadPointer (0),
                            buffer.getReadPointer (numChannels > 1 ? 1 : 0) };

    float* output[2] { buffer.getWritePointer (0),
                       numChannels > 1 ? buffer.getWritePointer (1) : nullptr };

    const int captureCapacity = windowBuffers[(size_t) captureIndex].getNumSamples();

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = input[0][n];
        const float dryR = input[1][n];

        // record the incoming audio
        if (capturePosition < captureCapacity)
        {
            capturePointer[0][capturePosition] = dryL;
            capturePointer[1][capturePosition] = dryR;
            ++capturePosition;
        }

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

                if (fadeLength > 0 && fades != nullptr)
                {
                    int index = -1;

                    if (playPosition < fadeLength)
                        index = playPosition;
                    else if (playPosition > playLength - fadeLength)
                        index = playLength - playPosition;

                    if (index >= 0)
                    {
                        const float fade = fades[juce::jlimit (0, fadeSizeInSamples - 1, index)];
                        wetL *= fade;
                        wetR *= fade;
                    }
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

        // window boundaries
        if (boundarySample >= 0 && n >= boundarySample)
        {
            boundarySample = -1;
            flushPending = true;
        }
        else if (! gridLocked && capturePosition >= captureLength)
        {
            flushPending = true;
        }

        if (flushPending)
            flushPending = ! startNewWindow();
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

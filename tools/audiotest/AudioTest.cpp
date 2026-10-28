/*
    Offline self-test for the MIRRORA engine.

    Runs the processor without a host and checks:
      * dry passthrough when MIX = 0
      * output gain is applied
      * SIMPLE mode plays the captured window backwards, exactly one window late
      * triplet note values (1/8T, 1/4T …) produce the right window length
      * RHYTHMIC mode locks every reversed chunk to the host grid (PPQ)
      * no NaN / runaway level while automating every parameter
*/
#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
    constexpr double testSampleRate = 44100.0;
    constexpr int    blockSize      = 256;

    int failures = 0;

    void check (bool condition, const juce::String& what)
    {
        std::cout << (condition ? "  PASS  " : "  FAIL  ") << what << std::endl;

        if (! condition)
            ++failures;
    }

    /** Minimal AudioPlayHead that reports a steady tempo and a rolling PPQ. */
    struct FakePlayHead final : public juce::AudioPlayHead
    {
        FakePlayHead (double bpmToUse, double startPpq) : bpm (bpmToUse), ppq (startPpq) {}

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm (bpm);
            info.setPpqPosition (ppq);
            info.setIsPlaying (true);
            info.setTimeSignature (TimeSignature { 4, 4 });
            return info;
        }

        void advance (int numSamples) { ppq += (double) numSamples * bpm / 60.0 / testSampleRate; }

        double bpm;
        double ppq;
    };

    struct Harness
    {
        Harness (float mixPercent, int lengthIndex, bool rhythmic, float outputDb = 0.0f,
                 double bpm = 120.0, int numLengthChoices = 9)
        {
            auto& state = processor.getState();

            state.getParameter ("mix")->setValueNotifyingHost (mixPercent / 100.0f);
            state.getParameter ("length")->setValueNotifyingHost ((float) lengthIndex / (float) (numLengthChoices - 1));
            state.getParameter ("mode")->setValueNotifyingHost (rhythmic ? 1.0f : 0.0f);
            state.getParameter ("output")->setValueNotifyingHost ((outputDb + 24.0f) / 36.0f);
            state.getParameter ("bpm")->setValueNotifyingHost ((float) ((bpm - 40.0) / 200.0));
            state.getParameter ("sync")->setValueNotifyingHost (1.0f);

            processor.setPlayConfigDetails (2, 2, testSampleRate, blockSize);
            processor.prepareToPlay (testSampleRate, blockSize);
        }

        std::vector<float> run (const std::vector<float>& input, FakePlayHead* playHead = nullptr)
        {
            if (playHead != nullptr)
                processor.setPlayHead (playHead);

            std::vector<float> output (input.size(), 0.0f);
            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;

            size_t position = 0;

            while (position < input.size())
            {
                const int numSamples = (int) juce::jmin ((size_t) blockSize, input.size() - position);

                block.clear();
                block.copyFrom (0, 0, input.data() + position, numSamples);
                block.copyFrom (1, 0, input.data() + position, numSamples);

                processor.processBlock (block, midi);

                if (playHead != nullptr)
                    playHead->advance (numSamples);

                for (int n = 0; n < numSamples; ++n)
                    output[position + (size_t) n] = block.getSample (0, n);

                position += (size_t) numSamples;
            }

            if (playHead != nullptr)
                processor.setPlayHead (nullptr);

            return output;
        }

        MIRRORAProcessor processor;
    };

    float maxAbs (const std::vector<float>& v, size_t from, size_t to)
    {
        float m = 0.0f;

        for (size_t i = from; i < juce::jmin (to, v.size()); ++i)
            m = juce::jmax (m, std::abs (v[i]));

        return m;
    }

    size_t peakIndex (const std::vector<float>& v, size_t from, size_t to)
    {
        size_t best = from;
        float peak = 0.0f;

        for (size_t i = from; i < juce::jmin (to, v.size()); ++i)
        {
            if (std::abs (v[i]) > peak)
            {
                peak = std::abs (v[i]);
                best = i;
            }
        }

        return best;
    }

    /** SIMPLE mode: an impulse `offset` samples into a window comes back, reversed,
        one window later at `window + (window - 1 - offset)`. */
    void simpleReverseTest (const juce::String& label, int lengthIndex, double windowSeconds)
    {
        const int window = (int) std::round (windowSeconds * testSampleRate);
        constexpr int offset = 200;

        std::vector<float> input ((size_t) (window * 2 + 512), 0.0f);
        input[(size_t) offset] = 1.0f;

        Harness harness (100.0f, lengthIndex, false);
        const auto output = harness.run (input);

        const size_t expected = (size_t) window + (size_t) (window - 1 - offset);
        const auto measured = peakIndex (output, (size_t) window, output.size());

        check (maxAbs (output, 1, (size_t) window - 4) < 1.0e-6f,
               label + ": silent while the first window is being filled");

        check (measured == expected,
               label + ": window is " + juce::String (window) + " samples, impulse reversed to "
               + juce::String ((int) expected) + " (got " + juce::String ((int) measured) + ")");
    }

    /** RHYTHMIC mode with a host playhead: chunks must flip exactly on the grid. */
    void gridLockedTest (const juce::String& label, int lengthIndex, double beats)
    {
        const double bpm = 120.0;
        const int cell = (int) std::round (beats * 60.0 / bpm * testSampleRate);   // samples per cell

        constexpr int offsetInCell2 = 700;
        const size_t impulseAt = (size_t) (cell * 2 + offsetInCell2);

        std::vector<float> input ((size_t) (cell * 4 + 1024), 0.0f);
        input[impulseAt] = 1.0f;

        Harness harness (100.0f, lengthIndex, true, 0.0f, bpm);
        FakePlayHead playHead (bpm, 0.0);

        const auto output = harness.run (input, &playHead);

        // the cell is captured during cell 2 and played back (reversed) during cell 3
        const size_t expected = (size_t) cell * 3 + (size_t) (cell - 1 - offsetInCell2);
        const auto measured = peakIndex (output, (size_t) cell * 2, output.size());

        check (std::abs ((int) measured - (int) expected) <= 2,
               label + ": grid cell " + juce::String (cell) + " samples, reversed impulse at "
               + juce::String ((int) expected) + " (got " + juce::String ((int) measured) + ")");
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::cout << "MIRRORA engine self-test" << std::endl;

    //--------------------------------------------------------------------------
    std::cout << "\n[1] Dry passthrough (MIX = 0)" << std::endl;
    {
        std::vector<float> input (44100);
        juce::Random random (7);

        for (auto& sample : input)
            sample = random.nextFloat() * 2.0f - 1.0f;

        Harness harness (0.0f, 4, false);
        const auto output = harness.run (input);

        float worstError = 0.0f;

        for (size_t i = 0; i < input.size(); ++i)
            worstError = juce::jmax (worstError, std::abs (output[i] - input[i]));

        check (worstError < 1.0e-4f, "dry signal is passed through unchanged (max error "
                                     + juce::String (worstError, 8) + ")");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[2] Output gain" << std::endl;
    {
        std::vector<float> input (22050, 0.5f);

        Harness harness (0.0f, 4, false, -6.0f);
        const auto output = harness.run (input);

        const float expected = 0.5f * juce::Decibels::decibelsToGain (-6.0f);
        check (std::abs (output[output.size() - 1] - expected) < 0.002f,
               "-6 dB is applied (got " + juce::String (output[output.size() - 1], 4)
               + ", expected " + juce::String (expected, 4) + ")");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[3] SIMPLE mode, straight note values" << std::endl;
    simpleReverseTest ("1/16",  0, 0.125);
    simpleReverseTest ("1/8",   2, 0.25);
    simpleReverseTest ("1/4",   4, 0.5);
    simpleReverseTest ("1/2",   6, 1.0);

    //--------------------------------------------------------------------------
    std::cout << "\n[4] SIMPLE mode, triplets" << std::endl;
    simpleReverseTest ("1/8T",  1, 1.0 / 6.0);
    simpleReverseTest ("1/4T",  3, 1.0 / 3.0);
    simpleReverseTest ("1/2T",  5, 2.0 / 3.0);

    //--------------------------------------------------------------------------
    std::cout << "\n[5] RHYTHMIC mode is locked to the host grid (120 BPM)" << std::endl;
    gridLockedTest ("1/8",  2, 0.5);
    gridLockedTest ("1/4",  4, 1.0);
    gridLockedTest ("1/8T", 1, 1.0 / 3.0);
    gridLockedTest ("1 Bar", 7, 4.0);

    //--------------------------------------------------------------------------
    std::cout << "\n[6] RHYTHMIC mode follows the tempo" << std::endl;
    {
        // 140 BPM, "1/4" => 0.4286 s per window
        const double bpm = 140.0;
        const int cell = (int) std::round (60.0 / bpm * testSampleRate);
        constexpr int offset = 300;

        std::vector<float> input ((size_t) (cell * 4 + 1024), 0.0f);
        input[(size_t) (cell * 2 + offset)] = 1.0f;

        Harness harness (100.0f, 4, true, 0.0f, bpm);
        FakePlayHead playHead (bpm, 0.0);
        const auto output = harness.run (input, &playHead);

        const size_t expected = (size_t) cell * 3 + (size_t) (cell - 1 - offset);
        const auto measured = peakIndex (output, (size_t) cell * 2, output.size());

        check (std::abs ((int) measured - (int) expected) <= 2,
               "140 BPM grid cell " + juce::String (cell) + " samples, impulse at "
               + juce::String ((int) expected) + " (got " + juce::String ((int) measured) + ")");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[7] Drag & drop export snapshot" << std::endl;
    {
        std::vector<float> input ((size_t) 44100, 0.0f);
        input[(size_t) 20000] = 0.9f;

        Harness harness (100.0f, 4, false);
        harness.run (input);

        juce::AudioBuffer<float> exported;
        const bool ok = harness.processor.copyReversedWindow (exported);

        check (ok, "copyReversedWindow returns a snapshot");

        // the snapshot must be time-reversed: the impulse sits near the start
        int peak = 0;
        float value = 0.0f;

        for (int i = 0; i < exported.getNumSamples(); ++i)
            if (std::abs (exported.getSample (0, i)) > value)
            {
                value = std::abs (exported.getSample (0, i));
                peak = i;
            }

        check (ok && std::abs (peak - 2049) <= 1,
               "exported audio is reversed (impulse at " + juce::String (peak) + ", expected 2049)");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[8] Automation stability" << std::endl;
    {
        Harness harness (100.0f, 4, false);

        std::vector<float> input (300000);
        juce::Random random (3);

        for (auto& sample : input)
            sample = random.nextFloat() * 2.0f - 1.0f;

        std::vector<float> output (input.size(), 0.0f);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        size_t position = 0;
        int blockIndex = 0;

        auto& state = harness.processor.getState();

        while (position < input.size())
        {
            const int numSamples = (int) juce::jmin ((size_t) blockSize, input.size() - position);

            if (blockIndex % 40 == 0)
            {
                state.getParameter ("mode")->setValueNotifyingHost ((blockIndex / 40) % 2 == 0 ? 1.0f : 0.0f);
                state.getParameter ("length")->setValueNotifyingHost ((float) (blockIndex / 40 % 9) / 8.0f);
                state.getParameter ("mix")->setValueNotifyingHost ((blockIndex / 40 % 5) / 4.0f);
                state.getParameter ("output")->setValueNotifyingHost ((float) (blockIndex / 40 % 3) / 2.0f);
            }

            block.clear();
            block.copyFrom (0, 0, input.data() + position, numSamples);
            block.copyFrom (1, 0, input.data() + position, numSamples);

            harness.processor.processBlock (block, midi);

            for (int n = 0; n < numSamples; ++n)
                output[position + (size_t) n] = block.getSample (0, n);

            position += (size_t) numSamples;
            ++blockIndex;
        }

        bool finite = true;

        for (auto sample : output)
            if (! std::isfinite (sample) || std::abs (sample) > 4.0f)
                finite = false;

        check (finite, "no NaN / Infinity / runaway level while automating");
    }

    //--------------------------------------------------------------------------
    std::cout << (failures == 0 ? "\nALL TESTS PASSED" : "\n" + juce::String (failures) + " TEST(S) FAILED")
              << std::endl;

    return failures == 0 ? 0 : 1;
}

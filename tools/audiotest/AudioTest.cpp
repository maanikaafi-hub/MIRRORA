/*
    Offline self-test for the MIRRORA engine.

    Runs the processor without a host and checks the things that were broken in
    the original prototype:
      * dry passthrough when MIX = 0
      * output gain is applied
      * SIMPLE mode plays the captured window backwards, exactly one window late
      * RHYTHMIC mode locks the window to the tempo
      * the wet signal is silent before the first window is filled
*/
#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
    int failures = 0;

    void check (bool condition, const juce::String& what)
    {
        std::cout << (condition ? "  PASS  " : "  FAIL  ") << what << std::endl;

        if (! condition)
            ++failures;
    }

    struct Harness
    {
        Harness (float mixPercent, int lengthIndex, bool rhythmic, float outputDb = 0.0f, double bpm = 120.0)
        {
            auto& state = processor.getState();

            state.getParameter ("mix")->setValueNotifyingHost (mixPercent / 100.0f);
            state.getParameter ("length")->setValueNotifyingHost ((float) lengthIndex / 3.0f);
            state.getParameter ("mode")->setValueNotifyingHost (rhythmic ? 1.0f : 0.0f);
            state.getParameter ("output")->setValueNotifyingHost ((outputDb + 24.0f) / 36.0f);
            state.getParameter ("bpm")->setValueNotifyingHost ((float) ((bpm - 40.0) / 200.0));
            state.getParameter ("sync")->setValueNotifyingHost (0.0f);   // no host => use the bpm parameter

            processor.setPlayConfigDetails (2, 2, 44100.0, 256);
            processor.prepareToPlay (44100.0, 256);
        }

        /** Feeds `input` through the processor in 256-sample blocks. */
        std::vector<float> run (const std::vector<float>& input)
        {
            std::vector<float> output (input.size(), 0.0f);
            juce::AudioBuffer<float> block (2, 256);
            juce::MidiBuffer midi;

            size_t position = 0;

            while (position < input.size())
            {
                const int numSamples = (int) juce::jmin ((size_t) 256, input.size() - position);

                block.clear();
                block.copyFrom (0, 0, input.data() + position, numSamples);
                block.copyFrom (1, 0, input.data() + position, numSamples);

                processor.processBlock (block, midi);

                for (int n = 0; n < numSamples; ++n)
                    output[position + (size_t) n] = block.getSample (0, n);

                position += (size_t) numSamples;
            }

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

        Harness harness (0.0f, 1, false);
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

        Harness harness (0.0f, 1, false, -6.0f);
        const auto output = harness.run (input);

        const float expected = 0.5f * juce::Decibels::decibelsToGain (-6.0f);
        check (std::abs (output[output.size() - 1] - expected) < 0.002f,
               "-6 dB is applied (got " + juce::String (output[output.size() - 1], 4)
               + ", expected " + juce::String (expected, 4) + ")");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[3] SIMPLE mode reverses the window" << std::endl;
    {
        // 0.5 s window at 44.1 kHz = 22050 samples
        constexpr int window = 22050;
        constexpr int impulseAt = 1000;

        std::vector<float> input ((size_t) (window * 2 + 512), 0.0f);
        input[(size_t) impulseAt] = 1.0f;

        Harness harness (100.0f, 1, false);
        const auto output = harness.run (input);

        check (maxAbs (output, 1, (size_t) window - 4) < 1.0e-6f,
               "wet output is silent before the first window is complete");

        // the impulse should come back reversed, i.e. at window + (window - 1 - impulseAt)
        const size_t expectedIndex = (size_t) window + (size_t) (window - 1 - impulseAt);

        size_t peakIndex = 0;
        float peak = 0.0f;

        for (size_t i = (size_t) window; i < output.size(); ++i)
        {
            if (std::abs (output[i]) > peak)
            {
                peak = std::abs (output[i]);
                peakIndex = i;
            }
        }

        check (std::abs (peak - 1.0f) < 0.01f,
               "reversed impulse has full amplitude (got " + juce::String (peak, 4) + ")");

        check (peakIndex == expectedIndex,
               "impulse lands at " + juce::String ((int) expectedIndex) + " (got "
               + juce::String ((int) peakIndex) + ")");

        check (maxAbs (output, expectedIndex + 200, output.size()) < 1.0e-6f,
               "nothing else leaks into the wet output");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[4] RHYTHMIC mode locks the window to the tempo" << std::endl;
    {
        // 120 BPM, length "1 Bar" (index 2) => 4 beats = 2 s = 88200 samples
        constexpr int window = 88200;
        constexpr int impulseAt = 500;

        std::vector<float> input ((size_t) (window + 4096), 0.0f);
        input[(size_t) impulseAt] = 1.0f;

        Harness harness (100.0f, 2, true, 0.0f, 120.0);
        const auto output = harness.run (input);

        const size_t expectedIndex = (size_t) window + (size_t) (window - 1 - impulseAt);

        check (maxAbs (output, 1, (size_t) window - 4) < 1.0e-6f,
               "still silent during the first 2 s window");

        size_t peakIndex = 0;
        float peak = 0.0f;

        for (size_t i = (size_t) window - 64; i < output.size(); ++i)
        {
            if (std::abs (output[i]) > peak)
            {
                peak = std::abs (output[i]);
                peakIndex = i;
            }
        }

        check (peakIndex == expectedIndex,
               "tempo-locked window is 2 s: impulse lands at " + juce::String ((int) expectedIndex)
               + " (got " + juce::String ((int) peakIndex) + ")");
    }

    //--------------------------------------------------------------------------
    std::cout << "\n[5] Mode switch and length change stay finite" << std::endl;
    {
        Harness harness (100.0f, 1, false);

        std::vector<float> input (300000);
        juce::Random random (3);

        for (auto& sample : input)
            sample = random.nextFloat() * 2.0f - 1.0f;

        std::vector<float> output (input.size(), 0.0f);
        juce::AudioBuffer<float> block (2, 256);
        juce::MidiBuffer midi;

        size_t position = 0;
        int blockIndex = 0;

        auto& state = harness.processor.getState();

        while (position < input.size())
        {
            const int numSamples = (int) juce::jmin ((size_t) 256, input.size() - position);

            // flip parameters while audio is running
            if (blockIndex % 40 == 0)
            {
                state.getParameter ("mode")->setValueNotifyingHost ((blockIndex / 40) % 2 == 0 ? 1.0f : 0.0f);
                state.getParameter ("length")->setValueNotifyingHost ((float) (blockIndex / 40 % 4) / 3.0f);
                state.getParameter ("mix")->setValueNotifyingHost ((blockIndex / 40 % 5) / 4.0f);
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

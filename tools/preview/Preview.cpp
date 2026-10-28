/*
    Off-screen renderer for the MIRRORA editor.

    Builds on Linux/macOS/Windows (see MIRRORA_BUILD_PREVIEW in CMakeLists.txt)
    and writes a PNG of the editor without opening a window. Used by the
    "GUI preview" GitHub workflow to check the design.  (render trigger)
*/
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    MIRRORAProcessor processor;
    processor.setPlayConfigDetails (2, 2, 44100.0, 512);
    processor.prepareToPlay (44100.0, 512);

    // Feed a musical-looking signal so the scope has something to display.
    juce::AudioBuffer<float> block (2, 512);
    juce::MidiBuffer midi;

    for (int b = 0; b < 240; ++b)
    {
        for (int n = 0; n < 512; ++n)
        {
            const double t = (double) (b * 512 + n) / 44100.0;

            // a musical-looking envelope: slow swell + accents + noise floor
            const double swell = 0.42 + 0.34 * std::sin (juce::MathConstants<double>::twoPi * t * 0.55);
            const double accent = 0.55 * std::pow (std::abs (std::sin (juce::MathConstants<double>::twoPi * t * 1.7)), 6.0);
            const double noise = 0.10 * std::sin (t * 5237.0) * std::sin (t * 811.0);

            const double tone = 0.55 * std::sin (juce::MathConstants<double>::twoPi * t * 220.0)
                              + 0.25 * std::sin (juce::MathConstants<double>::twoPi * t * 333.0)
                              + 0.18 * std::sin (juce::MathConstants<double>::twoPi * t * 97.0);

            const float sample = (float) (juce::jlimit (0.0, 1.0, swell + accent + noise) * 0.62 * tone);
            block.setSample (0, n, sample);
            block.setSample (1, n, sample * 0.93f);
        }

        processor.processBlock (block, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (960, 570);
    editor->setVisible (true);

    // Let the scope timer run a few frames.
    juce::MessageManager::getInstance()->runDispatchLoopUntil (700);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.5f);

    const juce::String fileName = argc > 1 ? juce::String (argv[1]) : juce::String ("mirrora-gui.png");
    const auto outputFile = juce::File::getCurrentWorkingDirectory().getChildFile (fileName);

    outputFile.deleteFile();

    if (auto stream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
        stream->flush();
    }

    std::cout << "wrote " << outputFile.getFullPathName() << " ("
              << image.getWidth() << "x" << image.getHeight() << ")" << std::endl;

    return 0;
}

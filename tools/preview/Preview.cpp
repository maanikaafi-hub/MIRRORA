/*
    Off-screen renderer for the MIRRORA editor.

    Builds on Linux/macOS/Windows (see MIRRORA_BUILD_PREVIEW in CMakeLists.txt)
    and writes a PNG of the editor without opening a window. Used by the
    "GUI preview" GitHub workflow to check the design.
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
            const double envelope = 0.30 + 0.55 * std::abs (std::sin (t * 2.1))
                                          + 0.15 * std::abs (std::sin (t * 7.3));
            const float sample = (float) (envelope * 0.55
                                          * std::sin (juce::MathConstants<double>::twoPi * t * 180.0));
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

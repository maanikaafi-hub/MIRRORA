#include "WaveformDisplay.h"

#include <cmath>

WaveformDisplay::WaveformDisplay (MIRRORAProcessor& processorToUse)
    : processor (processorToUse)
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    startTimerHz (30);
}

void WaveformDisplay::timerCallback()
{
    processor.readScope (incoming.data());

    bool needsRepaint = false;

    for (size_t i = 0; i < displayed.size(); ++i)
    {
        const float target = incoming[i];
        const float coefficient = target > displayed[i] ? 0.55f : 0.13f;
        const float next = displayed[i] + (target - displayed[i]) * coefficient;

        if (std::abs (next - displayed[i]) > 0.0015f)
            needsRepaint = true;

        displayed[i] = next;
    }

    if (hintFrames > 0)
    {
        --hintFrames;
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void WaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragStart = e.getPosition();
    dragging = false;
}

void WaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 8)
        return;

    dragging = true;
    beginExport();
}

void WaveformDisplay::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

void WaveformDisplay::mouseEnter (const juce::MouseEvent&)
{
    hovering = true;
    repaint();
}

void WaveformDisplay::mouseExit (const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}

juce::File WaveformDisplay::createExportFile()
{
    juce::AudioBuffer<float> reversed;

    if (! processor.copyReversedWindow (reversed))
        return {};

    const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("MIRRORA");

    if (! folder.createDirectory())
        return {};

    // keep only the most recent exports around
    for (const auto& entry : juce::RangedDirectoryIterator (folder, false, "*.wav"))
        if (entry.getFile().getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours (2))
            entry.getFile().deleteFile();

    const auto file = folder.getNonexistentChildFile (processor.describeExportName(),
                                                      ".wav", false);

    if (auto stream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream()))
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            format.createWriterFor (stream.get(), processor.getSampleRate(),
                                    (unsigned int) reversed.getNumChannels(), 24, {}, 0));

        if (writer != nullptr)
        {
            stream.release();   // the writer owns the stream now
            writer->writeFromAudioSampleBuffer (reversed, 0, reversed.getNumSamples());
            writer.reset();
            return file;
        }
    }

    file.deleteFile();
    return {};
}

void WaveformDisplay::beginExport()
{
    const auto file = createExportFile();

    if (! file.existsAsFile())
    {
        hintFrames = 90;
        hintText = "PLAY SOME AUDIO FIRST";
        repaint();
        return;
    }

    hintFrames = 90;
    hintText = "EXPORTED: " + file.getFileName();
    repaint();

    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() },
                                                                false, this, nullptr);
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    using namespace mirrora;

    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const float radius = 10.0f;

    juce::Path panel;
    panel.addRoundedRectangle (bounds, radius);

    juce::ColourGradient background (juce::Colour (0xff0e0d15), bounds.getCentreX(), bounds.getY(),
                                     juce::Colour (0xff060509), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (background);
    g.fillPath (panel);

    {
        juce::Graphics::ScopedSaveState clipScope (g);
        g.reduceClipRegion (panel);

        //---- grid ------------------------------------------------------------
        g.setColour (juce::Colours::white.withAlpha (0.045f));

        for (int i = 1; i < 8; ++i)
        {
            const float x = bounds.getX() + bounds.getWidth() * (float) i / 8.0f;
            g.fillRect (x, bounds.getY() + 9.0f, 1.0f, bounds.getHeight() - 18.0f);
        }

        for (int i = 1; i < 4; ++i)
        {
            const float y = bounds.getY() + bounds.getHeight() * (float) i / 4.0f;
            g.fillRect (bounds.getX() + 9.0f, y, bounds.getWidth() - 18.0f, 1.0f);
        }

        //---- waveform --------------------------------------------------------
        const float centreY    = bounds.getCentreY();
        const float halfHeight = bounds.getHeight() * 0.40f;
        const float left       = bounds.getX() + 12.0f;
        const float right      = bounds.getRight() - 12.0f;
        const float width      = juce::jmax (1.0f, right - left);

        const int numBins = (int) displayed.size();
        const float step = width / (float) numBins;
        const float barWidth = juce::jmax (1.0f, step * 0.42f);

        juce::Path bars;

        for (int i = 0; i < numBins; ++i)
        {
            const float value = juce::jlimit (0.0f, 1.0f, displayed[(size_t) i]);
            const float shaped = std::pow (value, 0.80f);
            const float amplitude = juce::jmax (0.8f, shaped * halfHeight);
            const float x = left + step * (float) i;

            bars.addRectangle (x, centreY - amplitude, barWidth, amplitude * 2.0f);
        }

        g.setColour (colours::purple.withAlpha (0.20f));
        g.fillPath (bars);

        juce::ColourGradient barGradient (colours::purpleDeep.withAlpha (0.45f),
                                          bounds.getCentreX(), bounds.getY() + 10.0f,
                                          colours::purpleDeep.withAlpha (0.45f),
                                          bounds.getCentreX(), bounds.getBottom() - 10.0f, false);
        barGradient.addColour (0.35, colours::purple.withAlpha (0.85f));
        barGradient.addColour (0.5, juce::Colour (0xfff0e4ff).withAlpha (0.98f));
        barGradient.addColour (0.65, colours::purple.withAlpha (0.85f));
        g.setGradientFill (barGradient);
        g.fillPath (bars);

        //---- centre playhead -------------------------------------------------
        juce::Path playhead;
        playhead.addRoundedRectangle (bounds.getCentreX() - 0.9f, bounds.getY() + 14.0f,
                                      1.8f, bounds.getHeight() - 28.0f, 0.9f);

        juce::ColourGradient headGradient (juce::Colours::transparentWhite,
                                          bounds.getCentreX(), bounds.getY() + 14.0f,
                                          juce::Colours::transparentWhite,
                                          bounds.getCentreX(), bounds.getBottom() - 14.0f, false);
        headGradient.addColour (0.5, juce::Colours::white.withAlpha (0.85f));
        g.setGradientFill (headGradient);
        g.fillPath (playhead);

        g.setColour (colours::purple.withAlpha (0.18f));
        g.fillRect (bounds.getCentreX() - 3.0f, bounds.getY() + 22.0f, 6.0f, bounds.getHeight() - 44.0f);

        //---- drag hint -------------------------------------------------------
        if (hovering || hintFrames > 0)
        {
            const juce::String label = hintFrames > 0 ? hintText : "DRAG  →  EXPORT REVERSED AUDIO";

            const auto pill = juce::Rectangle<float> (bounds.getCentreX() - 120.0f,
                                                      bounds.getBottom() - 30.0f, 240.0f, 20.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (pill, 10.0f);
            g.setColour (colours::purple.withAlpha (0.35f));
            g.drawRoundedRectangle (pill, 10.0f, 1.0f);
            mirrora::drawText (g, label, pill, mirrora::uiFont (8.5f, 0.22f),
                               colours::purpleSoft.withAlpha (0.95f));
        }

        //---- end markers -----------------------------------------------------
        const float capX[2] { bounds.getX() + 7.0f, bounds.getRight() - 9.5f };

        for (float x : capX)
        {
            juce::Path cap;
            cap.addRoundedRectangle (x, centreY - bounds.getHeight() * 0.15f,
                                     2.5f, bounds.getHeight() * 0.30f, 1.25f);
            g.setColour (colours::purple.withAlpha (0.30f));
            g.fillPath (cap);
            g.setColour (colours::purpleSoft.withAlpha (0.80f));
            g.fillPath (cap);
        }
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (panel, juce::PathStrokeType (1.2f));
}

#include "PluginEditor.h"

#include <cmath>

namespace
{
    juce::RangedAudioParameter& paramRef (MIRRORAProcessor& processor, const juce::String& id)
    {
        auto* parameter = processor.getState().getParameter (id);
        jassert (parameter != nullptr);
        return *parameter;
    }
}

//==============================================================================
void PanelBackground::paint (juce::Graphics& g)
{
    using namespace mirrora;

    const auto outer = getLocalBounds().toFloat().reduced (1.0f);
    const float outerRadius = 16.0f;

    //---- raised bezel -------------------------------------------------------
    juce::Path bezel;
    bezel.addRoundedRectangle (outer, outerRadius);

    juce::ColourGradient bezelFill (juce::Colour (0xff3a3742), outer.getCentreX(), outer.getY(),
                                    juce::Colour (0xff15141a), outer.getCentreX(), outer.getBottom(), false);
    bezelFill.addColour (0.12, juce::Colour (0xff2e2b36));
    g.setGradientFill (bezelFill);
    g.fillPath (bezel);

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.strokePath (bezel, juce::PathStrokeType (2.0f));

    //---- body ---------------------------------------------------------------
    const auto bounds = outer.reduced (4.0f);
    const float radius = 13.0f;

    juce::Path panel;
    panel.addRoundedRectangle (bounds, radius);

    juce::ColourGradient body (colours::panelTop, bounds.getCentreX(), bounds.getY(),
                               colours::panelBottom, bounds.getCentreX(), bounds.getBottom(), false);
    body.addColour (0.28, juce::Colour (0xff1d1c23));
    body.addColour (0.60, juce::Colour (0xff17161c));
    g.setGradientFill (body);
    g.fillPath (panel);

    {
        juce::Graphics::ScopedSaveState scope (g);
        g.reduceClipRegion (panel);

        // brushed horizontal hairlines
        for (float y = bounds.getY(); y < bounds.getBottom(); y += 3.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (std::fmod (y, 6.0f) < 1.5f ? 0.016f : 0.007f));
            g.fillRect (bounds.getX(), y, bounds.getWidth(), 1.0f);
        }

        // top sheen
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.06f),
                                    bounds.getCentreX(), bounds.getY(),
                                    juce::Colours::transparentBlack,
                                    bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.45f, false);
        g.setGradientFill (sheen);
        g.fillRect (bounds);

        // very soft purple bloom behind the knob row
        juce::ColourGradient glow (colours::purple.withAlpha (0.10f),
                                   bounds.getCentreX(), bounds.getY() + 130.0f,
                                   juce::Colours::transparentBlack,
                                   bounds.getCentreX(), bounds.getY() + 330.0f, true);
        g.setGradientFill (glow);
        g.fillRect (bounds);
    }

    //---- header -------------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (bounds.getX() + 10.0f, 51.0f, bounds.getWidth() - 20.0f, 1.0f);

    // zoom cluster backdrop
    {
        const juce::Rectangle<float> cluster (620.0f, 11.0f, 128.0f, 28.0f);
        g.setColour (juce::Colours::white.withAlpha (0.030f));
        g.fillRoundedRectangle (cluster, 14.0f);
        g.setColour (juce::Colours::white.withAlpha (0.075f));
        g.drawRoundedRectangle (cluster.reduced (0.5f), 14.0f, 1.0f);
    }

    // separator before the store label
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRect (822.0f, 10.0f, 1.0f, 30.0f);

    //---- frame --------------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (panel, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (1.8f), radius - 1.2f, 1.0f);

    //---- logo ---------------------------------------------------------------
    const auto logoColour = colours::purpleSoft;

    g.setColour (logoColour);
    g.fillRoundedRectangle (32.0f, 14.0f, 3.6f, 21.0f, 1.8f);
    g.fillEllipse (41.6f, 23.0f, 4.4f, 4.4f);
    g.fillRoundedRectangle (49.0f, 18.5f, 3.6f, 13.0f, 1.8f);

    //---- title --------------------------------------------------------------
    drawText (g, "MIRRORA", { 62.0f, 8.0f, 260.0f, 32.0f },
              uiFont (19.0f, 0.30f), colours::textBright, juce::Justification::centredLeft);

    //---- zoom readout -------------------------------------------------------
    drawText (g, juce::String (zoomPercent) + " %", { 654.0f, 12.0f, 60.0f, 26.0f },
              uiFont (11.5f, 0.06f), colours::textDim);

    //---- plugin store -------------------------------------------------------
    mirrora::IconButton::drawGlyph (g, mirrora::IconType::bars,
                                    { 840.0f, 15.0f, 16.0f, 18.0f }, colours::textDim);
    drawText (g, "Plugin Store", { 862.0f, 12.0f, 92.0f, 26.0f },
              uiFont (11.5f, 0.02f), colours::textDim, juce::Justification::centredLeft);

    //---- captions -----------------------------------------------------------
    drawText (g, "REVERSE MODE", { 34.0f, 210.0f, 276.0f, 20.0f },
              uiFont (9.0f, 0.30f), colours::textFaint);

    //---- wordmark -----------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRect (352.0f, 504.0f, 82.0f, 1.0f);
    g.fillRect (526.0f, 504.0f, 82.0f, 1.0f);

    drawText (g, "MIRRORA", { 432.0f, 489.0f, 96.0f, 30.0f },
              uiFont (11.0f, 0.42f), colours::textDim);

    //---- waveform chip ------------------------------------------------------
    const juce::Rectangle<float> chip (34.0f, 528.0f, 122.0f, 24.0f);
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (chip, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawRoundedRectangle (chip, 12.0f, 1.0f);
    g.setColour (colours::purple.withAlpha (0.85f));
    g.fillEllipse (chip.getX() + 10.0f, chip.getCentreY() - 2.5f, 5.0f, 5.0f);
    drawText (g, "WAVEFORM", chip.withTrimmedLeft (24.0f),
              uiFont (8.5f, 0.30f), colours::textFaint, juce::Justification::centredLeft);
}

//==============================================================================
AboutOverlay::AboutOverlay()
{
    setVisible (false);
    setAlwaysOnTop (true);
}

void AboutOverlay::paint (juce::Graphics& g)
{
    using namespace mirrora;

    g.setColour (juce::Colours::black.withAlpha (0.60f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);

    const auto card = getLocalBounds().toFloat().withSizeKeepingCentre (430.0f, 240.0f);

    juce::Path cardPath;
    cardPath.addRoundedRectangle (card, 16.0f);

    juce::ColourGradient fill (juce::Colour (0xff241d33), card.getCentreX(), card.getY(),
                               juce::Colour (0xff121016), card.getCentreX(), card.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (cardPath);

    g.setColour (colours::purple.withAlpha (0.55f));
    g.strokePath (cardPath, juce::PathStrokeType (1.3f));

    auto text = card.reduced (26.0f, 20.0f);

    drawText (g, "MIRRORA", text.removeFromTop (34.0f),
              uiFont (22.0f, 0.34f), colours::textBright, juce::Justification::centredLeft);

    drawText (g, juce::String ("v") + JucePlugin_VersionString + "   ·   real-time reverse engine",
              text.removeFromTop (20.0f), uiFont (10.5f, 0.08f), colours::textDim,
              juce::Justification::centredLeft);

    text.removeFromTop (14.0f);

    drawText (g, "Drag a knob to change a value, double-click to reset.",
              text.removeFromTop (18.0f), uiFont (10.5f, 0.0f), colours::textDim,
              juce::Justification::centredLeft);
    drawText (g, "SIMPLE follows a fixed window, RHYTHMIC locks to the host tempo.",
              text.removeFromTop (18.0f), uiFont (10.5f, 0.0f), colours::textDim,
              juce::Justification::centredLeft);
    drawText (g, "The waveform shows the signal being captured for reversal.",
              text.removeFromTop (18.0f), uiFont (10.5f, 0.0f), colours::textDim,
              juce::Justification::centredLeft);

    drawText (g, "by MaaNi KaaFi", text.removeFromBottom (24.0f),
              uiFont (10.0f, 0.24f), colours::purpleSoft, juce::Justification::centred);
}

void AboutOverlay::mouseDown (const juce::MouseEvent&)
{
    setVisible (false);
}

//==============================================================================
MIRRORAEditor::MIRRORAEditor (MIRRORAProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      lengthStepper (paramRef (p, "length"), "LENGTH", mirrora::IconType::loop, 1.0),
      outputStepper (paramRef (p, "output"), "OUTPUT", mirrora::IconType::bars, 0.5),
      waveform (p)
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (mirrora::layout::designWidth * 0.70f),
                     juce::roundToInt (mirrora::layout::designHeight * 0.70f),
                     juce::roundToInt (mirrora::layout::designWidth * 1.75f),
                     juce::roundToInt (mirrora::layout::designHeight * 1.75f));

    addAndMakeVisible (content);
    content.addAndMakeVisible (background);
    content.addAndMakeVisible (simpleButton);
    content.addAndMakeVisible (rhythmicButton);
    content.addAndMakeVisible (mixKnob);
    content.addAndMakeVisible (lengthKnob);
    content.addAndMakeVisible (outputKnob);
    content.addAndMakeVisible (syncButton);
    content.addAndMakeVisible (waveform);
    content.addAndMakeVisible (zoomOutButton);
    content.addAndMakeVisible (zoomInButton);
    content.addAndMakeVisible (infoButton);
    content.addAndMakeVisible (lengthStepper);
    content.addAndMakeVisible (outputStepper);
    content.addAndMakeVisible (about);
    about.setVisible (false);

    //---- knobs --------------------------------------------------------------
    mixKnob.setCaption ("MIX");
    mixKnob.setLarge (true);
    mixKnob.setTooltip ("Mix — dry/wet balance");

    lengthKnob.setCaption ("LENGTH");
    lengthKnob.setTooltip ("Length — size of the reversed window");

    outputKnob.setCaption ("OUTPUT");
    outputKnob.setTooltip ("Output level");

    mixAttachment    = std::make_unique<juce::SliderParameterAttachment> (paramRef (processor, "mix"),    mixKnob,    nullptr);
    lengthAttachment = std::make_unique<juce::SliderParameterAttachment> (paramRef (processor, "length"), lengthKnob, nullptr);
    outputAttachment = std::make_unique<juce::SliderParameterAttachment> (paramRef (processor, "output"), outputKnob, nullptr);

    //---- mode ---------------------------------------------------------------
    simpleButton.setTooltip ("Simple reverse mode");
    rhythmicButton.setTooltip ("Rhythmic reverse mode (tempo synced)");

    modeAttachment = std::make_unique<juce::ParameterAttachment> (
        paramRef (processor, "mode"),
        [this] (float value)
        {
            const bool isSimple = value < 0.5f;
            simpleButton.setToggleState (isSimple,   juce::dontSendNotification);
            rhythmicButton.setToggleState (! isSimple, juce::dontSendNotification);
        });
    modeAttachment->sendInitialUpdate();

    simpleButton.onClick   = [this] { modeAttachment->setValueAsCompleteGesture (0.0f); };
    rhythmicButton.onClick = [this] { modeAttachment->setValueAsCompleteGesture (1.0f); };

    //---- tempo sync ---------------------------------------------------------
    syncButton.setTooltip ("Follow the host tempo");
    syncAttachment = std::make_unique<juce::ParameterAttachment> (
        paramRef (processor, "sync"),
        [this] (float value) { syncButton.setToggleState (value > 0.5f, juce::dontSendNotification); });
    syncAttachment->sendInitialUpdate();

    syncButton.onClick = [this]
    {
        syncAttachment->setValueAsCompleteGesture (syncButton.getToggleState() ? 1.0f : 0.0f);
    };

    //---- header buttons -----------------------------------------------------
    zoomOutButton.setTooltip ("Zoom out");
    zoomInButton.setTooltip ("Zoom in");
    infoButton.setTooltip ("About MIRRORA");

    zoomOutButton.onClick = [this] { setZoom (currentScale() - 0.1f); };
    zoomInButton.onClick  = [this] { setZoom (currentScale() + 0.1f); };

    infoButton.onClick = [this]
    {
        about.setVisible (! about.isVisible());

        if (about.isVisible())
            about.toFront (false);
    };

    layoutContent();
    setSize (juce::roundToInt (mirrora::layout::designWidth),
             juce::roundToInt (mirrora::layout::designHeight));
}

MIRRORAEditor::~MIRRORAEditor() = default;

//==============================================================================
void MIRRORAEditor::layoutContent()
{
    content.setBounds (0, 0,
                       juce::roundToInt (mirrora::layout::designWidth),
                       juce::roundToInt (mirrora::layout::designHeight));

    background.setBounds (content.getLocalBounds());
    about.setBounds (content.getLocalBounds());

    simpleButton  .setBounds (34,  124, 130,  80);
    rhythmicButton.setBounds (180, 124, 130,  80);

    mixKnob   .setBounds (392,  56, 172, 214);
    lengthKnob.setBounds (623,  96,  96, 130);
    outputKnob.setBounds (848,  96,  96, 130);

    syncButton.setBounds (735, 128, 110, 38);

    waveform.setBounds (34, 276, 892, 162);

    zoomOutButton.setBounds (624, 12, 26, 26);
    zoomInButton .setBounds (718, 12, 26, 26);
    infoButton   .setBounds (760, 12, 26, 26);

    lengthStepper.setBounds ( 68, 462, 200, 60);
    outputStepper.setBounds (733, 462, 200, 60);
}

void MIRRORAEditor::paint (juce::Graphics& g)
{
    using namespace mirrora;

    g.fillAll (colours::windowBg);

    const auto area = getLocalBounds().toFloat();

    juce::ColourGradient bloom (colours::purple.withAlpha (0.11f),
                                area.getCentreX(), area.getY() + area.getHeight() * 0.05f,
                                juce::Colours::transparentBlack,
                                area.getCentreX(), area.getBottom(), true);
    g.setGradientFill (bloom);
    g.fillRect (area);
}

void MIRRORAEditor::resized()
{
    using namespace mirrora;

    const float scale = juce::jmin ((float) getWidth()  / layout::designWidth,
                                    (float) getHeight() / layout::designHeight);

    content.setTransform (juce::AffineTransform::scale (scale)
                              .translated ((getWidth()  - layout::designWidth  * scale) * 0.5f,
                                           (getHeight() - layout::designHeight * scale) * 0.5f));

    const int percent = juce::roundToInt (scale * 100.0f);

    if (percent != background.zoomPercent)
    {
        background.zoomPercent = percent;
        background.repaint();
    }
}

void MIRRORAEditor::setZoom (float newScale)
{
    newScale = juce::jlimit (0.70f, 1.75f, newScale);

    setSize (juce::roundToInt (mirrora::layout::designWidth  * newScale),
             juce::roundToInt (mirrora::layout::designHeight * newScale));
}

float MIRRORAEditor::currentScale() const
{
    return (float) getWidth() / mirrora::layout::designWidth;
}

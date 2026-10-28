/*
    MIRRORA — custom GUI widgets (metal knobs, mode buttons, pills, steppers).
*/
#pragma once

#include <JuceHeader.h>
#include "MirroraTheme.h"

namespace mirrora
{

//==============================================================================
/** Large or small brushed-metal rotary knob with caption + value readout.

    The component is a juce::Slider; the caption is drawn underneath the dial and
    the value text comes from the parameter's own text formatting, so it always
    shows exactly what the host shows.
*/
class Knob final : public juce::Slider
{
public:
    Knob();
    ~Knob() override = default;

    void setCaption (juce::String newCaption)  { caption = std::move (newCaption); repaint(); }
    void setLarge (bool shouldBeLarge)         { large = shouldBeLarge; repaint(); }
    void setAccent (juce::Colour c)            { accent = c; repaint(); }

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void paintDial (juce::Graphics&, juce::Rectangle<float> area);

    juce::String caption { "MIX" };
    juce::Colour accent  { colours::purple };
    bool large { false };
    bool hovering { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};

//==============================================================================
/** The two big mode buttons (SIMPLE / RHYTHMIC). */
class ModeButton final : public juce::Button
{
public:
    enum class Glyph { reverse, rhythmic };

    ModeButton (const juce::String& buttonText, Glyph g);
    ~ModeButton() override = default;

    void paintButton (juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

private:
    Glyph glyph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeButton)
};

//==============================================================================
/** Rounded "BPM SYNC" style pill toggle with an icon. */
class PillToggle final : public juce::Button
{
public:
    explicit PillToggle (const juce::String& buttonText);
    ~PillToggle() override = default;

    void paintButton (juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

private:
    juce::String label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PillToggle)
};

//==============================================================================
enum class IconType { plus, minus, gear, chevronLeft, chevronRight,
                      note, reverse, loop, bars };

/** Small flat icon button used in the header and the bottom steppers. */
class IconButton final : public juce::Button
{
public:
    explicit IconButton (IconType typeToUse);
    ~IconButton() override = default;

    void setIcon (IconType t) { type = t; repaint(); }
    void setIconColour (juce::Colour c) { iconColour = c; repaint(); }
    void setDrawsCircle (bool shouldDrawCircle) { drawCircle = shouldDrawCircle; repaint(); }

    void paintButton (juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

    static void drawGlyph (juce::Graphics&, IconType, juce::Rectangle<float> area, juce::Colour);

private:
    IconType type;
    juce::Colour iconColour { colours::textDim };
    bool drawCircle { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};

//==============================================================================
/** Bottom-row parameter stepper: circular icon, caption, value and < > arrows. */
class StepperRow final : public juce::Component
{
public:
    StepperRow (juce::RangedAudioParameter& parameter,
                juce::String captionText,
                IconType iconType,
                double stepInValueUnits);

    ~StepperRow() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void nudge (int direction);

    juce::RangedAudioParameter& param;
    juce::String caption;
    IconType icon;
    double step;
    juce::String valueText;

    IconButton previous { IconType::chevronLeft };
    IconButton next     { IconType::chevronRight };

    std::unique_ptr<juce::ParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepperRow)
};

} // namespace mirrora

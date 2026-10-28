#include "MirroraControls.h"

#include <cmath>

namespace mirrora
{

//==============================================================================
// Knob
//==============================================================================
Knob::Knob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f,
                         true);
    setMouseDragSensitivity (260);
    setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (true);
    setVelocityBasedMode (false);
}

void Knob::mouseEnter (const juce::MouseEvent&)
{
    hovering = true;
    repaint();
}

void Knob::mouseExit (const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}

void Knob::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    const float labelBand = large ? 40.0f : 34.0f;
    auto dialArea = area.removeFromTop (juce::jmax (12.0f, area.getHeight() - labelBand));

    paintDial (g, dialArea);

    auto captionArea = area.removeFromTop (area.getHeight() * 0.5f);

    drawText (g, caption, captionArea,
              uiFont (large ? 11.0f : 9.0f, 0.24f), colours::textDim);

    drawText (g, getTextFromValue (getValue()), area,
              uiFont (large ? 14.0f : 11.5f, 0.05f), colours::textBright);
}

void Knob::paintDial (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float inset      = large ? 8.0f : 4.0f;
    const float diameter   = juce::jmax (10.0f, juce::jmin (area.getWidth(), area.getHeight()) - inset * 2.0f);
    const auto  centre     = area.getCentre();
    const float ringRadius = diameter * 0.5f;
    const float ringWidth  = large ? 3.6f : 2.6f;
    const float bodyRadius = ringRadius - (large ? 9.0f : 6.5f);

    const auto rotary = getRotaryParameters();
    const float startAngle = rotary.startAngleRadians;
    const float endAngle   = rotary.endAngleRadians;
    const float proportion = juce::jlimit (0.0f, 1.0f, (float) valueToProportionOfLength (getValue()));
    const float angle      = startAngle + proportion * (endAngle - startAngle);

    // soft halo on hover
    if (hovering || isMouseButtonDown())
    {
        g.setColour (accent.withAlpha (0.05f));
        g.fillEllipse (centre.x - ringRadius - 9.0f, centre.y - ringRadius - 9.0f,
                       (ringRadius + 9.0f) * 2.0f, (ringRadius + 9.0f) * 2.0f);
    }

    // track
    const auto trackArc = makeArc (centre, ringRadius, startAngle, endAngle);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (trackArc, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // value arc
    if (proportion > 0.002f)
    {
        const auto valueArc = makeArc (centre, ringRadius, startAngle,
                                       juce::jmax (startAngle + 0.02f, angle));
        drawGlowPath (g, valueArc, accent, ringWidth, ringWidth * 4.5f, 0.30f);
    }

    // body shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (centre.x - bodyRadius - 2.5f, centre.y - bodyRadius - 2.0f,
                   (bodyRadius + 2.5f) * 2.0f, (bodyRadius + 2.5f) * 2.0f);

    juce::Path bodyPath;
    bodyPath.addEllipse (centre.x - bodyRadius, centre.y - bodyRadius,
                         bodyRadius * 2.0f, bodyRadius * 2.0f);

    {
        juce::Graphics::ScopedSaveState clipScope (g);
        g.reduceClipRegion (bodyPath);

        // spherical base shading
        juce::ColourGradient base (juce::Colour (0xff75757f),
                                   centre.x - bodyRadius * 0.28f, centre.y - bodyRadius * 0.42f,
                                   juce::Colour (0xff111016),
                                   centre.x + bodyRadius * 0.80f, centre.y + bodyRadius * 0.80f,
                                   true);
        base.addColour (0.30, juce::Colour (0xff5c5b66));
        base.addColour (0.62, juce::Colour (0xff2a2931));
        g.setGradientFill (base);
        g.fillRect (area);

        juce::Graphics::ScopedSaveState rotateScope (g);
        g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));

        // rotating brushed sheen
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.22f),
                                    centre.x - bodyRadius, centre.y,
                                    juce::Colours::transparentBlack,
                                    centre.x + bodyRadius, centre.y, false);
        sheen.addColour (0.34, juce::Colours::white.withAlpha (0.05f));
        sheen.addColour (0.62, juce::Colours::black.withAlpha (0.30f));
        sheen.addColour (1.0, juce::Colours::transparentBlack);
        g.setGradientFill (sheen);
        g.fillRect (area);

        // radial "spokes" that turn with the knob
        const int spokes = large ? 120 : 64;

        for (int i = 0; i < spokes; ++i)
        {
            const float spokeAngle = (float) i * juce::MathConstants<float>::twoPi / (float) spokes;
            const float alpha = 0.010f + 0.020f * (0.5f + 0.5f * std::sin (spokeAngle * 7.0f + 1.3f));

            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.drawLine (juce::Line<float> (centre, centre.getPointOnCircumference (bodyRadius, spokeAngle)),
                        0.8f);
        }
    }

    // vignette
    {
        juce::Graphics::ScopedSaveState clipScope (g);
        g.reduceClipRegion (bodyPath);

        juce::ColourGradient vignette (juce::Colours::transparentBlack, centre.x, centre.y,
                                       juce::Colours::black.withAlpha (0.62f),
                                       centre.x, centre.y - bodyRadius, true);
        g.setGradientFill (vignette);
        g.fillRect (area);
    }

    // rim light
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (centre.x - bodyRadius + 0.5f, centre.y - bodyRadius + 0.5f,
                   (bodyRadius - 0.5f) * 2.0f, (bodyRadius - 0.5f) * 2.0f, 1.1f);

    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.strokePath (makeArc (centre, bodyRadius - 1.4f,
                           juce::MathConstants<float>::pi * 1.15f,
                           juce::MathConstants<float>::pi * 1.80f),
                  juce::PathStrokeType (1.5f));

    // centre cap
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (centre.x - bodyRadius * 0.13f, centre.y - bodyRadius * 0.13f,
                   bodyRadius * 0.26f, bodyRadius * 0.26f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawEllipse (centre.x - bodyRadius * 0.13f, centre.y - bodyRadius * 0.13f,
                   bodyRadius * 0.26f, bodyRadius * 0.26f, 1.0f);

    // needle
    juce::Path needle;
    needle.startNewSubPath (centre.getPointOnCircumference (bodyRadius * 0.62f, angle));
    needle.lineTo (centre.getPointOnCircumference (bodyRadius * 0.95f, angle));

    g.setColour (accent.withAlpha (0.30f));
    g.strokePath (needle, juce::PathStrokeType (large ? 7.0f : 5.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (needle, juce::PathStrokeType (large ? 3.0f : 2.3f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (needle, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

//==============================================================================
// ModeButton
//==============================================================================
ModeButton::ModeButton (const juce::String& buttonText, Glyph g)
    : juce::Button (buttonText), glyph (g)
{
    setWantsKeyboardFocus (false);
}

void ModeButton::paintButton (juce::Graphics& g, bool shouldDrawHighlighted, bool /*shouldDrawDown*/)
{
    auto area = getLocalBounds().toFloat().reduced (1.0f, 1.0f);
    const bool on = getToggleState();

    const auto topColour    = on ? juce::Colour (0xff2a1f47) : juce::Colour (0xff1b1a21);
    const auto bottomColour = on ? juce::Colour (0xff15101f) : juce::Colour (0xff131218);

    juce::ColourGradient fill (topColour, area.getCentreX(), area.getY(),
                               bottomColour, area.getCentreX(), area.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (area, 15.0f);

    // inner top highlight
    g.setColour (juce::Colours::white.withAlpha (on ? 0.08f : 0.045f));
    g.drawRoundedRectangle (area.reduced (1.5f).withHeight (area.getHeight() * 0.55f), 14.0f, 1.0f);

    if (on)
    {
        g.setColour (colours::purple.withAlpha (0.10f));
        g.drawRoundedRectangle (area.expanded (4.0f), 18.0f, 4.0f);
        g.setColour (colours::purple.withAlpha (0.16f));
        g.drawRoundedRectangle (area.expanded (2.0f), 16.5f, 2.0f);
        g.setColour (colours::purple.withAlpha (0.90f));
        g.drawRoundedRectangle (area, 15.0f, 1.6f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (shouldDrawHighlighted ? 0.16f : 0.09f));
        g.drawRoundedRectangle (area, 15.0f, 1.2f);
    }

    const auto iconColour = on ? colours::purpleSoft : colours::textDim;
    const auto textColour = on ? colours::textBright : colours::textDim;

    juce::Rectangle<float> iconArea (area.getCentreX() - 16.0f, area.getY() + area.getHeight() * 0.20f, 32.0f, 22.0f);
    IconButton::drawGlyph (g, glyph == Glyph::reverse ? IconType::reverse : IconType::bars,
                           iconArea, iconColour);

    drawText (g, getButtonText(),
              { area.getX(), area.getY() + area.getHeight() * 0.56f, area.getWidth(), area.getHeight() * 0.24f },
              uiFont (10.5f, 0.22f), textColour);
}

//==============================================================================
// PillToggle
//==============================================================================
PillToggle::PillToggle (const juce::String& buttonText)
    : juce::Button (buttonText), label (buttonText)
{
    setClickingTogglesState (true);
    setWantsKeyboardFocus (false);
}

void PillToggle::paintButton (juce::Graphics& g, bool shouldDrawHighlighted, bool /*shouldDrawDown*/)
{
    auto area = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    const float radius = area.getHeight() * 0.5f;

    g.setColour (on ? juce::Colour (0xff201735) : juce::Colour (0xff17161d));
    g.fillRoundedRectangle (area, radius);

    if (on)
    {
        g.setColour (colours::purple.withAlpha (0.12f));
        g.drawRoundedRectangle (area.expanded (3.0f), radius + 3.0f, 3.0f);
        g.setColour (colours::purple.withAlpha (0.85f));
        g.drawRoundedRectangle (area, radius, 1.4f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (shouldDrawHighlighted ? 0.16f : 0.09f));
        g.drawRoundedRectangle (area, radius, 1.1f);
    }

    // note badge
    const auto badge = juce::Rectangle<float> (area.getX() + 6.0f, area.getCentreY() - 9.0f, 18.0f, 18.0f);
    g.setColour (on ? colours::purple.withAlpha (0.22f) : juce::Colours::white.withAlpha (0.05f));
    g.fillEllipse (badge);
    IconButton::drawGlyph (g, IconType::note, badge.reduced (4.5f),
                           on ? colours::purpleSoft : colours::textDim);

    drawText (g, label,
              { badge.getRight() + 5.0f, area.getY(), area.getRight() - badge.getRight() - 7.0f, area.getHeight() },
              uiFont (9.5f, 0.10f), on ? colours::textBright : colours::textDim,
              juce::Justification::centredLeft);
}

//==============================================================================
// IconButton
//==============================================================================
IconButton::IconButton (IconType typeToUse) : juce::Button (""), type (typeToUse)
{
    setWantsKeyboardFocus (false);
}

void IconButton::paintButton (juce::Graphics& g, bool shouldDrawHighlighted, bool shouldDrawDown)
{
    auto area = getLocalBounds().toFloat();
    const auto circle = area.reduced (1.0f);
    const bool lit = shouldDrawHighlighted || shouldDrawDown;

    if (drawCircle)
    {
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.09f : 0.035f));
        g.fillEllipse (circle);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.22f : 0.13f));
        g.drawEllipse (circle.reduced (0.5f), 1.1f);
    }
    else if (lit)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillEllipse (circle);
    }

    const auto glyphArea = area.reduced (area.getWidth() * (drawCircle ? 0.28f : 0.20f));
    drawGlyph (g, type, glyphArea, iconColour.withAlpha (lit ? 1.0f : 0.9f));
}

void IconButton::drawGlyph (juce::Graphics& g, IconType type, juce::Rectangle<float> area, juce::Colour colour)
{
    if (area.isEmpty())
        return;

    const auto centre = area.getCentre();
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const float lineWidth = juce::jmax (1.0f, size * 0.10f);

    g.setColour (colour);

    switch (type)
    {
        case IconType::plus:
        {
            const float h = size * 0.5f;
            g.drawLine (centre.x - h, centre.y, centre.x + h, centre.y, lineWidth);
            g.drawLine (centre.x, centre.y - h, centre.x, centre.y + h, lineWidth);
            break;
        }

        case IconType::minus:
        {
            const float h = size * 0.5f;
            g.drawLine (centre.x - h, centre.y, centre.x + h, centre.y, lineWidth);
            break;
        }

        case IconType::gear:
        {
            const float radius = size * 0.30f;
            const float toothWidth = size * 0.11f;

            for (int i = 0; i < 8; ++i)
            {
                const float a = (float) i * juce::MathConstants<float>::twoPi / 8.0f;

                juce::Path tooth;
                tooth.addRoundedRectangle (-toothWidth * 0.5f, -radius - toothWidth * 0.95f,
                                           toothWidth, toothWidth * 1.55f, toothWidth * 0.3f);
                tooth.applyTransform (juce::AffineTransform::rotation (a).translated (centre.x, centre.y));
                g.fillPath (tooth);
            }

            g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, lineWidth * 0.85f);
            g.drawEllipse (centre.x - radius * 0.34f, centre.y - radius * 0.34f,
                           radius * 0.68f, radius * 0.68f, lineWidth * 0.85f);
            break;
        }

        case IconType::chevronLeft:
        case IconType::chevronRight:
        {
            const float w = size * 0.26f;
            const float h = size * 0.30f;
            const float direction = (type == IconType::chevronLeft) ? -1.0f : 1.0f;

            juce::Path chevron;
            chevron.startNewSubPath (centre.x - direction * w * 0.5f, centre.y - h);
            chevron.lineTo (centre.x + direction * w * 0.5f, centre.y);
            chevron.lineTo (centre.x - direction * w * 0.5f, centre.y + h);
            g.strokePath (chevron, juce::PathStrokeType (lineWidth * 1.1f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            break;
        }

        case IconType::note:
        {
            const float headRadius = size * 0.20f;
            const float stemTop = centre.y - size * 0.40f;

            juce::Path stem;
            stem.startNewSubPath (centre.x + headRadius * 0.9f, centre.y + headRadius * 0.3f);
            stem.lineTo (centre.x + headRadius * 0.9f, stemTop);
            stem.lineTo (centre.x + headRadius * 2.1f, stemTop + headRadius * 0.55f);
            g.strokePath (stem, juce::PathStrokeType (lineWidth * 0.9f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

            g.fillEllipse (centre.x - headRadius * 0.6f, centre.y + headRadius * 0.1f,
                           headRadius * 2.0f, headRadius * 1.5f);
            break;
        }

        case IconType::reverse:
        {
            const float w = size * 0.26f;
            const float h = size * 0.30f;

            for (int i = 0; i < 2; ++i)
            {
                const float offset = (i == 0 ? -w * 0.62f : w * 0.62f);

                juce::Path triangle;
                triangle.startNewSubPath (centre.x + offset + w * 0.5f, centre.y - h);
                triangle.lineTo (centre.x + offset - w * 0.5f, centre.y);
                triangle.lineTo (centre.x + offset + w * 0.5f, centre.y + h);
                triangle.closeSubPath();
                g.fillPath (triangle);
            }
            break;
        }

        case IconType::loop:
        {
            const float radius = size * 0.34f;

            juce::Path arc;
            arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                               juce::MathConstants<float>::pi * 0.85f,
                               juce::MathConstants<float>::pi * 1.95f, true);
            g.strokePath (arc, juce::PathStrokeType (lineWidth * 0.95f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            juce::Path arrow;
            arrow.startNewSubPath (centre.x + radius * 0.05f, centre.y - radius - lineWidth);
            arrow.lineTo (centre.x + radius * 0.62f, centre.y - radius * 0.72f);
            arrow.lineTo (centre.x - radius * 0.05f, centre.y - radius * 0.32f);
            g.strokePath (arrow, juce::PathStrokeType (lineWidth * 0.95f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            break;
        }

        case IconType::bars:
        default:
        {
            static const float heights[] { 0.34f, 0.72f, 1.0f, 0.52f, 0.30f };
            const int numBars = (int) (sizeof (heights) / sizeof (heights[0]));
            const float totalWidth = size * 0.82f;
            const float step = totalWidth / (float) numBars;

            for (int i = 0; i < numBars; ++i)
            {
                const float h = size * heights[i];
                const float x = centre.x - totalWidth * 0.5f + step * ((float) i + 0.5f) - lineWidth * 0.35f;

                g.fillRoundedRectangle (x, centre.y - h * 0.5f, lineWidth * 0.7f, h,
                                        lineWidth * 0.35f);
            }
            break;
        }
    }
}

//==============================================================================
// StepperRow
//==============================================================================
StepperRow::StepperRow (juce::RangedAudioParameter& parameter,
                        juce::String captionText,
                        IconType iconType,
                        double stepInValueUnits)
    : param (parameter),
      caption (std::move (captionText)),
      icon (iconType),
      step (stepInValueUnits)
{
    addAndMakeVisible (previous);
    addAndMakeVisible (next);

    previous.onClick = [this] { nudge (-1); };
    next.onClick     = [this] { nudge (1); };

    attachment = std::make_unique<juce::ParameterAttachment> (param, [this] (float newValue)
    {
        valueText = param.getText (param.convertTo0to1 (newValue), 16);
        repaint();
    });

    attachment->sendInitialUpdate();
}

StepperRow::~StepperRow() = default;

void StepperRow::nudge (int direction)
{
    const auto range = param.getNormalisableRange();
    const double current = (double) param.convertFrom0to1 (param.getValue());
    const double target = juce::jlimit ((double) range.start, (double) range.end,
                                        current + step * (double) direction);

    attachment->setValueAsCompleteGesture (range.snapToLegalValue ((float) target));
}

void StepperRow::resized()
{
    auto area = getLocalBounds();
    previous.setBounds (area.getRight() - 64, area.getY() + 22, 22, 22);
    next.setBounds     (area.getRight() - 40, area.getY() + 22, 22, 22);
}

void StepperRow::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    const auto badge = juce::Rectangle<float> (area.getX(), area.getY() + 17.0f, 28.0f, 28.0f);
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.fillEllipse (badge);
    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.drawEllipse (badge.reduced (0.6f), 1.1f);
    IconButton::drawGlyph (g, icon, badge.reduced (7.5f), colours::textDim);

    drawText (g, caption,
              { area.getX() + 38.0f, area.getY() + 1.0f, 110.0f, 18.0f },
              uiFont (9.0f, 0.26f), colours::textFaint, juce::Justification::centredLeft);

    drawText (g, valueText,
              { area.getX() + 38.0f, area.getY() + 19.0f, 110.0f, 24.0f },
              uiFont (13.0f, 0.05f), colours::textBright, juce::Justification::centredLeft);
}

} // namespace mirrora

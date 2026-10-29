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
    // Analog-style knob: tick scale, chrome bezel, fluted (knurled) body,
    // domed cap and a bright pointer.
    const float margin   = large ? 7.0f : 5.0f;      // space for the tick marks
    const float diameter = juce::jmax (12.0f, juce::jmin (area.getWidth(), area.getHeight()) - margin * 2.0f);
    const auto  centre   = area.getCentre();

    const float tickRadius = diameter * 0.5f;
    const float ringRadius = tickRadius - (large ? 5.5f : 4.5f);      // glowing value ring
    const float bezelRadius = ringRadius - (large ? 4.5f : 3.5f);     // chrome bezel
    const float bodyRadius  = bezelRadius - (large ? 6.5f : 5.0f);    // fluted body

    const float ringWidth = large ? 3.4f : 2.4f;

    const auto rotary = getRotaryParameters();
    const float startAngle = rotary.startAngleRadians;
    const float endAngle   = rotary.endAngleRadians;
    const float proportion = juce::jlimit (0.0f, 1.0f, (float) valueToProportionOfLength (getValue()));
    const float angle      = startAngle + proportion * (endAngle - startAngle);

    //---- tick scale ---------------------------------------------------------
    constexpr int numTicks = 11;

    for (int i = 0; i < numTicks; ++i)
    {
        const float t = (float) i / (float) (numTicks - 1);
        const float tickAngle = startAngle + t * (endAngle - startAngle);
        const bool passed = t <= proportion + 1.0e-4f;

        const float inner = tickRadius - (large ? 1.0f : 0.8f);
        const float outer = tickRadius + (large ? 3.6f : 2.8f);

        g.setColour (passed ? accent.withAlpha (0.95f) : juce::Colours::white.withAlpha (0.16f));
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (inner, tickAngle),
                                       centre.getPointOnCircumference (outer, tickAngle)),
                    large ? 1.9f : 1.4f);
    }

    //---- glowing value ring -------------------------------------------------
    const auto trackArc = makeArc (centre, ringRadius, startAngle, endAngle);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.strokePath (trackArc, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    if (proportion > 0.002f)
    {
        const auto valueArc = makeArc (centre, ringRadius, startAngle,
                                       juce::jmax (startAngle + 0.02f, angle));
        drawGlowPath (g, valueArc, accent, ringWidth, ringWidth * 4.5f, 0.30f);
    }

    //---- chrome bezel -------------------------------------------------------
    {
        g.setColour (juce::Colours::black.withAlpha (0.60f));
        g.fillEllipse (centre.x - bezelRadius - 1.5f, centre.y - bezelRadius - 0.5f,
                       (bezelRadius + 1.5f) * 2.0f, (bezelRadius + 1.5f) * 2.0f);

        juce::ColourGradient chrome (juce::Colour (0xff8f8e9a),
                                     centre.x - bezelRadius * 0.5f, centre.y - bezelRadius * 0.7f,
                                     juce::Colour (0xff191820),
                                     centre.x + bezelRadius * 0.6f, centre.y + bezelRadius * 0.8f,
                                     true);
        chrome.addColour (0.45, juce::Colour (0xff45444e));
        g.setGradientFill (chrome);
        g.fillEllipse (centre.x - bezelRadius, centre.y - bezelRadius,
                       bezelRadius * 2.0f, bezelRadius * 2.0f);

        juce::Path bezelPath;
        bezelPath.addEllipse (centre.x - bezelRadius, centre.y - bezelRadius,
                              bezelRadius * 2.0f, bezelRadius * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.strokePath (bezelPath, juce::PathStrokeType (1.0f));
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawEllipse (centre.x - bezelRadius + 1.0f, centre.y - bezelRadius + 1.0f,
                       (bezelRadius - 1.0f) * 2.0f, (bezelRadius - 1.0f) * 2.0f, 1.2f);
    }

    //---- fluted body --------------------------------------------------------
    juce::Path bodyPath;
    bodyPath.addEllipse (centre.x - bodyRadius, centre.y - bodyRadius,
                         bodyRadius * 2.0f, bodyRadius * 2.0f);

    {
        juce::Graphics::ScopedSaveState clipScope (g);
        g.reduceClipRegion (bodyPath);

        // base dome
        juce::ColourGradient base (juce::Colour (0xff9a99a5),
                                   centre.x - bodyRadius * 0.34f, centre.y - bodyRadius * 0.46f,
                                   juce::Colour (0xff0b0a10),
                                   centre.x + bodyRadius * 0.95f, centre.y + bodyRadius * 0.95f,
                                   true);
        base.addColour (0.18, juce::Colour (0xff77767f));
        base.addColour (0.44, juce::Colour (0xff38373f));
        base.addColour (0.72, juce::Colour (0xff1d1c23));
        g.setGradientFill (base);
        g.fillRect (area);

        // knurled flutes that rotate with the knob
        const int numFlutes = large ? 44 : 32;
        const float fluteStep = juce::MathConstants<float>::twoPi / (float) numFlutes;

        juce::Graphics::ScopedSaveState rotateScope (g);
        g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));

        for (int i = 0; i < numFlutes; ++i)
        {
            const float a0 = (float) i * fluteStep;
            const float a1 = a0 + fluteStep * 0.5f;

            juce::Path ridge;
            ridge.addPieSegment (centre.x - bodyRadius, centre.y - bodyRadius,
                                 bodyRadius * 2.0f, bodyRadius * 2.0f,
                                 a0 - juce::MathConstants<float>::halfPi,
                                 a1 - juce::MathConstants<float>::halfPi,
                                 0.42f);

            g.setColour (juce::Colours::white.withAlpha (0.115f));
            g.fillPath (ridge);

            juce::Path groove;
            groove.addPieSegment (centre.x - bodyRadius, centre.y - bodyRadius,
                                  bodyRadius * 2.0f, bodyRadius * 2.0f,
                                  a1 - juce::MathConstants<float>::halfPi,
                                  (a1 + fluteStep * 0.5f) - juce::MathConstants<float>::halfPi,
                                  0.42f);
            g.setColour (juce::Colours::black.withAlpha (0.38f));
            g.fillPath (groove);
        }

        // specular bloom
        juce::ColourGradient specular (juce::Colours::white.withAlpha (0.20f),
                                       centre.x - bodyRadius * 0.36f, centre.y - bodyRadius * 0.44f,
                                       juce::Colours::transparentWhite,
                                       centre.x - bodyRadius * 0.36f, centre.y - bodyRadius * 0.44f,
                                       true);
        specular.addColour (0.40, juce::Colours::white.withAlpha (0.06f));
        g.setGradientFill (specular);
        g.fillRect (area);

        // edge shading so the cap looks domed
        juce::ColourGradient vignette (juce::Colours::transparentBlack, centre.x, centre.y,
                                       juce::Colours::black.withAlpha (0.60f),
                                       centre.x, centre.y - bodyRadius, true);
        vignette.addColour (0.62, juce::Colours::transparentBlack);
        g.setGradientFill (vignette);
        g.fillRect (area);
    }

    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.strokePath (bodyPath, juce::PathStrokeType (1.1f));

    // glossy highlight on the top-left shoulder
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.strokePath (makeArc (centre, bodyRadius * 0.80f,
                           juce::MathConstants<float>::pi * 1.12f,
                           juce::MathConstants<float>::pi * 1.40f),
                  juce::PathStrokeType (large ? 2.6f : 1.9f,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    //---- centre cap ---------------------------------------------------------
    const float capRadius = bodyRadius * 0.20f;

    juce::ColourGradient cap (juce::Colour (0xff4a4954), centre.x - capRadius * 0.4f, centre.y - capRadius * 0.5f,
                              juce::Colour (0xff121118), centre.x + capRadius * 0.7f, centre.y + capRadius * 0.7f,
                              true);
    g.setGradientFill (cap);
    g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (centre.x - capRadius * 0.55f, centre.y - capRadius * 0.75f,
                   capRadius * 0.9f, capRadius * 0.9f, 1.0f);

    //---- pointer ------------------------------------------------------------
    const auto tip   = centre.getPointOnCircumference (bodyRadius * 0.92f, angle);
    const auto inner = centre.getPointOnCircumference (bodyRadius * 0.34f, angle);

    g.setColour (accent.withAlpha (0.35f));
    g.drawLine (juce::Line<float> (inner, tip), large ? 7.5f : 6.0f);
    g.setColour (colours::purpleSoft);
    g.drawLine (juce::Line<float> (inner, tip), large ? 3.4f : 2.7f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine (juce::Line<float> (inner, tip), 1.1f);

    g.setColour (accent);
    g.fillEllipse (tip.x - (large ? 3.0f : 2.3f), tip.y - (large ? 3.0f : 2.3f),
                   large ? 6.0f : 4.6f, large ? 6.0f : 4.6f);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.fillEllipse (tip.x - 1.0f, tip.y - 1.0f, 2.0f, 2.0f);
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

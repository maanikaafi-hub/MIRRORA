/*
    MIRRORA — visual language (colours, fonts, small drawing helpers).
    Everything is drawn with vector graphics so the GUI stays crisp at any zoom.
*/
#pragma once

#include <JuceHeader.h>

namespace mirrora
{

namespace layout
{
    inline constexpr float designWidth  = 960.0f;
    inline constexpr float designHeight = 570.0f;
}

namespace colours
{
    inline const juce::Colour windowBg    { 0xff060509 };
    inline const juce::Colour panelTop    { 0xff202027 };
    inline const juce::Colour panelBottom { 0xff131218 };
    inline const juce::Colour panelLine   { 0xff2a2833 };
    inline const juce::Colour purple      { 0xffa175ff };
    inline const juce::Colour purpleDeep  { 0xff7a4bdc };
    inline const juce::Colour purpleSoft  { 0xffd7c2ff };
    inline const juce::Colour textBright  { 0xffede9f6 };
    inline const juce::Colour textDim     { 0xffa49eb4 };
    inline const juce::Colour textFaint   { 0xff7b7489 };
    inline const juce::Colour scopeBg     { 0xff09080e };
    inline const juce::Colour metalLight  { 0xff55555f };
    inline const juce::Colour metalDark   { 0xff0e0d12 };
}

/** First available "thin / modern" sans-serif family on this machine. */
inline const juce::String& sansFamily()
{
    static const juce::String family = []
    {
        const auto names = juce::Font::findAllTypefaceNames();

        const juce::StringArray candidates { "Helvetica Neue", "SF Pro Display", "SF Pro Text",
                                             "Segoe UI", "Roboto", "Inter", "Avenir Next",
                                             "DejaVu Sans", "Arial", "Verdana" };

        for (const auto& candidate : candidates)
            if (names.contains (candidate))
                return candidate;

        return juce::Font::getDefaultSansSerifFontName();
    }();

    return family;
}

/** UI font with optional letter-spacing (the MIRRORA look relies on wide tracking). */
inline juce::Font uiFont (float height, float kerning = 0.0f, bool bold = false)
{
    const auto options = juce::FontOptions (sansFamily(), height,
                                            bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font (options).withExtraKerningFactor (kerning);
}

inline void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                      const juce::Font& font, juce::Colour colour,
                      juce::Justification justification = juce::Justification::centred)
{
    g.setFont (font);
    g.setColour (colour);
    g.drawText (text, area, justification, false);
}

/** Strokes a path several times with falling alpha to fake a soft bloom. */
inline void drawGlowPath (juce::Graphics& g, const juce::Path& path, juce::Colour colour,
                          float coreWidth, float glowWidth, float glowAlpha)
{
    if (glowWidth > coreWidth)
    {
        const int steps = 4;

        for (int i = steps; i >= 1; --i)
        {
            const float t = (float) i / (float) steps;
            const float width = coreWidth + (glowWidth - coreWidth) * t;

            g.setColour (colour.withAlpha (glowAlpha * (1.0f - t) / (float) steps * 3.0f));
            g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }

    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (coreWidth, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

inline juce::Path makeArc (juce::Point<float> centre, float radius,
                           float fromRadians, float toRadians)
{
    juce::Path p;
    p.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, fromRadians, toRadians, true);
    return p;
}

} // namespace mirrora

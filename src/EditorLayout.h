#pragma once

#include <juce_core/juce_core.h>

#include "MeterSettings.h"

/** Window geometry and chrome of the editor.

    Single source of truth: the editor, the persisted window sizes
    (`EditorWindowSizes`) and the offscreen preview tool all read the numbers
    from here, so the sizes they use can never drift apart.

    All values are logical pixels. The host applies its own display scale, so a
    saved size means the same layout on every monitor.
*/
namespace EditorLayout
{
    /** Padding between the window edge and everything inside it. */
    constexpr int outerPadding = 6;

    /** Height of the reserved control strip. Vertical and Compact keep a strip
        row for the icon buttons; Horizontal overlays them on the corners
        instead (see `overlaysControls`). */
    constexpr int controlStripHeight = 22;

    /** Clickable size of the Reset / Settings / About icon buttons. */
    constexpr int buttonSize = 20;
    constexpr int buttonGap = 2;

    /** Space the overlaid buttons reserve at the sides of the meter's own rows:
        one button plus breathing room. Used by the Horizontal TP / LS / LM row
        and by the Compact reading capsule, both of which sit between the
        left-hand About / Reset buttons and the right-hand Settings button. */
    constexpr int overlaidRowInset = buttonSize + 6;

    /** Geometry of one display mode: the window it opens at, and the smallest
        window it may be dragged to. The minimum is enforced by the editor's
        constrainer and also clamps a restored window size. */
    struct ModeSize
    {
        int preferredWidth  = 0;
        int preferredHeight = 0;
        int minWidth        = 0;
        int minHeight       = 0;
    };

    /** Vertical is a slim column (height unchanged), Horizontal a short strip
        (width unchanged), Compact a small square. The Horizontal minimum width
        keeps the TP / LS / LM row clear of the overlaid corner buttons. */
    inline ModeSize forMode (MeterDisplayMode mode) noexcept
    {
        switch (mode)
        {
            case MeterDisplayMode::vertical:   return { 176, 360, 156, 300 };
            case MeterDisplayMode::compact:    return { 200, 176, 176, 160 };

            case MeterDisplayMode::horizontal:
            default:                           return { 430, 120, 360, 112 };
        }
    }

    /** Largest window the editor may be dragged to. */
    constexpr int maxWindowSize = 4000;

    /** True when the icon buttons are drawn over the meter's empty top corners
        instead of taking a strip row of their own. Horizontal only: that is
        what lets the window stay short. */
    inline bool overlaysControls (MeterDisplayMode mode) noexcept
    {
        return mode == MeterDisplayMode::horizontal;
    }

    /** Placement of the three icon buttons inside the inner area (the window
        minus `outerPadding`). About sits in the top-left corner and Settings in
        the top-right one in every mode; Reset moves out of the way of the
        meter:

        - Vertical and Horizontal: the bottom-left corner, under the scale.
        - Compact: the left edge, vertically centred, beside the reading.

        The editor positions its buttons with this, and the preview tool draws
        its buttons with it too, so a render shows exactly what a host shows. */
    struct ControlPlacement
    {
        int aboutX = 0, aboutY = 0;
        int resetX = 0, resetY = 0;
        int settingsX = 0, settingsY = 0;
    };

    inline ControlPlacement controlsIn (int innerWidth, int innerHeight,
                                        MeterDisplayMode mode) noexcept
    {
        const int topY = (controlStripHeight - buttonSize) / 2;
        const int rightX = juce::jmax (0, innerWidth - buttonSize);

        // Compact keeps the reading centred, so Reset goes to the left edge at
        // half height; the other modes put it in the bottom-left corner.
        const int resetY = mode == MeterDisplayMode::compact
                               ? juce::jmax (0, (innerHeight - buttonSize) / 2)
                               : juce::jmax (0, innerHeight - buttonSize);

        return { 0, topY, 0, resetY, rightX, topY };
    }

    /** The horizontal room the overlaid buttons take from the rows the meter
        draws beside them — the Horizontal TP / LS / LM row and the Compact
        reading capsule. Vertical's readouts sit beside the bar and its buttons
        are in the corners, so it reserves nothing. */
    inline int rowInsetFor (MeterDisplayMode mode) noexcept
    {
        switch (mode)
        {
            case MeterDisplayMode::horizontal:
            case MeterDisplayMode::compact:    return overlaidRowInset;

            case MeterDisplayMode::vertical:
            default:                           return 0;
        }
    }
}

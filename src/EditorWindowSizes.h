#pragma once

#include <juce_core/juce_core.h>

#include "EditorLayout.h"
#include "MeterSettings.h"

/** The window size the user last dragged each display mode to.

    Hosts recreate the editor whenever they reopen a plug-in window, and the
    editor then reports the size it was constructed with — so unless the size is
    part of the plug-in state, a host (REAPER included) has nothing to restore
    and the window snaps back to its default. The editor therefore stores the
    size it was given per display mode and writes it into the state chunk; see
    docs/decisions/0009-editor-window-size-persistence.md.

    A size of 0 means "this mode has never been resized": the editor opens at the
    mode's preferred size. Sizes are logical pixels and are clamped to the
    mode's limits on load, so a chunk written by another build (or a damaged
    one) can never produce an unusable window.
*/
class EditorWindowSizes
{
public:
    struct Size
    {
        int width  = 0;
        int height = 0;

        bool isUnset() const noexcept { return width <= 0 || height <= 0; }

        bool operator== (const Size& other) const noexcept
        {
            return width == other.width && height == other.height;
        }

        bool operator!= (const Size& other) const noexcept { return ! (*this == other); }
    };

    Size sizeFor (MeterDisplayMode mode) const noexcept
    {
        return sizeRef (mode);
    }

    /** Remembers a size the user chose. Values are clamped to the mode's limits;
        a non-positive size is ignored, so a degenerate window (or a host that
        reports 0 before laying the view out) can never erase a valid memory. */
    void setSize (MeterDisplayMode mode, int width, int height) noexcept
    {
        if (width <= 0 || height <= 0)
            return;

        sizeRef (mode) = clamp (mode, { width, height });
    }

    /** Brings a size into the mode's allowed range. The editor's constrainer
        enforces the same limits, so this only guards the state chunk against a
        size the editor would refuse to restore. */
    static Size clamp (MeterDisplayMode mode, Size size) noexcept
    {
        const auto limits = EditorLayout::forMode (mode);

        return { juce::jlimit (limits.minWidth, EditorLayout::maxWindowSize, size.width),
                 juce::jlimit (limits.minHeight, EditorLayout::maxWindowSize, size.height) };
    }

    /** Forgets the mode's size, so it opens at its preferred size again. */
    void reset (MeterDisplayMode mode) noexcept { sizeRef (mode) = {}; }

    bool operator== (const EditorWindowSizes& other) const noexcept
    {
        for (int i = 0; i < numModes; ++i)
        {
            const auto mode = modeAt (i);

            if (sizeFor (mode) != other.sizeFor (mode))
                return false;
        }

        return true;
    }

    bool operator!= (const EditorWindowSizes& other) const noexcept
    {
        return ! (*this == other);
    }

    static constexpr const char* elementName = "windowSizes";

    /** Writes one `<mode width=".." height=".."/>` child per remembered size.
        The parent node is only created when at least one mode has been resized,
        so an untouched editor adds nothing to the state chunk. */
    void toXml (juce::XmlElement& parent) const
    {
        bool anySet = false;

        for (int i = 0; i < numModes; ++i)
        {
            const auto mode = modeAt (i);
            anySet = anySet || ! sizeFor (mode).isUnset();
        }

        if (! anySet)
            return;

        auto* node = parent.createNewChildElement (elementName);

        for (int i = 0; i < numModes; ++i)
        {
            const auto mode = modeAt (i);
            const auto size = sizeFor (mode);

            if (size.isUnset())
                continue;

            auto* child = node->createNewChildElement (modeName (mode));
            child->setAttribute ("width", size.width);
            child->setAttribute ("height", size.height);
        }
    }

    /** Reads what `toXml` wrote. A missing node, missing mode or invalid size
        all leave that mode unset (and therefore at its preferred size). */
    static EditorWindowSizes fromXml (const juce::XmlElement& parent)
    {
        EditorWindowSizes sizes;

        if (auto* node = parent.getChildByName (elementName))
            for (int i = 0; i < numModes; ++i)
            {
                const auto mode = modeAt (i);

                if (auto* child = node->getChildByName (modeName (mode)))
                    sizes.setSize (mode,
                                   child->getIntAttribute ("width", 0),
                                   child->getIntAttribute ("height", 0));
            }

        return sizes;
    }

    static juce::String modeName (MeterDisplayMode mode)
    {
        switch (mode)
        {
            case MeterDisplayMode::vertical:   return "vertical";
            case MeterDisplayMode::compact:    return "compact";

            case MeterDisplayMode::horizontal:
            default:                           return "horizontal";
        }
    }

    /** The three modes are the enum's own contiguous values (vertical = 0,
        horizontal = 1, compact = 2), which is also how they are stored and
        written to the state chunk. */
    static constexpr int numModes = 3;

    static constexpr MeterDisplayMode modeAt (int index) noexcept
    {
        return static_cast<MeterDisplayMode> (index);
    }

private:
    /** Index by the enum's own numbering (vertical = 0 … compact = 2), clamped
        so an out-of-range mode coming from a corrupt chunk cannot read past the
        array. `MeterDisplayMode` is guaranteed contiguous from 0. */
    Size& sizeRef (MeterDisplayMode mode) noexcept
    {
        return sizes_[juce::jlimit (0, numModes - 1, (int) mode)];
    }

    const Size& sizeRef (MeterDisplayMode mode) const noexcept
    {
        return sizes_[juce::jlimit (0, numModes - 1, (int) mode)];
    }

    Size sizes_[numModes] {};
};

#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
    /** Fills a TransportTracker block context from the host play head. Returns
        false when the host cannot report transport information at all. */
    bool readTransport (juce::AudioPlayHead* head, int numSamples, double sampleRate,
                        TransportTracker::BlockContext& ctx)
    {
        ctx.numSamples = numSamples;
        ctx.sampleRate = sampleRate;
        ctx.nowMs      = juce::Time::getMillisecondCounter();

        if (head == nullptr)
            return false;

        const auto position = head->getPosition();

        if (! position.hasValue())
            return false;

        ctx.transportPlaying = position->getIsPlaying();

        if (const auto seconds = position->getTimeInSeconds())
        {
            ctx.hasPosition     = true;
            ctx.positionSeconds = *seconds;
        }
        else if (const auto ppq = position->getPpqPosition())
        {
            // Fall back to musical time when a time base is unavailable.
            const double bpm = position->getBpm().orFallback (120.0);
            ctx.hasPosition     = true;
            ctx.positionSeconds = *ppq * 60.0 / (bpm > 0.0 ? bpm : 120.0);
        }

        return true;
    }
}

LufsMeterAudioProcessor::LufsMeterAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

LufsMeterAudioProcessor::~LufsMeterAudioProcessor() = default;

void LufsMeterAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    transportTracker_.reset();

    meter_.prepare (sampleRate, getTotalNumInputChannels());
}

void LufsMeterAudioProcessor::releaseResources()
{
    meter_.release();
}

bool LufsMeterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Metering is pass-through, so the input and output layouts must match.
    if (mainIn != mainOut)
        return false;

    if (mainOut.isDisabled())
        return false;

    return true;
}

void LufsMeterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    TransportTracker::BlockContext ctx;

    const bool transportKnown = readTransport (getPlayHead(), numSamples,
                                               currentSampleRate_, ctx);

    if (! transportKnown)
    {
        // A host that cannot report transport is treated as continuously
        // playing: the measurement accumulates exactly like a standalone
        // loudness meter and is only cleared by the manual Reset button.
        ctx.transportPlaying = true;
        ctx.hasPosition      = false;
    }

    if (transportTracker_.update (ctx))
    {
        // A new pass starts here. Discard anything queued from the previous
        // pass so no stale audio can leak into the new measurement; the reset
        // itself is performed on the measurement thread.
        meter_.requestReset (true);
    }

    if (! ctx.transportPlaying)
    {
        // Transport is stopped or paused: freeze the displayed value by not
        // feeding the meter. (Silence while *playing* is still measured.)
        return;
    }

    meter_.pushBlock (buffer, numSamples);
}

juce::AudioProcessorEditor* LufsMeterAudioProcessor::createEditor()
{
    return new LufsMeterAudioProcessorEditor (*this);
}

void LufsMeterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = settings_.toXml())
        copyXmlToBinary (*xml, destData);
}

void LufsMeterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        settings_ = MeterSettings::fromXml (*xml);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LufsMeterAudioProcessor();
}

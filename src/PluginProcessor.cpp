#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
    // Peak level below which the input is considered a genuine break in the
    // signal (roughly -100 dBFS). This detects hard silence, not quiet music.
    constexpr float silencePeakThreshold = 1.0e-5f;

    // How long a break must last before it counts as an interruption.
    constexpr double silenceHangoverSeconds = 0.5;

    // If processBlock is not called for this long (host stops processing while
    // the transport is stopped), the next call starts a new measurement.
    constexpr double blockGapResetMs = 1000.0;
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
    sessionInterrupted_ = true;
    silentFrames_ = 0;
    lastBlockTimeMs_ = juce::Time::getMillisecondCounter();

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

bool LufsMeterAudioProcessor::isTransportPlaying() const
{
    if (auto* head = getPlayHead())
    {
        if (const auto position = head->getPosition())
            return position->getIsPlaying();
    }

    // Host does not report transport state: assume playing and rely on signal
    // detection instead.
    return true;
}

void LufsMeterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    const bool playing = isTransportPlaying();

    float peak = 0.0f;

    for (int c = 0; c < buffer.getNumChannels(); ++c)
        peak = juce::jmax (peak, buffer.getMagnitude (c, 0, numSamples));

    const bool silent = peak < silencePeakThreshold;

    const double now = juce::Time::getMillisecondCounter();
    const double gapMs = now - lastBlockTimeMs_;
    lastBlockTimeMs_ = now;

    const bool active = playing && ! silent;

    if (! active)
    {
        // Interruption: keep the last trustworthy value visible (freeze) and
        // stop feeding the measurement.
        silentFrames_ = silent ? silentFrames_ + numSamples : 0;

        const int hangoverFrames = (int) (silenceHangoverSeconds * currentSampleRate_);

        if (! playing || silentFrames_ >= hangoverFrames)
            sessionInterrupted_ = true;

        return;
    }

    // Audio (re)started: begin a fresh measurement that excludes everything
    // accumulated before the interruption. Queued samples are kept because the
    // queue holds only new-session audio by now.
    if (sessionInterrupted_ || gapMs > blockGapResetMs)
    {
        meter_.requestReset (false);
        sessionInterrupted_ = false;
    }

    silentFrames_ = 0;
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

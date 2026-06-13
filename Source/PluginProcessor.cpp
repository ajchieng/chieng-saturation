#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Tilt voicing: high-shelf corner and Q used for both pre- and de-emphasis.
    constexpr float shelfFreq = 3500.0f;
    constexpr float shelfQ    = 0.5f;
}

//==============================================================================
ChiengSaturationAudioProcessor::ChiengSaturationAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    driveParam = apvts.getRawParameterValue ("drive");
}

ChiengSaturationAudioProcessor::~ChiengSaturationAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChiengSaturationAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.25f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f)) + " %";
            })));

    return layout;
}

//==============================================================================
void ChiengSaturationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = (size_t) juce::jmax (1, getTotalNumOutputChannels());

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        numChannels, oversampleStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // maximum quality
        true);  // integer latency, so we can report it cleanly to the host

    oversampler->initProcessing ((size_t) samplesPerBlock);
    oversampler->reset();
    setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()));

    oversampledRate = sampleRate * (double) oversampler->getOversamplingFactor();

    // DC blocker is fixed; shelves are (re)computed from Drive in processBlock.
    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (
        oversampledRate, 20.0f);

    for (auto& ch : chains)
    {
        ch.dcBlocker.coefficients = dcCoeffs;
        ch.dcBlocker.reset();
        ch.preShelf.reset();
        ch.deShelf.reset();
    }

    lastDrive = -1.0f; // force a shelf recompute on the first block
}

void ChiengSaturationAudioProcessor::releaseResources()
{
    if (oversampler != nullptr)
        oversampler->reset();
}

bool ChiengSaturationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

//==============================================================================
void ChiengSaturationAudioProcessor::updateShelves (float drive)
{
    // Pre-emphasis cuts highs into the saturator (less harsh); de-emphasis is the
    // exact complement, so the clean tone stays balanced while the generated
    // harmonics are smoother. Cut deepens from 0 dB to ~ -7 dB as Drive rises.
    const float cutGain = juce::Decibels::decibelsToGain (juce::jmap (drive, 0.0f, 1.0f, 0.0f, -7.0f));

    auto preCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        oversampledRate, shelfFreq, shelfQ, cutGain);
    auto deCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        oversampledRate, shelfFreq, shelfQ, 1.0f / cutGain);

    for (auto& ch : chains)
    {
        ch.preShelf.coefficients = preCoeffs;  // shared, ref-counted; state stays per-channel
        ch.deShelf.coefficients  = deCoeffs;
    }
}

void ChiengSaturationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const float drive = driveParam->load(); // 0..1

    // Drive-dependent voicing (computed once per block):
    const float pre  = juce::jmap (drive, 0.0f, 1.0f, 1.0f, 20.0f); // input drive
    const float bias = juce::jmap (drive, 0.0f, 1.0f, 0.0f, 0.35f); // asymmetry -> even harmonics

    // Normalise to unity small-signal gain so quiet passages A/B fairly, then a
    // gentle trim to tame the energy build-up at high drive.
    const float biasT   = std::tanh (bias);
    const float linGain = pre * (1.0f - biasT * biasT);
    const float outGain = (1.0f / linGain) * juce::jmap (drive, 0.0f, 1.0f, 1.0f, 0.7f);

    if (std::abs (drive - lastDrive) > 1.0e-4f)
    {
        updateShelves (drive);
        lastDrive = drive;
    }

    // ---- Oversampled non-linear processing -------------------------------
    juce::dsp::AudioBlock<float> block (buffer);
    auto osBlock = oversampler->processSamplesUp (block);

    const auto numCh   = (int) osBlock.getNumChannels();
    const auto numSamp = (int) osBlock.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto& chain = chains[(size_t) juce::jmin (ch, 1)];
        auto* data  = osBlock.getChannelPointer ((size_t) ch);

        for (int n = 0; n < numSamp; ++n)
        {
            float s = chain.preShelf.processSample (data[n]);     // tame highs
            s = std::tanh (pre * s + bias);                       // asymmetric saturation
            s = chain.dcBlocker.processSample (s);                // remove DC offset
            s = chain.deShelf.processSample (s);                  // restore brightness
            data[n] = s * outGain;                                // loudness compensation
        }
    }

    oversampler->processSamplesDown (block);
}

//==============================================================================
juce::AudioProcessorEditor* ChiengSaturationAudioProcessor::createEditor()
{
    return new ChiengSaturationAudioProcessorEditor (*this);
}

//==============================================================================
void ChiengSaturationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ChiengSaturationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChiengSaturationAudioProcessor();
}

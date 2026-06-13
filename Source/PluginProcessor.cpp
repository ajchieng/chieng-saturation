#include "PluginProcessor.h"
#include "PluginEditor.h"

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
void ChiengSaturationAudioProcessor::prepareToPlay (double, int) {}
void ChiengSaturationAudioProcessor::releaseResources() {}

bool ChiengSaturationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    // Accept mono or stereo, as long as input and output match.
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

void ChiengSaturationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // One knob: drive (0..1) -> pre-gain (1..24), then tanh soft-clip with
    // output compensation so a full-scale peak stays near unity as drive rises.
    const float drive = driveParam->load();
    const float pre   = juce::jmap (drive, 0.0f, 1.0f, 1.0f, 24.0f);
    const float comp  = 1.0f / std::tanh (pre);

    for (int ch = 0; ch < numIn; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            x[n] = std::tanh (pre * x[n]) * comp;
    }
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
// Entry point JUCE uses to create the plugin instance.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChiengSaturationAudioProcessor();
}

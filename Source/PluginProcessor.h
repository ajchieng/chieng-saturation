#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
// "Smart" one-knob saturation processor.
//
// A single Drive knob simultaneously controls the whole character:
//   pre-gain -> pre-emphasis tilt -> asymmetric tanh -> DC blocker
//   -> de-emphasis tilt -> loudness compensation,  all run 4x oversampled
//   to suppress aliasing.
class ChiengSaturationAudioProcessor : public juce::AudioProcessor
{
public:
    ChiengSaturationAudioProcessor();
    ~ChiengSaturationAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateShelves (float driveFactor);

    std::atomic<float>* driveParam = nullptr;

    // 4x oversampling (2 stages of 2x), polyphase IIR for low latency.
    static constexpr size_t oversampleStages = 2;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double oversampledRate = 0.0;

    // Per-channel filter chain (runs at the oversampled rate).
    struct ChannelChain
    {
        juce::dsp::IIR::Filter<float> preShelf;   // tames highs going into the saturator
        juce::dsp::IIR::Filter<float> dcBlocker;  // removes DC offset from asymmetry
        juce::dsp::IIR::Filter<float> deShelf;    // restores brightness afterwards
    };
    std::array<ChannelChain, 2> chains;

    float lastDrive = -1.0f; // so shelf coefficients only recompute when Drive moves

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChiengSaturationAudioProcessor)
};

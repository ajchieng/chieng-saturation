#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

//==============================================================================
// Live harmonic display.
//
// On a timer it reads the current Drive, replicates the saturation waveshaper
// on a reference sine, and runs a small DFT at each harmonic. The bars show the
// fundamental + overtones the curve generates; even harmonics (the "warmth"
// produced by the asymmetry) are coloured differently from the odd ones, so you
// can see exactly what the saturation is doing as you turn the knob.
class HarmonicMeter : public juce::Component,
                      private juce::Timer
{
public:
    explicit HarmonicMeter (ChiengSaturationAudioProcessor&);
    ~HarmonicMeter() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void computeHarmonics (float drive);

    ChiengSaturationAudioProcessor& proc;

    static constexpr int numHarmonics = 8;
    static constexpr int probeLength  = 1024;

    std::array<float, numHarmonics> targets {};   // normalised bar heights (0..1)
    std::array<float, numHarmonics> display {};    // smoothed values actually drawn
    float lastDrive = -2.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicMeter)
};

//==============================================================================
// One rotary "Drive" knob + the harmonic display.
class ChiengSaturationAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChiengSaturationAudioProcessorEditor (ChiengSaturationAudioProcessor&);
    ~ChiengSaturationAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider driveKnob;
    juce::Label  driveLabel;
    juce::Label  titleLabel;
    HarmonicMeter harmonicMeter;

    // Declared after driveKnob so the slider exists when the attachment binds to it.
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChiengSaturationAudioProcessorEditor)
};

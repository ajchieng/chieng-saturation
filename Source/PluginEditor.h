#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// A single rotary "Drive" knob.
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

    // Declared after driveKnob so the slider exists when the attachment binds to it.
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChiengSaturationAudioProcessorEditor)
};

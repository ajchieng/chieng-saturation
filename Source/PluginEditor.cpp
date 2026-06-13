#include "PluginEditor.h"

//==============================================================================
ChiengSaturationAudioProcessorEditor::ChiengSaturationAudioProcessorEditor (
    ChiengSaturationAudioProcessor& p)
    : AudioProcessorEditor (&p),
      driveAttachment (p.apvts, "drive", driveKnob)
{
    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    driveKnob.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff7a18));
    driveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3e));
    driveKnob.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    driveKnob.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    driveKnob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (driveKnob);

    driveLabel.setText ("DRIVE", juce::dontSendNotification);
    driveLabel.setJustificationType (juce::Justification::centred);
    driveLabel.setFont (juce::Font (juce::FontOptions().withHeight (15.0f).withStyle ("Bold")));
    driveLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0d0d4));
    addAndMakeVisible (driveLabel);

    titleLabel.setText ("CHIENG SATURATION", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions().withHeight (17.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    setSize (260, 300);
}

ChiengSaturationAudioProcessorEditor::~ChiengSaturationAudioProcessorEditor() = default;

//==============================================================================
void ChiengSaturationAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (juce::Colour (0xff2b2b2e), 0.0f, 0.0f,
                               juce::Colour (0xff141416), 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();
}

void ChiengSaturationAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (30));
    driveLabel.setBounds (area.removeFromBottom (24));
    driveKnob.setBounds (area.reduced (8));
}

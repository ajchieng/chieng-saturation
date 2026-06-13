#include "PluginEditor.h"

namespace
{
    // Colours shared by the meter.
    const juce::Colour kEven   { 0xffff7a18 }; // warm orange = even harmonics
    const juce::Colour kOdd    { 0xff3fd0c8 }; // teal       = odd harmonics
    const juce::Colour kPanel  { 0xff0e0e10 };
    const juce::Colour kStroke { 0xff2a2a2e };
}

//==============================================================================
HarmonicMeter::HarmonicMeter (ChiengSaturationAudioProcessor& p) : proc (p)
{
    startTimerHz (30);
}

HarmonicMeter::~HarmonicMeter()
{
    stopTimer();
}

void HarmonicMeter::computeHarmonics (float drive)
{
    // Must mirror the saturation voicing in PluginProcessor::processBlock.
    const float pre  = juce::jmap (drive, 0.0f, 1.0f, 1.0f, 20.0f);
    const float bias = juce::jmap (drive, 0.0f, 1.0f, 0.0f, 0.35f);

    constexpr float probeAmp = 0.7071f; // -3 dBFS reference sine
    constexpr int   N        = probeLength;
    constexpr float twoPiOverN = juce::MathConstants<float>::twoPi / (float) N;

    // One period of the waveshaped sine. (DC / 0th harmonic is ignored by the
    // DFT below, so the asymmetry's offset doesn't need removing explicitly.)
    std::array<float, N> wave;
    for (int n = 0; n < N; ++n)
    {
        const float s = probeAmp * std::sin (twoPiOverN * (float) n);
        wave[(size_t) n] = std::tanh (pre * s + bias);
    }

    // Magnitude of each harmonic via a direct DFT at k cycles per period.
    std::array<float, numHarmonics + 1> mag {}; // index 1..numHarmonics
    for (int k = 1; k <= numHarmonics; ++k)
    {
        float re = 0.0f, im = 0.0f;
        for (int n = 0; n < N; ++n)
        {
            const float phase = twoPiOverN * (float) (k * n);
            re += wave[(size_t) n] * std::cos (phase);
            im += wave[(size_t) n] * std::sin (phase);
        }
        mag[(size_t) k] = std::sqrt (re * re + im * im) * (2.0f / (float) N);
    }

    // Normalise to the fundamental and map to a -60..0 dB bar height.
    const float fund = juce::jmax (mag[1], 1.0e-9f);
    for (int k = 1; k <= numHarmonics; ++k)
    {
        const float db = juce::Decibels::gainToDecibels (mag[(size_t) k] / fund, -60.0f);
        targets[(size_t) (k - 1)] = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
    }
}

void HarmonicMeter::timerCallback()
{
    const float drive = proc.apvts.getRawParameterValue ("drive")->load();

    if (std::abs (drive - lastDrive) > 1.0e-4f)
    {
        computeHarmonics (drive);
        lastDrive = drive;
    }

    bool moved = false;
    for (size_t i = 0; i < (size_t) numHarmonics; ++i)
    {
        const float delta = targets[i] - display[i];
        if (std::abs (delta) > 0.001f)
        {
            display[i] += delta * 0.25f;
            moved = true;
        }
    }

    if (moved)
        repaint();
}

void HarmonicMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (kPanel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (kStroke);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    auto area = bounds.reduced (10.0f);

    // Header: title on the left, even/odd legend on the right.
    auto header = area.removeFromTop (16.0f);
    g.setColour (juce::Colour (0xffd0d0d4));
    g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f).withStyle ("Bold")));
    g.drawText ("HARMONICS", header, juce::Justification::centredLeft);

    auto legend = header.removeFromRight (96.0f);
    g.setFont (juce::Font (juce::FontOptions().withHeight (10.0f)));
    auto drawSwatch = [&] (juce::Rectangle<float> r, juce::Colour c, const juce::String& t)
    {
        auto dot = r.removeFromLeft (8.0f).withSizeKeepingCentre (8.0f, 8.0f);
        g.setColour (c);
        g.fillRoundedRectangle (dot, 2.0f);
        g.setColour (juce::Colour (0xff9a9aa0));
        g.drawText (t, r.withTrimmedLeft (2.0f), juce::Justification::centredLeft);
    };
    drawSwatch (legend.removeFromLeft (44.0f), kEven, "even");
    drawSwatch (legend.removeFromLeft (44.0f), kOdd,  "odd");

    // Plot area (leave room for the index labels along the bottom).
    constexpr float labelH = 13.0f;
    auto plot = area;
    auto labels = plot.removeFromBottom (labelH);

    // Faint dB gridlines at -12 / -24 / -36 / -48.
    g.setColour (kStroke.withAlpha (0.6f));
    for (int dbLine : { -12, -24, -36, -48 })
    {
        const float y = juce::jmap ((float) dbLine, -60.0f, 0.0f, plot.getBottom(), plot.getY());
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    const float slot = plot.getWidth() / (float) numHarmonics;
    const float barW = slot * 0.5f;
    const float plotH = plot.getHeight();

    for (int i = 0; i < numHarmonics; ++i)
    {
        const int   k  = i + 1;
        const float cx = plot.getX() + slot * ((float) i + 0.5f);
        const float h  = juce::jlimit (0.0f, 1.0f, display[(size_t) i]) * plotH;

        juce::Rectangle<float> bar (cx - barW * 0.5f, plot.getBottom() - h, barW, h);

        const juce::Colour c = (k == 1) ? juce::Colours::white
                                        : (k % 2 == 0 ? kEven : kOdd);
        g.setColour (c.withAlpha (0.92f));
        g.fillRoundedRectangle (bar, 1.5f);

        g.setColour (juce::Colour (0xff7a7a82));
        g.setFont (juce::Font (juce::FontOptions().withHeight (10.0f)));
        g.drawText (juce::String (k),
                    juce::Rectangle<float> (cx - slot * 0.5f, labels.getY(), slot, labelH),
                    juce::Justification::centred);
    }
}

//==============================================================================
ChiengSaturationAudioProcessorEditor::ChiengSaturationAudioProcessorEditor (
    ChiengSaturationAudioProcessor& p)
    : AudioProcessorEditor (&p),
      harmonicMeter (p),
      driveAttachment (p.apvts, "drive", driveKnob)
{
    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    driveKnob.setColour (juce::Slider::rotarySliderFillColourId, kEven);
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

    addAndMakeVisible (harmonicMeter);

    setSize (300, 440);
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
    auto area = getLocalBounds().reduced (18);
    titleLabel.setBounds (area.removeFromTop (26));
    harmonicMeter.setBounds (area.removeFromBottom (160));
    area.removeFromBottom (8);
    driveLabel.setBounds (area.removeFromBottom (22));
    driveKnob.setBounds (area);
}

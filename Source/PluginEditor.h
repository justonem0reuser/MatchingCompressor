#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MCLookAndFeel.h"
#include "MatchWindow.h"
#include "Components/CurvePlotComponent.h"
#include "Components/SliderWithAttachment.h"
#include "Components/ComboBoxWithAttachment.h"

//==============================================================================
class MatchCompressorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            public juce::ComponentListener
{
public:
    MatchCompressorAudioProcessorEditor (MatchCompressorAudioProcessor&);
    ~MatchCompressorAudioProcessorEditor();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const int margin = 10;
    const int leftPanelWidth = 430;
    const int rightPanelWidth = 300;
    const int sliderWidth = 128;
    const int sliderHeight = sliderWidth + 30;
    const int matchButtonSize = 60;
    const int kneeIndexButtonSize = 46;
    const int comboBoxHeight = 25;
    const int labelWidth = 50;
    const int resetButtonHeight = 30;

    juce::Component::SafePointer<MatchCompressorAudioProcessorEditor> safePtr { this };

    MatchCompressorAudioProcessor& audioProcessor;

    std::vector<ParameterInfo> parameterInfos;

    juce::ValueTree properties, unchangedProperties;

    std::unique_ptr<MatchWindow> matchWindow;

    std::vector<float> calculatedCompParams;

    // Components
    juce::ImageButton matchButton;
    juce::TextButton resetButton;
    std::array<std::unique_ptr<juce::ImageButton>, DynamicShaper<float>::maxKneesNumber> kneeIndexButtons;
    std::array<std::unique_ptr<juce::Label>, DynamicShaper<float>::maxKneesNumber> kneeIndexLabels;
    juce::Label
        kneesNumberLabel,
        balFilterTypeLabel, 
        channelAggregationTypeLabel;
    SliderWithAttachment 
        thresholdSlider, 
        gainSlider, 
        kneeWidthSlider, 
        ratioSlider, 
        attackSlider, 
        releaseSlider;
    ComboBoxWithAttachment 
        kneesNumberComboBox,
        balFilterTypeComboBox, 
        channelAggregationTypeComboBox;
    std::unique_ptr<CurvePlotComponent> freeFormCurve;

    // Look and feel properties
    MCLookAndFeel laf;
    juce::Image
        leftPanelBackground,
        rightPanelBackground,
        toggleOffImage,
        toggleOnImage;
    juce::Rectangle<float> groupRect;
    juce::Slider::RotaryParameters standardRotaryParameters; // this should be after sliders because of the initializing order

    void fillDefaultProperties();
    void resetToCalculatedData();

    // Sliders and knee index buttons manipulating
    int getCheckedButtonIndex();
    void updateKneeIndexButtonsVisibility();
    void updateAttachments();
    void updateSliderBounds(
        SliderWithAttachment& slider,
        const juce::NormalisableRange<float>& fullRange,
        float minValue, 
        float maxValue);
    void updateSlidersBounds(
        int index,
        bool updateThreshold,
        bool updateKneeWidth);

    // working with MatchWindow
    void matchButtonClicked();
    void componentVisibilityChanged(Component& component) override;
    void endCollectingData(bool saveData, bool resetButtonsState);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchCompressorAudioProcessorEditor)
};

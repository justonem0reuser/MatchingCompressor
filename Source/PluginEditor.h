#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MCLookAndFeel.h"
#include "MatchWindow.h"
#include "Components/CurvePlotComponent.h"
#include "Components/SliderWithAttachment.h"
#include "Components/ComboBoxWithAttachment.h"
#include "Components/BaseMainView.h"
#include "Controllers/MatchController.h"
#include "Data/CurveData.h"
#include "Data/CurveBounds.h"
#include "Controllers/MainController.h"

//==============================================================================
class MatchCompressorAudioProcessorEditor : 
    public juce::AudioProcessorEditor,                                        
    public BaseMainView
{
public:
    MatchCompressorAudioProcessorEditor (MatchCompressorAudioProcessor&);
    ~MatchCompressorAudioProcessorEditor();

    void paint (juce::Graphics&) override;
    void resized() override;

    BaseMatchView* getMatchView() override;

private:
    const int margin = 10;
    const int leftPanelWidth = 446;
    const int rightPanelWidth = 300;
    const int sliderImageWidth = 128;
    const int sliderWidth = 140;
    const int sliderHeight = sliderImageWidth + 30;
    const int matchButtonSize = 60;
    const int kneeIndexButtonSize = 46;
    const int comboBoxHeight = 25;
    const int labelWidth = 50;
    const int resetButtonHeight = 30;

    MatchCompressorAudioProcessor& audioProcessor;

    std::unique_ptr<MatchWindow> matchWindow;

    std::unique_ptr<MainController> mainController;

    // Components
    juce::ImageButton toolButton;
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

    void createController();
    void resetToCalculatedData();
    void toolButtonClicked();

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
        int fromIndex,
        bool updateThreshold,
        bool updateKneeWidth);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchCompressorAudioProcessorEditor)
};

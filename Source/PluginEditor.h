#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MatchWindow.h"
#include "Components/CurvePlotComponent.h"
#include "Components/SliderWithAttachment.h"
#include "Components/ButtonChoiceComponent.h"
#include "Components/ButtonChoiceWithAttachment.h"
#include "Components/BaseMainView.h"
#include "Controllers/MatchController.h"
#include "Data/CurveData.h"
#include "Controllers/MainController.h"
#include "LookAndFeel/MCLookAndFeel.h"

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
    enum class Mode { normal, fixed };

    const int margin = 10;
    const int leftPanelWidth = 586;
    const int rightPanelWidth = 300;
    const int sliderImageWidth = 128;
    const int sliderWidth = 140;
    const int sliderHeight = sliderImageWidth + 30;
    const int hpfButtonsHeight = 20;
    const int matchButtonSize = 75;
    const int kneeIndexButtonSize = 46;
    const int comboBoxHeight = 25;
    const int labelWidth = 50;
    const int resetButtonHeight = 30;
    const int themeButtonHeight = 78;
    const int themeButtonWidth = 100;
    const int fixationButtonWidth = 200;
    const int editorHeight = 580;

    MatchCompressorAudioProcessor& audioProcessor;

    std::unique_ptr<MatchWindow> matchWindow;

    std::unique_ptr<MainController> mainController;

    // Components
    juce::ImageButton toolButton;
    juce::TextButton resetButton;
    ButtonChoiceComponent themeButtons;
    ButtonChoiceComponent modeButtons;
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
        hpfSlider,
        attackSlider, 
        releaseSlider;
    ButtonChoiceWithAttachment
        kneesNumberButtons,
        balFilterTypeButtons,
        channelAggregationTypeButtons,
        hpfButtons;
    std::unique_ptr<CurvePlotComponent> freeFormCurve;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    Mode mode = Mode::normal;
    bool restoringCalculatedData = false;

    // Look and feel properties
    std::unique_ptr<MCLookAndFeel> laf;
    juce::Rectangle<float> groupRect;
    juce::Rectangle<float> attackReleaseRect;
    juce::Slider::RotaryParameters standardRotaryParameters; // this should be after sliders because of the initializing order

    void createController();
    void resetToCalculatedData();
    void toolButtonClicked();
    void themeButtonClicked();
    void modeButtonClicked();
    void applyTheme(std::unique_ptr<MCLookAndFeel> newLaf);

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
    void onBallisticsSliderChanged();
    void onUserCurveEdit();
    void onStructuralConfigChanged();
    bool requestReferenceScore();
    void setBallisticsCallbacks(bool enabled);
    bool isParametersCalculated();
    void updateStateFromMatchingData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchCompressorAudioProcessorEditor)
};

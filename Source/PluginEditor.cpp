#pragma once
#include "PluginEditor.h"
#include "ParamsCalculator/CompParamsCalculatorEnv1D.h"
#include "ParamsCalculator/CompParamsCalculatorEnv2D.h"
#include "ParamsCalculator/CompParamsCalculatorNoEnv.h"
#include "Components/ComponentInitializerHelper.h"
#include "Data/Messages.h"
#include "Data/Ranges.h"
#include "Data/Colours.h"

MatchCompressorAudioProcessorEditor::MatchCompressorAudioProcessorEditor(
    MatchCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    kneesNumberComboBox(audioProcessor.apvts, kneesNumberId, juce::StringArray(std::vector<juce::String>(kneeIndexButtons.size()).data(), kneeIndexButtons.size())),
    channelAggregationTypeComboBox(audioProcessor.apvts, channelAggrerationTypeId, channelAggregationTypes),
    balFilterTypeComboBox(audioProcessor.apvts, balFilterTypeId, balFilterTypes),
    gainSlider(audioProcessor.apvts, gainId),
    thresholdSlider(audioProcessor.apvts, thresholdId + "0"),
    ratioSlider(audioProcessor.apvts, ratioId + "0"),
    kneeWidthSlider(audioProcessor.apvts, kneeWidthId + "0"),
    attackSlider(audioProcessor.apvts, attackId),
    releaseSlider(audioProcessor.apvts, releaseId),
    toolButton("matchButton"),
    groupRect(0.f, 0.f, 0.f, 0.f),
    leftPanelBackground(juce::ImageCache::getFromMemory(
        BinaryData::background_jpg,
        BinaryData::background_jpgSize)),
    rightPanelBackground(juce::ImageCache::getFromMemory(
        BinaryData::plotBackground_jpg,
        BinaryData::plotBackground_jpgSize)),
    standardRotaryParameters(gainSlider.getRotaryParameters())
{
    juce::LookAndFeel::setDefaultLookAndFeel(&laf);

    auto toolImage = juce::ImageCache::getFromMemory(
        BinaryData::tool_png, BinaryData::tool_pngSize);
    auto toolImageHover = juce::ImageCache::getFromMemory(
        BinaryData::toolhover_png, BinaryData::toolhover_pngSize);
    toggleOffImage = juce::ImageCache::getFromMemory(
        BinaryData::toggleoff_png,
        BinaryData::toggleoff_pngSize);
    toggleOnImage = juce::ImageCache::getFromMemory(
        BinaryData::toggleon_png,
        BinaryData::toggleon_pngSize);

    toolButton.setImages(true, true, true,
        toolImage, 1.0f, juce::Colours::transparentWhite,
        toolImageHover, 1.0f, juce::Colours::transparentWhite,
        toolImageHover, 1.0f, juce::Colours::transparentWhite);
    toolButton.onClick = [this] { toolButtonClicked(); };

    ComponentInitializerHelper::initTextButton(
        this, 
        resetButton, 
        resetBtnStr, 
        [this] { resetToCalculatedData(); });
    resetButton.setEnabled(false);

    const std::string btnName = "kneeIndex";
    const std::string labelText = "Knee ";
    for (int i = 0; i < kneeIndexButtons.size(); i++)
    {
        kneeIndexButtons[i] = std::make_unique<juce::ImageButton>(btnName + std::to_string(i));
        kneeIndexButtons[i]->setImages(false, true, true,
            toggleOffImage, 1.0f, juce::Colours::transparentWhite,
            toggleOffImage, 1.0f, juce::Colours::transparentWhite,
            toggleOnImage, 1.0f, juce::Colours::transparentWhite);
        kneeIndexButtons[i]->setClickingTogglesState(true);
        kneeIndexButtons[i]->setRadioGroupId(1, juce::NotificationType::dontSendNotification);
        kneeIndexButtons[i]->onClick = [this] { updateAttachments(); };
        addAndMakeVisible(*kneeIndexButtons[i]);

        kneeIndexLabels[i] = std::make_unique<juce::Label>();
        kneeIndexLabels[i]->setText(labelText + std::to_string(i + 1), juce::NotificationType::dontSendNotification);
        kneeIndexLabels[i]->setFont(juce::Font(18.0f, juce::Font::bold));
        addAndMakeVisible(*kneeIndexLabels[i]);
    }

    freeFormCurve = std::make_unique<CurvePlotComponent>(
        juce::Colours::white,
        calculatedCompCurveColour,
        actualCompCurveColour,
        thresholdVerticalLineColour);
    std::vector<float> empty;
    freeFormCurve->setData(empty);
    freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
    
    std::function<void()> updateProcessor = [&]
        {
            audioProcessor.setNeedUpdate();
        };
    std::function<void()> updateProcessorAndCurve = [&]
        {
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            audioProcessor.setNeedUpdate();
        };

    thresholdSlider.onValueChange = [&]
        {
            if (thresholdSlider.getIsParameterChanging())
                return;
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            int checkedButtonIndex = getCheckedButtonIndex();
            if (checkedButtonIndex >= 0)
                updateSlidersBounds(checkedButtonIndex, false, true);
            audioProcessor.setNeedUpdate();
        };
    kneeWidthSlider.onValueChange = [&]
        {
            if (kneeWidthSlider.getIsParameterChanging())
                return;
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            int checkedButtonIndex = getCheckedButtonIndex();
            if (checkedButtonIndex >= 0)
                updateSlidersBounds(checkedButtonIndex, true, false);
            audioProcessor.setNeedUpdate();
        };
    ratioSlider.onValueChange = [&]
        {
            if (ratioSlider.getIsParameterChanging())
                return;
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            audioProcessor.setNeedUpdate();
        };

    gainSlider.onValueChange = [&]
        {
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            audioProcessor.setNeedUpdate();
        };
    attackSlider.onValueChange = updateProcessor;
    releaseSlider.onValueChange = updateProcessor;
    balFilterTypeComboBox.onChange = updateProcessor;
    channelAggregationTypeComboBox.onChange = updateProcessor;
    kneesNumberComboBox.onChange = [&]
        {
            updateKneeIndexButtonsVisibility();
            freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumberComboBox.getSelectedId());
            int checkedButtonIndex = getCheckedButtonIndex();
            if (checkedButtonIndex >= 0)
                updateSlidersBounds(checkedButtonIndex, true, true);
            audioProcessor.setNeedUpdate();
        };
    
    int selectedId = kneesNumberComboBox.getSelectedId();
    for (int i = 1; i <= kneeIndexButtons.size(); i++)
        kneesNumberComboBox.changeItemText(i, std::to_string(i));
    kneesNumberComboBox.setSelectedId(selectedId);
    kneesNumberComboBox.repaint();

    kneesNumberComboBox.setColour(juce::ComboBox::backgroundColourId, controlBackgroundColour);
    balFilterTypeComboBox.setColour(juce::ComboBox::backgroundColourId, controlBackgroundColour);
    channelAggregationTypeComboBox.setColour(juce::ComboBox::backgroundColourId, controlBackgroundColour);

    addAndMakeVisible(thresholdSlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(ratioSlider);
    addAndMakeVisible(kneeWidthSlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(kneesNumberComboBox);
    addAndMakeVisible(balFilterTypeComboBox);
    addAndMakeVisible(channelAggregationTypeComboBox);
    addAndMakeVisible(toolButton);
    addAndMakeVisible(freeFormCurve.get());

    ComponentInitializerHelper::initLabel(this, kneesNumberLabel, &kneesNumberComboBox, false);
    ComponentInitializerHelper::initLabel(this, balFilterTypeLabel, &balFilterTypeComboBox, false);
    ComponentInitializerHelper::initLabel(this, channelAggregationTypeLabel, &channelAggregationTypeComboBox, false);

    kneeIndexButtons[0]->setToggleState(true, true);

    setSize (leftPanelWidth + rightPanelWidth, 550);

    matchWindow.reset(new MatchWindow(
        audioProcessor.getMatchingData().properties,
        audioProcessor.getMatchingData().parameterInfos,
        audioProcessor.isInputBusConnected(0),
        audioProcessor.isInputBusConnected(1)));
    createController();
}

MatchCompressorAudioProcessorEditor::~MatchCompressorAudioProcessorEditor()
{
    matchWindow->resetLookAndFeel();
    audioProcessor.PrepareToPlay = NULL;
    audioProcessor.PlayHeadStartPlaying = NULL;
    audioProcessor.PlayHeadStopPlaying = NULL;
    audioProcessor.DataCollectorMemoryFull = NULL;
}

//==============================================================================
void MatchCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto localBounds = getLocalBounds().toFloat();
    auto leftPanelBounds = localBounds;
    auto leftImageBounds = leftPanelBackground.getBounds();
    leftPanelBounds.removeFromRight(
        localBounds.getWidth() - leftImageBounds.getWidth() * localBounds.getHeight() / leftImageBounds.getHeight());
    g.drawImage(leftPanelBackground, leftPanelBounds);

    auto rightPanelBounds = localBounds.removeFromRight(rightPanelWidth);
    g.drawImage(rightPanelBackground, rightPanelBounds);
    g.setColour(controlBackgroundColour);
    g.fillRect(rightPanelBounds);

    g.setColour(panelVerticalLineColour);
    g.drawLine(rightPanelBounds.getX(), 0, rightPanelBounds.getX(), localBounds.getHeight(), 4);

    if (kneeIndexButtons[0]->isVisible())
    {
        g.setColour(groupRectColour);
        g.drawRoundedRectangle(groupRect, 10.f, 1.f);
    }
}

void MatchCompressorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Right panel

    auto rightPanelBounds = bounds.removeFromRight(rightPanelWidth);
    rightPanelBounds.removeFromTop(margin);
    rightPanelBounds.removeFromBottom(margin);
    auto paramCurveBounds = rightPanelBounds.removeFromBottom(rightPanelWidth);
    freeFormCurve->setBounds(paramCurveBounds); 

    rightPanelBounds.removeFromLeft(freeFormCurve->margin + freeFormCurve->plotMargin);
    rightPanelBounds.removeFromRight(freeFormCurve->margin + freeFormCurve->plotMargin);
    auto rightPanelControlWidth = rightPanelBounds.getWidth();

    resetButton.setBounds(rightPanelBounds.getX(), rightPanelBounds.getY(), rightPanelControlWidth, resetButtonHeight);

    rightPanelBounds.removeFromTop(resetButton.getHeight() + 4 * margin);
    auto y = rightPanelBounds.getY();
    kneesNumberComboBox.setBounds(rightPanelBounds.getX(), y, rightPanelControlWidth, comboBoxHeight);
    y += comboBoxHeight + 3 * margin;
    balFilterTypeComboBox.setBounds(rightPanelBounds.getX(), y, rightPanelControlWidth, comboBoxHeight);
    y += comboBoxHeight + 3 * margin;
    channelAggregationTypeComboBox.setBounds(rightPanelBounds.getX(), y, rightPanelControlWidth, comboBoxHeight);

    // Left panel
    
    bounds.removeFromLeft(margin);
    bounds.removeFromRight(margin);
    bounds.removeFromTop(margin);

    toolButton.setBounds(bounds.getRight() - matchButtonSize, bounds.getY(), matchButtonSize, matchButtonSize);
    bounds.removeFromTop(matchButtonSize + 2 * margin);
    groupRect.setX(bounds.getX() - margin + 1);
    groupRect.setY(bounds.getY() - margin + 1);

    auto buttonsNumber = kneeIndexButtons.size();
    auto buttonAndLabelWidth = (bounds.getWidth()) / buttonsNumber - margin;
    auto labelWidth = buttonAndLabelWidth - kneeIndexButtonSize;
    auto x = bounds.getX();
    y = bounds.getY();
    for (int i = 0; i < buttonsNumber; i++)
    {
        kneeIndexButtons[i]->setBounds(x, y, kneeIndexButtonSize, kneeIndexButtonSize);
        kneeIndexLabels[i]->setBounds(x + kneeIndexButtonSize, y, labelWidth, kneeIndexButtonSize);
        x += buttonAndLabelWidth + margin;
    }

    bounds.removeFromTop(kneeIndexButtonSize + 2 * margin);

    x = bounds.getX();
    y = bounds.getY();
    thresholdSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth;
    ratioSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth;
    kneeWidthSlider.setBounds(x, y, sliderWidth, sliderHeight);

    groupRect.setRight(bounds.getRight() + margin - 1);
    groupRect.setBottom(kneeWidthSlider.getBottom() + margin);

    bounds.removeFromTop(sliderHeight + 4 * margin);

    gainSlider.setBounds(thresholdSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
    attackSlider.setBounds(ratioSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
    releaseSlider.setBounds(kneeWidthSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
}

BaseMatchView* MatchCompressorAudioProcessorEditor::getMatchView()
{
    return matchWindow->getMatchView();
}

void MatchCompressorAudioProcessorEditor::resetToCalculatedData()
{
    kneeIndexButtons[0]->setToggleState(true, true);

    thresholdSlider.setNormalisableRange({ thresholdRange.start, thresholdRange.end, thresholdRange.interval });
    thresholdSlider.setRotaryParameters(standardRotaryParameters);
    kneeWidthSlider.setNormalisableRange({ kneeWidthRange.start, kneeWidthRange.end, kneeWidthRange.interval });
    kneeWidthSlider.setRotaryParameters(standardRotaryParameters);

    juce::NullCheckedInvocation::invoke(ResetButtonClicked);

    auto& matchingData = audioProcessor.getMatchingData();
    int kneesNumber = matchingData.properties.getProperty(setKneesNumberId);

    // double click return value it can be set 
    // only for the sliders that always occupy a full possible range
    gainSlider.setDoubleClickReturnValue(true, matchingData.calculatedCompParams[0]);
    attackSlider.setDoubleClickReturnValue(true, matchingData.properties.getProperty(setAttackId));
    releaseSlider.setDoubleClickReturnValue(true, matchingData.properties.getProperty(setReleaseId));

    updateSlidersBounds(0, true, true);

    freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumber);
    repaint();
}

// Sliders and knee index buttons manipulating

int MatchCompressorAudioProcessorEditor::getCheckedButtonIndex()
{
    int checkedIndexPlus1 = 0;
    for (int i = 0; i < kneeIndexButtons.size(); i++)
        checkedIndexPlus1 += (i + 1) * kneeIndexButtons[i]->getToggleState();
    return checkedIndexPlus1 - 1;
}

void MatchCompressorAudioProcessorEditor::updateKneeIndexButtonsVisibility()
{
    int newKneesNumber = kneesNumberComboBox.getSelectedId();
    if (getCheckedButtonIndex() >= newKneesNumber && newKneesNumber >= 1)
        kneeIndexButtons[newKneesNumber - 1]->setToggleState(true, true);
    for (int i = 0; i < kneeIndexButtons.size(); i++)
    {
        bool isVisible = newKneesNumber > std::max(i, 1);
        kneeIndexButtons[i]->setVisible(isVisible);
        kneeIndexLabels[i]->setVisible(isVisible);
    }
    repaint(); // groupRect
}

void MatchCompressorAudioProcessorEditor::updateAttachments()
{
    int checkedButtonIndex = getCheckedButtonIndex();
    if (checkedButtonIndex >= 0)
    {
        auto iStr = std::to_string(checkedButtonIndex);
        thresholdSlider.changeParameter(thresholdId + iStr);
        ratioSlider.changeParameter(ratioId + iStr);
        kneeWidthSlider.changeParameter(kneeWidthId + iStr);
        updateSlidersBounds(checkedButtonIndex, true, true);
    }
}

void MatchCompressorAudioProcessorEditor::updateSliderBounds(
    SliderWithAttachment& slider,
    const juce::NormalisableRange<float>& fullRange,
    float minValue,
    float maxValue)
{
    if (maxValue - minValue < fullRange.interval ||
        maxValue == minValue) // both conditions are needed
    {
        slider.setNormalisableRange({ fullRange.start, fullRange.end, fullRange.interval });
        slider.setRotaryParameters(standardRotaryParameters);
        slider.setEnabled(false);
    }
    else
    {
        const float standStartAngle = standardRotaryParameters.startAngleRadians;
        const float standDeltaAngle = standardRotaryParameters.endAngleRadians - standStartAngle;
        const float coeff = standDeltaAngle / (fullRange.end - fullRange.start);
        const float startAngle = standStartAngle + coeff * (minValue - fullRange.start);
        const float endAngle = standStartAngle + coeff * (maxValue - fullRange.start);
        slider.setNormalisableRange({ minValue, maxValue, fullRange.interval });
        slider.setRotaryParameters(startAngle, endAngle, standardRotaryParameters.stopAtEnd);
        slider.setEnabled(true);
    }
    slider.repaint();
}

void MatchCompressorAudioProcessorEditor::updateSlidersBounds(
    int fromIndex,
    bool updateThreshold,
    bool updateKneeWidth)
{
    auto& apvts = audioProcessor.apvts;
    const float prevIndexBound =
        fromIndex == 0 ?
        10.f * thresholdRange.start : // much less than it can really be
        *apvts.getRawParameterValue(thresholdId + std::to_string(fromIndex - 1)) + 
        *apvts.getRawParameterValue(kneeWidthId + std::to_string(fromIndex - 1)) * 0.5f;
    const float nextIndexBound =
        fromIndex == kneesNumberComboBox.getSelectedId() - 1 ?
        -10.f * thresholdRange.start : // much more than it can really be
        *apvts.getRawParameterValue(thresholdId + std::to_string(fromIndex + 1)) - 
        *apvts.getRawParameterValue(kneeWidthId + std::to_string(fromIndex + 1)) * 0.5f;
    
    float currentThreshold =
        *apvts.getRawParameterValue(thresholdId + std::to_string(fromIndex));
    float currentKneeWidth =
        *apvts.getRawParameterValue(kneeWidthId + std::to_string(fromIndex));

    if (updateThreshold)
    {
        const float minTValue = std::max(
            thresholdRange.start,
            prevIndexBound + 0.5f * currentKneeWidth);
        const float maxTValue = std::min(
            thresholdRange.end,
            nextIndexBound - 0.5f * currentKneeWidth);
        updateSliderBounds(thresholdSlider, thresholdRange, minTValue, maxTValue);
    }
    if (updateKneeWidth)
    {
        const float maxKValue = std::min(std::min(
            kneeWidthRange.end,
            2.f * (nextIndexBound - currentThreshold)),
            2.f * (currentThreshold - prevIndexBound));
        updateSliderBounds(kneeWidthSlider, kneeWidthRange, kneeWidthRange.start, maxKValue);
    }

    juce::NullCheckedInvocation::invoke(BoundsChanged);

    // update knees enabling
    int kneesNumber = *apvts.getRawParameterValue(kneesNumberId);
    for (int i = 0; i < kneeIndexButtons.size(); i++)
    {
        if (i < kneesNumber)
            kneesNumberComboBox.setItemEnabled(i + 1, true);
        else
        {
            auto prevStr = std::to_string(i - 1);
            float prevThreshold =
                *apvts.getRawParameterValue(thresholdId + prevStr);
            float prevKneeWidth =
                *apvts.getRawParameterValue(kneeWidthId + prevStr);
            float leftBound = std::min(prevThreshold + 0.5f * prevKneeWidth, 0.f);
            kneesNumberComboBox.setItemEnabled(i + 1, leftBound < 0.f);
        }
    }
}

void MatchCompressorAudioProcessorEditor::createController()
{
    mainController = std::make_unique<MainController>(
        this, 
        audioProcessor);
    
    mainController->getMatchController().CompParamsCalculated = [this]
        {
            if (audioProcessor.getMatchingData().calculatedCompParams.size() < 4 ||
                (audioProcessor.getMatchingData().calculatedCompParams.size() - 1) % 3 != 0)
                return;

            resetButton.setEnabled(true);
            freeFormCurve->setData(audioProcessor.getMatchingData().calculatedCompParams);
            resetToCalculatedData();
        };
}

void MatchCompressorAudioProcessorEditor::toolButtonClicked()
{
    juce::NullCheckedInvocation::invoke(ToolButtonClicked);
    matchWindow->toFront(true);
    matchWindow->setVisible(true);
}

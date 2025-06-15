#pragma once
#include "PluginEditor.h"
#include "ParamsCalculator/CompParamsCalculatorEnv1D.h"
#include "ParamsCalculator/CompParamsCalculatorEnv2D.h"
#include "ParamsCalculator/CompParamsCalculatorNoEnv.h"
#include "ComponentInitializerHelper.h"
#include "Messages.h"
#include "Ranges.h"
#include "Colours.h"

MatchCompressorAudioProcessorEditor::MatchCompressorAudioProcessorEditor(MatchCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    properties("properties"),
    unchangedProperties(properties.getType()),
    kneesNumberComboBox(audioProcessor.apvts, kneesNumberId, juce::StringArray(std::vector<juce::String>(kneeIndexButtons.size()).data(), kneeIndexButtons.size())),
    channelAggregationTypeComboBox(audioProcessor.apvts, channelAggrerationTypeId, channelAggregationTypes),
    balFilterTypeComboBox(audioProcessor.apvts, balFilterTypeId, balFilterTypes),
    gainSlider(audioProcessor.apvts, gainId),
    thresholdSlider(audioProcessor.apvts, thresholdId + "0"),
    ratioSlider(audioProcessor.apvts, ratioId + "0"),
    kneeWidthSlider(audioProcessor.apvts, kneeWidthId + "0"),
    attackSlider(audioProcessor.apvts, attackId),
    releaseSlider(audioProcessor.apvts, releaseId),
    matchButton("matchButton"),
    groupRect(0.f, 0.f, 0.f, 0.f),
    leftPanelBackground(juce::ImageCache::getFromMemory(
        BinaryData::background_jpg,
        BinaryData::background_jpgSize)),
    rightPanelBackground(juce::ImageCache::getFromMemory(
        BinaryData::plotBackground_jpg,
        BinaryData::plotBackground_jpgSize)),
    standardRotaryParameters(gainSlider.getRotaryParameters())
{
    fillDefaultProperties();

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

    matchButton.setImages(true, true, true,
        toolImage, 1.0f, juce::Colours::transparentWhite,
        toolImageHover, 1.0f, juce::Colours::transparentWhite,
        toolImageHover, 1.0f, juce::Colours::transparentWhite);
    matchButton.onClick = [this] { matchButtonClicked(); };

    ComponentInitializerHelper::initTextButton(this, resetButton, resetBtnStr, [this] { resetToCalculatedData(); });
    resetButton.setEnabled(false);

    auto buttonSwitcher = [safePtr = this->safePtr]()
        {
            if (auto* c = safePtr.getComponent())
                c->updateAttachments();
        };

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
        kneeIndexButtons[i]->onClick = buttonSwitcher;
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
            //resetPassiveKnees();
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
    addAndMakeVisible(matchButton);
    addAndMakeVisible(freeFormCurve.get());

    ComponentInitializerHelper::initLabel(this, kneesNumberLabel, &kneesNumberComboBox, false);
    ComponentInitializerHelper::initLabel(this, balFilterTypeLabel, &balFilterTypeComboBox, false);
    ComponentInitializerHelper::initLabel(this, channelAggregationTypeLabel, &channelAggregationTypeComboBox, false);

    kneeIndexButtons[0]->setToggleState(true, true);

    setSize (leftPanelWidth + rightPanelWidth, 550);
}

MatchCompressorAudioProcessorEditor::~MatchCompressorAudioProcessorEditor()
{
    if (matchWindow)
        matchWindow->resetLookAndFeel();
    audioProcessor.onPrepareToPlay = NULL;
    audioProcessor.onPlayHeadStartPlaying = NULL;
    audioProcessor.onPlayHeadStopPlaying = NULL;
    audioProcessor.setMemoryFullFunc(NULL);
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

    matchButton.setBounds(bounds.getRight() - matchButtonSize, bounds.getY(), matchButtonSize, matchButtonSize);
    bounds.removeFromTop(matchButtonSize + 2 * margin);
    groupRect.setX(bounds.getX() - margin);
    groupRect.setY(bounds.getY() - margin);

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
    x += sliderWidth + margin;
    ratioSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + margin;
    kneeWidthSlider.setBounds(x, y, sliderWidth, sliderHeight);

    groupRect.setRight(bounds.getRight() + margin - 1);
    groupRect.setBottom(kneeWidthSlider.getBottom() + margin);

    bounds.removeFromTop(sliderHeight + 4 * margin);

    gainSlider.setBounds(thresholdSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
    attackSlider.setBounds(ratioSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
    releaseSlider.setBounds(kneeWidthSlider.getX(), bounds.getY(), sliderWidth, sliderHeight);
}

//==============================================================================
void MatchCompressorAudioProcessorEditor::fillDefaultProperties()
{
    auto channelAggregationTypeRange = audioProcessor.apvts.getParameterRange(channelAggrerationTypeId);

    parameterInfos.clear();
    // Sharp accuracy decreasing below -60 dB when optimizing in dBs;
    // accuracy doesn't decreased when optimizing in gain units instead of dBs.
    // For hard-knee optimization gainRegionsNumber=100, quantileRegionsNumber=1000 are enough
    // and it makes the process significantly faster;
    // For soft-knee optimization values around gainRegionsNumber=500, quantileRegionsNumber=5000 are needed.

    parameterInfos.push_back(ParameterInfo(
        setGainRegionsNumberId, "Gain regions number", 4000, 1000, 5000, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setQuantileRegionsNumberId, "Quantile regions number", 400, 100, 1000, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setKneesNumberId, "Knees number", 1, 1, DynamicShaper<float>::maxKneesNumber, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setKneeTypeId, "Knee type", 1, kneeTypes));
    parameterInfos.push_back(ParameterInfo(
        setBalFilterTypeId, "Envelope type", 1, balFilterTypes));
    parameterInfos.push_back(ParameterInfo(
        setChannelAggregationTypeId, "Stereo processing", 1, channelAggregationTypes));
    parameterInfos.push_back(ParameterInfo(
        setAttackId, "Attack (ms)", 0.f, attackRange.start, attackRange.end, attackRange.interval, false, true));
    parameterInfos.push_back(ParameterInfo(
        setReleaseId, "Release (ms)", 0.f, releaseRange.start, releaseRange.end, releaseRange.interval, false, true));

    for (int i = 0; i < parameterInfos.size(); i++)
        properties.setProperty(
            parameterInfos[i].name, parameterInfos[i].defaultValue, nullptr);
}

void MatchCompressorAudioProcessorEditor::resetToCalculatedData()
{
    int kneesNumber = properties.getProperty(setKneesNumberId);
    int chAggrType = properties.getProperty(setChannelAggregationTypeId);
    int filterType = properties.getProperty(setBalFilterTypeId);
    float attackMs = properties.getProperty(setAttackId);
    float releaseMs = properties.getProperty(setReleaseId);
    kneesNumberComboBox.setSelectedId(kneesNumber, juce::NotificationType::sendNotification);
    channelAggregationTypeComboBox.setSelectedId(chAggrType, juce::NotificationType::sendNotification);
    balFilterTypeComboBox.setSelectedId(filterType, juce::NotificationType::sendNotification);
    gainSlider.setValue(calculatedCompParams[0], juce::NotificationType::sendNotification); 
    attackSlider.setValue(attackMs, juce::NotificationType::sendNotification);
    releaseSlider.setValue(releaseMs, juce::NotificationType::sendNotification);

    // double click return value it can be set 
    // only for the sliders that always occupy a full possible range
    gainSlider.setDoubleClickReturnValue(true, calculatedCompParams[0]);
    attackSlider.setDoubleClickReturnValue(true, attackMs);
    releaseSlider.setDoubleClickReturnValue(true, releaseMs);

    DynamicShaper<float>::KneesArray thresholdsDb, ratios, widths;
    for (int i = 0; i < kneesNumber; i++)
    {
        thresholdsDb[i] = calculatedCompParams[1 + i * 3];
        ratios[i] = calculatedCompParams[2 + i * 3];
        widths[i] = calculatedCompParams[3 + i * 3];
    }
    audioProcessor.setCompressorParameters(
        thresholdsDb,
        ratios,
        widths,
        calculatedCompParams[0],
        kneesNumber);
    kneeIndexButtons[0]->setToggleState(true, true);
    int checkedButtonIndex = getCheckedButtonIndex();
    float ratioSliderValue =
        calculatedCompParams[2] >= 1. ?
        calculatedCompParams[2] :
        2.f - 1.f / calculatedCompParams[2];

    thresholdSlider.setNormalisableRange({ thresholdRange.start, thresholdRange.end, thresholdRange.interval });
    thresholdSlider.setRotaryParameters(standardRotaryParameters);
    thresholdSlider.setValue(calculatedCompParams[1], juce::NotificationType::sendNotification);
    ratioSlider.setValue(ratioSliderValue, juce::NotificationType::sendNotification);
    kneeWidthSlider.setNormalisableRange({ kneeWidthRange.start, kneeWidthRange.end, kneeWidthRange.interval });
    kneeWidthSlider.setRotaryParameters(standardRotaryParameters);
    kneeWidthSlider.setValue(calculatedCompParams[3], juce::NotificationType::sendNotification);

    for (int i = 1; i < DynamicShaper<float>::maxKneesNumber; i++)
    {
        float t, r, kw;
        if (i < kneesNumber)
        {
            t = calculatedCompParams[i * 3 + 1];
            r = calculatedCompParams[i * 3 + 2];
            if (r < 1)
                r = 2.f - 1.f / r;
            kw = calculatedCompParams[i * 3 + 3];
        }
        else
        {
            t = kw = 0.f;
            r = ratioSliderValue;
        }

        auto* par = audioProcessor.apvts.getParameter(thresholdId + std::to_string(i));
        par->beginChangeGesture();
        par->setValueNotifyingHost(thresholdRange.convertTo0to1(t));
        par->endChangeGesture();

        par = audioProcessor.apvts.getParameter(ratioId + std::to_string(i));
        par->beginChangeGesture();
        par->setValueNotifyingHost(ratioRange.convertTo0to1(r));
        par->endChangeGesture();

        par = audioProcessor.apvts.getParameter(kneeWidthId + std::to_string(i));
        par->beginChangeGesture();
        par->setValueNotifyingHost(kneeWidthRange.convertTo0to1(kw));
        par->endChangeGesture();
    }

    freeFormCurve->updateActualParameters(audioProcessor.apvts, kneesNumber);

    audioProcessor.setNeedUpdate();
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
        slider.setRotaryParameters(standardRotaryParameters.startAngleRadians, standardRotaryParameters.endAngleRadians, standardRotaryParameters.stopAtEnd);
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
    int index,
    bool updateThreshold,
    bool updateKneeWidth)
{
    const float prevIndexBound =
        index == 0 ?
        10.f * thresholdRange.start : // much less than it can really be
        *audioProcessor.apvts.getRawParameterValue(thresholdId + std::to_string(index - 1)) + 0.5f *
        *audioProcessor.apvts.getRawParameterValue(kneeWidthId + std::to_string(index - 1));
    const float nextIndexBound =
        index == kneesNumberComboBox.getSelectedId() - 1 ?
        -10.f * thresholdRange.start : // much more than it can really be
        *audioProcessor.apvts.getRawParameterValue(thresholdId + std::to_string(index + 1)) - 0.5f *
        *audioProcessor.apvts.getRawParameterValue(kneeWidthId + std::to_string(index + 1));
    
    float currentThreshold =
        *audioProcessor.apvts.getRawParameterValue(thresholdId + std::to_string(index));
    float currentKneeWidth =
        *audioProcessor.apvts.getRawParameterValue(kneeWidthId + std::to_string(index));

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

    // update next knees enabling
    if (index == kneesNumberComboBox.getSelectedId() - 1 &&
        index < kneeIndexButtons.size() - 1)
    {
        float rightBound = std::min(currentThreshold + 0.5f * currentKneeWidth, 0.f);
        for (int i = index + 1; i < kneeIndexButtons.size(); i++)
        {
            kneesNumberComboBox.setItemEnabled(i + 1, rightBound < 0.f);
            currentThreshold = *audioProcessor.apvts.getRawParameterValue(thresholdId + std::to_string(i));
            if (currentThreshold <= rightBound)
            {
                auto* par = audioProcessor.apvts.getParameter(thresholdId + std::to_string(i));
                par->beginChangeGesture();
                par->setValueNotifyingHost(thresholdRange.convertTo0to1(rightBound));
                par->endChangeGesture();

                par = audioProcessor.apvts.getParameter(kneeWidthId + std::to_string(i));
                par->beginChangeGesture();
                par->setValueNotifyingHost(kneeWidthRange.convertTo0to1(0.f));
                par->endChangeGesture();
            }
            else
            {
                currentKneeWidth = *audioProcessor.apvts.getRawParameterValue(kneeWidthId + std::to_string(i));
                float delta = currentThreshold - rightBound;
                if (i < kneeIndexButtons.size() - 1)
                    delta = std::min(delta, -currentThreshold);
                if (0.5f * currentKneeWidth >= delta)
                {
                    auto* par = audioProcessor.apvts.getParameter(kneeWidthId + std::to_string(i));
                    par->beginChangeGesture();
                    par->setValueNotifyingHost(kneeWidthRange.convertTo0to1(2.f * delta));
                    par->endChangeGesture();
                    rightBound = std::min(2.f * currentThreshold - rightBound, 0.f);
                }
            }
        }
    }
}

void MatchCompressorAudioProcessorEditor::matchButtonClicked()
{
    if (!matchWindow)
    {
        std::function<void(bool, bool)> toogleFunc = 
            [&](bool mainBus, bool sidechain) 
            { 
                audioProcessor.setDataCollectionBuses(mainBus, sidechain); 
            };
        matchWindow.reset(new MatchWindow(
            properties, 
            parameterInfos, 
            audioProcessor.isInputBusConnected(0),
            audioProcessor.isInputBusConnected(1),
            toogleFunc));
        matchWindow->addComponentListener(this);
        audioProcessor.onPrepareToPlay = [&]
            {
                matchWindow.get()->setBusesConnected(
                    audioProcessor.isInputBusConnected(0),
                    audioProcessor.isInputBusConnected(1));
            };

    }
    unchangedProperties.copyPropertiesFrom(properties, nullptr);
    matchWindow->toFront(true);
    matchWindow->setVisible(true);
}

void MatchCompressorAudioProcessorEditor::componentVisibilityChanged(Component& component)
{
    if (&component == matchWindow.get())
    {
        if (component.isVisible())
        {
            matchWindow.get()->startTimer();
            if (audioProcessor.isPlayHeadPlaying())
                matchWindow.get()->setToggleButtonsDisabled();
            audioProcessor.onPlayHeadStartPlaying = [&]
                {
                    matchWindow.get()->setToggleButtonsDisabled();
                    matchWindow.get()->setMustBeInFront(true);
                };
            audioProcessor.onPlayHeadStopPlaying = [&]
                {
                    endCollectingData(true, true);
                    matchWindow->setMustBeInFront(true);
                };
            audioProcessor.setMemoryFullFunc([&]
                {
                    endCollectingData(true, false);
                    matchWindow->setMustBeInFront(true);
                });
            matchWindow.get()->setOnTimer([&]
                {
                    if (matchWindow.get()->getMustBeInFront())
                    {
                        matchWindow.get()->toFront(false);
                        matchWindow.get()->setMustBeInFront(false);
                    }
                });
        }
        else
        {
            matchWindow.get()->stopTimer();
            endCollectingData(false, true);
            matchWindow.get()->timerCallback();
            matchWindow.get()->setOnTimer(NULL);
            audioProcessor.onPlayHeadStartPlaying = NULL;
            audioProcessor.onPlayHeadStopPlaying = NULL;
            audioProcessor.setMemoryFullFunc(NULL);
            
            auto& compParams = matchWindow->getResult();
            if (compParams.size() < 4 || (compParams.size() - 1) % 3 != 0)
                return;

            resetButton.setEnabled(true);
            calculatedCompParams = compParams;
            freeFormCurve->setData(calculatedCompParams);
            resetToCalculatedData();
        }
    }
}

void MatchCompressorAudioProcessorEditor::endCollectingData(bool saveData, bool resetButtonsState)
{
    auto* w = matchWindow.get();
    std::vector<std::vector<float>> mainBusData, sideChainData;
    double mainBusRate, sidechainRate;
    audioProcessor.getCollectedData(mainBusData, mainBusRate, sideChainData, sidechainRate);
    if (saveData)
        w->setFromDataCollector(sideChainData, sidechainRate, mainBusData, mainBusRate);
    if (resetButtonsState)
    {
        w->setBusesConnected(
            audioProcessor.isInputBusConnected(0),
            audioProcessor.isInputBusConnected(1));
        w->setToggleButtonsUnchecked();
    }
}


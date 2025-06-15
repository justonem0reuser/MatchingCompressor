#include "MatchWindow.h"
#include "Messages.h"
#include "Colours.h"

MatchWindow::MatchWindow(
    juce::ValueTree& properties,
    std::vector<ParameterInfo>& parameterInfos,
    bool isMainBusConnected, 
    bool isSidechainConnected,
    std::function<void(bool, bool)> toggleFunc) :
    DocumentWindow(
        matchWindowTitleStr,
        documentWindowBackgroundColour,
        juce::DocumentWindow::TitleBarButtons::closeButton,
        true),
    component(
        this, 
        properties, 
        parameterInfos, 
        isMainBusConnected, 
        isSidechainConnected, 
        toggleFunc),
    mustBeInFront(false)
{
    auto w = component.getWidth(),
        h = component.getHeight();
    setContentComponent(&component);
    setUsingNativeTitleBar(true);
    setSize(w, h);
    centreWithSize(w, h);
    setResizable(true, false);
}

std::vector<float>& MatchWindow::getResult()
{
    return component.getResult();
}

bool MatchWindow::getMustBeInFront()
{
    return mustBeInFront;
}

void MatchWindow::setMustBeInFront(bool mustBeInFront)
{
    this->mustBeInFront = mustBeInFront;
}

void MatchWindow::resetLookAndFeel()
{
    component.resetLookAndFeel();
}

void MatchWindow::setFromDataCollector(
    std::vector<std::vector<float>>& refSamples,
    double refSampleRate,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate)
{
    component.setFromDataCollector(refSamples, refSampleRate, destSamples, destSampleRate);
}

void MatchWindow::setBusesConnected(bool mainBus, bool sidechain)
{
    component.setBusesConnected(mainBus, sidechain);
}

void MatchWindow::setToggleButtonsDisabled()
{
    component.setToggleButtonsDisabled();
}

void MatchWindow::setToggleButtonsUnchecked()
{
    component.setToggleButtonsUnchecked();
}

void MatchWindow::setOnTimer(std::function<void()> func)
{
    component.setOnTimer(func);
}

void MatchWindow::startTimer()
{
    component.startTimer();
}

void MatchWindow::stopTimer()
{
    component.stopTimer();
}

void MatchWindow::timerCallback()
{
    component.timerCallback();
}

void MatchWindow::closeButtonPressed()
{
    component.cancelButtonPressed();
}

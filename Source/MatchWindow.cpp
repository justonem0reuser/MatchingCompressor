#include "MatchWindow.h"
#include "Data/Messages.h"
#include "Data/Colours.h"

MatchWindow::MatchWindow(
    juce::ValueTree& properties,
    std::vector<ParameterInfo>& parameterInfos,
    bool isMainBusConnected, 
    bool isSidechainConnected):
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
        isSidechainConnected)
{
    auto w = component.getWidth(),
        h = component.getHeight();
    setContentComponent(&component);
    setUsingNativeTitleBar(true);
    setSize(w, h);
    centreWithSize(w, h);
    setResizable(true, false);
}

BaseMatchView* MatchWindow::getMatchView()
{
    return &component;
}

void MatchWindow::resetLookAndFeel()
{
    component.resetLookAndFeel();
}

void MatchWindow::closeButtonPressed()
{
    juce::NullCheckedInvocation::invoke(component.onCancelButtonClicked);
}

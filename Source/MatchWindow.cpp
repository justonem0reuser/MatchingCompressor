#include "MatchWindow.h"
#include "Data/Messages.h"
#include "LookAndFeel/MCLookAndFeel.h"

MatchWindow::MatchWindow(
    juce::ValueTree& properties,
    std::vector<ParameterInfo>& parameterInfos,
    bool isMainBusConnected, 
    bool isSidechainConnected):
    DocumentWindow(
        matchWindowTitleStr,
        juce::LookAndFeel::getDefaultLookAndFeel().findColour(
            MCLookAndFeel::documentWindowBackgroundColourId),
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

bool MatchWindow::keyPressed(const juce::KeyPress& key)
{
    if (!key.isKeyCode(juce::KeyPress::tabKey) || component.hasKeyboardFocus(true))
        return DocumentWindow::keyPressed(key);

    auto components = juce::KeyboardFocusTraverser().getAllComponents(&component);
    if (components.empty())
        return false;

    auto* focusComponent = key.getModifiers().isShiftDown() ? components.back() : components.front();
    focusComponent->grabKeyboardFocus();
    return true;
}

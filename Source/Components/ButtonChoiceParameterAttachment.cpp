#include "ButtonChoiceParameterAttachment.h"

ButtonChoiceParameterAttachment::ButtonChoiceParameterAttachment(
    juce::RangedAudioParameter& param,
    ButtonChoiceComponent& bc,
    juce::UndoManager* um)
    : buttonChoice(bc),
      storedParameter(param),
      attachment(param, [this](float f) { setValue(f); }, um)
{
    sendInitialUpdate();
    buttonChoice.addListener(this);
}

ButtonChoiceParameterAttachment::~ButtonChoiceParameterAttachment()
{
    buttonChoice.removeListener(this);
}

void ButtonChoiceParameterAttachment::sendInitialUpdate()
{
    attachment.sendInitialUpdate();
}

void ButtonChoiceParameterAttachment::setValue(float newValue)
{
    const auto normValue = storedParameter.convertTo0to1(newValue);
    const auto index = juce::roundToInt(normValue * (float)(buttonChoice.getNumItems() - 1));

    if (index == buttonChoice.getSelectedItemIndex())
        return;

    const juce::ScopedValueSetter<bool> svs(ignoreCallbacks, true);
    buttonChoice.setSelectedItemIndex(index, juce::sendNotificationSync);
}

void ButtonChoiceParameterAttachment::buttonChoiceChanged(ButtonChoiceComponent*)
{
    if (ignoreCallbacks)
        return;

    const auto numItems = buttonChoice.getNumItems();
    const auto selected = (float)buttonChoice.getSelectedItemIndex();
    const auto newValue = numItems > 1 ? selected / (float)(numItems - 1) : 0.0f;

    attachment.setValueAsCompleteGesture(storedParameter.convertFrom0to1(newValue));
}

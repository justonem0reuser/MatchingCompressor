#pragma once
#include <JuceHeader.h>
#include "ButtonChoiceComponent.h"

// Analogue of juce::ComboBoxParameterAttachment
class ButtonChoiceParameterAttachment : private ButtonChoiceComponent::Listener
{
public:
    ButtonChoiceParameterAttachment(juce::RangedAudioParameter& parameter,
                                    ButtonChoiceComponent& buttonChoice,
                                    juce::UndoManager* undoManager = nullptr);

    ~ButtonChoiceParameterAttachment() override;

    // Call this after setting up ButtonChoiceComponent
    // in the case if extra setup is needed after constructing this attachment    
    void sendInitialUpdate();

private:
    void setValue(float newValue);
    void buttonChoiceChanged(ButtonChoiceComponent*) override;

    ButtonChoiceComponent& buttonChoice;
    juce::RangedAudioParameter& storedParameter;
    juce::ParameterAttachment attachment;
    bool ignoreCallbacks = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceParameterAttachment)
};

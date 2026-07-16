#include "ButtonChoicePropertyComponent.h"

namespace
{
    // Analogue of private juce::ChoiceRemapperValueSource
    class ButtonChoiceRemapperValueSource final : public juce::Value::ValueSource,
                                                  private juce::Value::Listener
    {
    public:
        ButtonChoiceRemapperValueSource(const juce::Value& source, const juce::Array<juce::var>& map)
            : sourceValue(source),
              mappings(map)
        {
            sourceValue.addListener(this);
        }

        juce::var getValue() const override
        {
            auto targetValue = sourceValue.getValue();

            for (auto& map : mappings)
                if (map.equalsWithSameType(targetValue))
                    return mappings.indexOf(map) + 1;

            return mappings.indexOf(targetValue) + 1;
        }

        void setValue(const juce::var& newValue) override
        {
            auto remappedVal = mappings[static_cast<int>(newValue) - 1];

            if (! remappedVal.equalsWithSameType(sourceValue))
                sourceValue = remappedVal;
        }

    protected:
        juce::Value sourceValue;
        juce::Array<juce::var> mappings;

        void valueChanged(juce::Value&) override { sendChangeMessage(true); }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceRemapperValueSource)
    };
}

ButtonChoicePropertyComponent::ButtonChoicePropertyComponent(
    const juce::Value& valueToControl,
    const juce::String& propertyName,
    const juce::StringArray& choices,
    const juce::Array<juce::var>& correspondingValues)
    : juce::PropertyComponent(propertyName),
      buttonChoice(choices)
{
    jassert(correspondingValues.size() == choices.size());

    juce::Value remapper(new ButtonChoiceRemapperValueSource(valueToControl, correspondingValues));
    buttonChoice.setSelectedId((int) remapper.getValue(), juce::dontSendNotification);
    buttonChoice.getSelectedIdAsValue().referTo(remapper);

    addAndMakeVisible(buttonChoice);
}

void ButtonChoicePropertyComponent::refresh()
{
}

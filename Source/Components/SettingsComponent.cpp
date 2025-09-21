#include "SettingsComponent.h"

SettingsComponent::SettingsComponent(
    Component* parent,
    juce::ValueTree& properties,
    std::vector<ParameterInfo>& parameterInfos) :
    parent(parent),
    properties(properties)
{
    propComps = createPropertyComponents(parameterInfos);
    panel.addProperties(propComps, margin);

    int currentY = margin;
    addAndMakeVisible(&panel);
    panel.setBounds(margin, currentY, width - 2 * margin, panel.getTotalContentHeight());
    currentY += panel.getBottom() + margin;

    addAndMakeVisible(&okButton);
    okButton.setButtonText("OK");
    okButton.setBounds(margin, currentY, buttonWidth, componentHeight);

    addAndMakeVisible(&cancelButton);
    cancelButton.setButtonText("Cancel");
    cancelButton.setBounds(width - margin - buttonWidth, currentY, buttonWidth, componentHeight);
    currentY += componentHeight + margin;

    setSize(width, currentY);
}

SettingsComponent::~SettingsComponent()
{
    resetLookAndFeel();
}

void SettingsComponent::resetLookAndFeel()
{
    for (auto* comp : propComps)
        for (int j = 0; j < comp->getNumChildComponents(); j++)
            comp->getChildComponent(j)->setLookAndFeel(nullptr);
}

juce::Array<juce::PropertyComponent*> SettingsComponent::createPropertyComponents(
    std::vector<ParameterInfo>& parameterInfos)
{
    juce::Array<juce::PropertyComponent*> components;
    for (int i = 0; i < parameterInfos.size(); i++)
    {
        auto& info = parameterInfos[i];
        auto prop = properties.getPropertyAsValue(info.name, nullptr);
        if (info.isBoolean)
            components.add(new juce::BooleanPropertyComponent(
                prop, info.text, ""));
        else if (info.isTextChoice)
        {
            juce::Array<juce::var> values;
            for (int j = 1; j <= info.choices.size(); j++)
                values.add(j);
            auto* comp = new juce::ChoicePropertyComponent(
                prop, info.text, info.choices, values);
            for (int j = 0; j < comp->getNumChildComponents(); j++)
                if (auto comboBox = dynamic_cast<juce::ComboBox*>(comp->getChildComponent(j)))
                    comboBox->setLookAndFeel(&getLookAndFeel());
            components.add(comp);
        }
        else
        {
            auto* comp = new juce::SliderPropertyComponent(
                prop, info.text, info.minValue, info.maxValue, info.step);
            for (int j = 0; j < comp->getNumChildComponents(); j++)
                if (auto slider = dynamic_cast<juce::Slider*>(comp->getChildComponent(j)))
                    slider->setLookAndFeel(&getLookAndFeel());
            components.add(comp);
        }
    }
    return components;
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// User presets shared by the AtelierDSP plugins: stored as XML in
//   ~/Documents/AtelierDSP/<PluginName>/<preset>.xml
namespace atelier
{
inline juce::File presetFolder (const juce::String& plugin)
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("AtelierDSP").getChildFile (plugin);
    dir.createDirectory();
    return dir;
}

inline juce::StringArray listPresets (const juce::String& plugin)
{
    juce::StringArray names;
    for (auto& f : presetFolder (plugin).findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

inline bool savePreset (juce::AudioProcessorValueTreeState& apvts, const juce::String& plugin, const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;
    auto xml = apvts.copyState().createXml();
    return xml != nullptr && xml->writeTo (presetFolder (plugin).getChildFile (name + ".xml"));
}

inline bool loadPreset (juce::AudioProcessorValueTreeState& apvts, const juce::String& plugin, const juce::String& name)
{
    auto file = presetFolder (plugin).getChildFile (name + ".xml");
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            return true;
        }
    return false;
}

// Small bar with a user-preset menu and a Save button.
class PresetBar : public juce::Component
{
public:
    PresetBar (juce::AudioProcessorValueTreeState& s, juce::String pluginName)
        : apvts (s), plugin (std::move (pluginName))
    {
        addAndMakeVisible (box);
        addAndMakeVisible (saveButton);
        box.setTextWhenNothingSelected ("User presets");
        box.onChange = [this]
        {
            const auto name = box.getText();
            if (name.isNotEmpty())
                loadPreset (apvts, plugin, name);
        };
        saveButton.onClick = [this] { askAndSave(); };
        refresh();
    }

    void refresh()
    {
        box.clear (juce::dontSendNotification);
        int id = 1;
        for (auto& n : listPresets (plugin))
            box.addItem (n, id++);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        saveButton.setBounds (r.removeFromRight (60));
        r.removeFromRight (6);
        box.setBounds (r);
    }

private:
    void askAndSave()
    {
        auto* aw = new juce::AlertWindow ("Save preset", "Name of the preset:", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", "");
        aw->addButton ("Save", 1);
        aw->addButton ("Cancel", 0);
        aw->enterModalState (true,
                             juce::ModalCallbackFunction::create ([this, aw] (int result)
                             {
                                 if (result == 1)
                                 {
                                     if (savePreset (apvts, plugin, aw->getTextEditorContents ("name")))
                                         refresh();
                                 }
                             }),
                             true);
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::String plugin;
    juce::ComboBox box;
    juce::TextButton saveButton { "Save" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
} // namespace atelier

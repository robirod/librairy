#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "metadata/MetadataModel.h"

namespace librairy
{

/**
 * Panel for viewing and editing metadata of the currently selected audio file.
 * Displays standard fields (BWF, iXML, ID3) and allows inline editing.
 */
class MetadataPanel : public juce::Component,
                      public juce::Timer
{
public:
    MetadataPanel();
    ~MetadataPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    /** Display metadata for the given file */
    void displayMetadata(const AudioFileInfo& info);

    /** Populate the given AudioFileInfo with current values from text fields */
    void populateUpdatedInfo(AudioFileInfo& info) const;

    /** Trigger a visual confirmation that metadata was saved successfully */
    void showSaveConfirmation();

    /** Clear displayed metadata */
    void clear();

    /** Listener for metadata edit events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void metadataChanged(const juce::String& field,
                                     const juce::String& newValue) = 0;
        virtual void metadataSaveRequested() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    juce::Label         titleLabel    { {}, "Metadata" };
    juce::TextButton    saveButton    { "Save" };
    juce::Label         savedNotificationLabel;

    // Editable fields
    struct MetadataField
    {
        juce::Label      label;
        juce::TextEditor editor;
        juce::String     fieldName;
    };

    std::vector<std::unique_ptr<MetadataField>> fields;

    juce::Viewport viewport;
    juce::Component fieldsContainer;

    juce::ListenerList<Listener> listeners;

    void createField(const juce::String& name,
                     const juce::String& displayName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataPanel)
};

} // namespace librairy


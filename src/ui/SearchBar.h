#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace librairy
{

/**
 * Top toolbar containing:
 * - "+ Add Folder" button
 * - Search bar with clear button
 * - Audio output device selector & Audio Setup dialog button
 * - Live scanning progress banner
 */
class SearchBar : public juce::Component
{
public:
    SearchBar();
    ~SearchBar() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Listener for toolbar events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void searchTextChanged(const juce::String& query) = 0;
        virtual void searchSubmitted(const juce::String& query) = 0;
        virtual void addFolderRequested() = 0;
        virtual void outputDeviceChanged(const juce::String& deviceName) = 0;
        virtual void audioSetupRequested() = 0;
        virtual void scanCancelRequested() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    juce::String getSearchText() const;
    void clear();

    /** Populate available audio output devices in the dropdown */
    void setAudioOutputDevices(const juce::StringArray& devices,
                               const juce::String& currentDevice);

    /** Update the visible scanning progress banner */
    void setScanningState(bool isScanning, double progress, const juce::String& statusText);

private:
    // Left: Import
    juce::TextButton addFolderButton { "+ Add Folder" };

    // Center: Search
    juce::TextEditor searchInput;
    juce::TextButton clearButton { "x" };

    // Right: Audio output controls
    juce::Label      outputLabel { {}, "Output:" };
    juce::ComboBox   outputDeviceBox;
    juce::TextButton audioSetupButton { "Audio Setup..." };

    // Bottom strip: Scanning progress banner (visible during indexing)
    bool             scanningActive = false;
    double           scanProgressValue = 0.0;
    juce::ProgressBar scanProgressBar { scanProgressValue };
    juce::Label      scanStatusLabel;
    juce::TextButton scanCancelButton { "Cancel" };

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchBar)
};

} // namespace librairy

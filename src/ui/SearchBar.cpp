#include "SearchBar.h"

namespace librairy
{

SearchBar::SearchBar()
{
    // 1. Add Folder Button
    addFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E66B2));
    addFolderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addFolderButton.onClick = [this]()
    {
        listeners.call([](Listener& l) { l.addFolderRequested(); });
    };
    addAndMakeVisible(addFolderButton);

    // 2. Search input
    searchInput.setMultiLine(false);
    searchInput.setTextToShowWhenEmpty("Search library...",
                                       juce::Colours::grey);
    searchInput.setColour(juce::TextEditor::backgroundColourId,
                          juce::Colour(0xFF252525));
    searchInput.setColour(juce::TextEditor::textColourId,
                          juce::Colours::white);
    searchInput.setColour(juce::TextEditor::outlineColourId,
                          juce::Colour(0xFF3D3D3D));
    searchInput.onTextChange = [this]()
    {
        listeners.call([this](Listener& l)
        {
            l.searchTextChanged(searchInput.getText());
        });
    };
    searchInput.onReturnKey = [this]()
    {
        listeners.call([this](Listener& l)
        {
            l.searchSubmitted(searchInput.getText());
        });
    };
    addAndMakeVisible(searchInput);

    // Clear button
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2A));
    clearButton.onClick = [this]() { clear(); };
    addAndMakeVisible(clearButton);

    // 3. Audio output device controls
    outputLabel.setFont(juce::FontOptions(12.0f));
    outputLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(outputLabel);

    outputDeviceBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF252525));
    outputDeviceBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF3D3D3D));
    outputDeviceBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    outputDeviceBox.onChange = [this]()
    {
        auto selected = outputDeviceBox.getText();
        if (selected.isNotEmpty())
        {
            listeners.call([selected](Listener& l)
            {
                l.outputDeviceChanged(selected);
            });
        }
    };
    addAndMakeVisible(outputDeviceBox);

    audioSetupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    audioSetupButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    audioSetupButton.onClick = [this]()
    {
        listeners.call([](Listener& l) { l.audioSetupRequested(); });
    };
    addAndMakeVisible(audioSetupButton);

    // 4. Scanning Progress Banner
    scanProgressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xFF4CAF50));
    scanProgressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xFF1E1E1E));
    addChildComponent(scanProgressBar);

    scanStatusLabel.setFont(juce::FontOptions(11.0f));
    scanStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF4CAF50));
    addChildComponent(scanStatusLabel);

    scanCancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF442222));
    scanCancelButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFF8888));
    scanCancelButton.onClick = [this]()
    {
        listeners.call([](Listener& l) { l.scanCancelRequested(); });
    };
    addChildComponent(scanCancelButton);
}

void SearchBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Bottom dividing line
    g.setColour(juce::Colour(0xFF333333));
    g.drawLine(0.f, static_cast<float>(getHeight() - 1),
               static_cast<float>(getWidth()), static_cast<float>(getHeight() - 1));

    if (scanningActive)
    {
        // Separator between toolbar and progress strip
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.drawLine(0.f, 44.f, static_cast<float>(getWidth()), 44.f);
    }
}

void SearchBar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 6);

    // Top row
    auto topRow = bounds.removeFromTop(32);

    addFolderButton.setBounds(topRow.removeFromLeft(105));
    topRow.removeFromLeft(8);

    // Right side: Audio controls
    audioSetupButton.setBounds(topRow.removeFromRight(100));
    topRow.removeFromRight(6);
    outputDeviceBox.setBounds(topRow.removeFromRight(160));
    outputLabel.setBounds(topRow.removeFromRight(50));
    topRow.removeFromRight(12);

    // Center: Search input + clear button
    clearButton.setBounds(topRow.removeFromRight(28));
    topRow.removeFromRight(4);
    searchInput.setBounds(topRow);

    // Bottom strip (if scanning)
    if (scanningActive)
    {
        bounds.removeFromTop(6); // padding
        auto progressRow = bounds.removeFromTop(22);
        scanCancelButton.setBounds(progressRow.removeFromRight(65));
        progressRow.removeFromRight(8);
        scanProgressBar.setBounds(progressRow.removeFromRight(180));
        progressRow.removeFromRight(8);
        scanStatusLabel.setBounds(progressRow);
    }
}

void SearchBar::setAudioOutputDevices(const juce::StringArray& devices,
                                     const juce::String& currentDevice)
{
    outputDeviceBox.clear(juce::dontSendNotification);

    for (int i = 0; i < devices.size(); ++i)
    {
        outputDeviceBox.addItem(devices[i], i + 1);
        if (devices[i] == currentDevice)
        {
            outputDeviceBox.setSelectedId(i + 1, juce::dontSendNotification);
        }
    }

    if (outputDeviceBox.getSelectedId() == 0 && devices.size() > 0)
    {
        outputDeviceBox.setSelectedId(1, juce::dontSendNotification);
    }
}

void SearchBar::setScanningState(bool isScanning, double progress, const juce::String& statusText)
{
    scanningActive = isScanning;
    scanProgressValue = progress;

    scanProgressBar.setVisible(isScanning);
    scanStatusLabel.setVisible(isScanning);
    scanCancelButton.setVisible(isScanning);

    if (isScanning)
    {
        scanStatusLabel.setText(statusText, juce::dontSendNotification);
    }

    if (auto* parent = getParentComponent())
    {
        parent->resized();
    }

    repaint();
}

void SearchBar::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SearchBar::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

juce::String SearchBar::getSearchText() const
{
    return searchInput.getText();
}

void SearchBar::clear()
{
    searchInput.clear();
    listeners.call([](Listener& l)
    {
        l.searchTextChanged("");
    });
}

} // namespace librairy

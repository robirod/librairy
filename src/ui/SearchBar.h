#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace librairy
{

/**
 * Search bar with full-text input and filter controls.
 * Sits at the top of the main window.
 */
class SearchBar : public juce::Component
{
public:
    SearchBar();
    ~SearchBar() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Listener for search events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void searchTextChanged(const juce::String& query) = 0;
        virtual void searchSubmitted(const juce::String& query) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    juce::String getSearchText() const;
    void clear();

private:
    juce::TextEditor searchInput;
    juce::TextButton clearButton { "✕" };
    juce::TextButton filterButton { "Filters" };

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchBar)
};

} // namespace librairy


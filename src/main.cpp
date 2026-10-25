/*
    Librairy — Professional Audio Library Manager
    Entry point
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/MainWindow.h"

//==============================================================================
class LibrairyApplication : public juce::JUCEApplication
{
public:
    LibrairyApplication() = default;

    const juce::String getApplicationName() override
    {
        return JUCE_APPLICATION_NAME_STRING;
    }

    const juce::String getApplicationVersion() override
    {
        return JUCE_APPLICATION_VERSION_STRING;
    }

    bool moreThanOneInstanceAllowed() override { return false; }

    //==========================================================================
    void initialise(const juce::String& /*commandLine*/) override
    {
        mainWindow = std::make_unique<librairy::MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    //==========================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
        // When another instance is launched, bring this one to front
        if (mainWindow != nullptr)
            mainWindow->toFront(true);
    }

private:
    std::unique_ptr<librairy::MainWindow> mainWindow;
};

//==============================================================================
// Macro that creates the application entry point
START_JUCE_APPLICATION(LibrairyApplication)


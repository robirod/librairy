#include "MainWindow.h"

namespace librairy
{

//==============================================================================
// MainWindow
//==============================================================================

static juce::File getWindowStateFile()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Librairy");
    dir.createDirectory();
    return dir.getChildFile("window.state");
}

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Colour(0xFF141414),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);

    contentComponent = std::make_unique<MainContent>();
    setContentOwned(contentComponent.release(), true);

    setResizable(true, true);
    setResizeLimits(800, 500, 3840, 2160);
    centreWithSize(1320, 840);
    setVisible(true);

    auto stateFile = getWindowStateFile();
    if (stateFile.existsAsFile())
    {
        restoreWindowStateFromString(stateFile.loadFileAsString());
    }
}

void MainWindow::closeButtonPressed()
{
    auto stateFile = getWindowStateFile();
    stateFile.replaceWithText(getWindowStateAsString());

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

//==============================================================================
// MainContent
//==============================================================================

juce::File MainWindow::MainContent::getDefaultDatabaseFile()
{
    auto appData = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory).getChildFile("Librairy");
    appData.createDirectory();
    return appData.getChildFile("librairy.db");
}

MainWindow::MainContent::MainContent()
{
    // 1. Initialize Database & Core
    auto dbFile = getDefaultDatabaseFile();
    db = std::make_unique<Database>(dbFile.getFullPathName());
    searchEngine = std::make_unique<SearchEngine>(*db);
    scanner = std::make_unique<Scanner>(*db);

    // 2. Add and display UI components
    addAndMakeVisible(searchBar);
    addAndMakeVisible(sidebar);
    addAndMakeVisible(resultsTable);
    addAndMakeVisible(waveformView);
    addAndMakeVisible(metadataPanel);

    // 3. Register listeners
    sidebar.addListener(this);
    searchBar.addListener(this);
    resultsTable.addListener(this);
    waveformView.addListener(this);
    metadataPanel.addListener(this);
    scanner->addListener(this);

    // 4. Initial populate
    refreshLibraries();
    performSearch();

    // 5. Start playback cursor tracking timer (30 Hz)
    startTimerHz(30);

    setWantsKeyboardFocus(true);
}

MainWindow::MainContent::~MainContent()
{
    stopTimer();
    scanner->cancelScan();
    scanner->removeListener(this);
    sidebar.removeListener(this);
    searchBar.removeListener(this);
    resultsTable.removeListener(this);
    waveformView.removeListener(this);
    metadataPanel.removeListener(this);
}

void MainWindow::MainContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
}

void MainWindow::MainContent::resized()
{
    auto bounds = getLocalBounds();

    // Top: Search bar (46px)
    searchBar.setBounds(bounds.removeFromTop(46));

    // Left: Sidebar (width 220px)
    auto sidebarWidth = juce::jlimit(180, 260, bounds.getWidth() / 5);
    sidebar.setBounds(bounds.removeFromLeft(sidebarWidth));

    // Right Area: Results Table (50%), Waveform (25%), Metadata (25%)
    auto rightArea = bounds;
    auto totalHeight = rightArea.getHeight();

    int resultsHeight  = static_cast<int>(totalHeight * 0.52);
    int waveformHeight = static_cast<int>(totalHeight * 0.23);

    resultsTable.setBounds(rightArea.removeFromTop(resultsHeight));
    waveformView.setBounds(rightArea.removeFromTop(waveformHeight));
    metadataPanel.setBounds(rightArea);
}

bool MainWindow::MainContent::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        audioEngine.togglePlayPause();
        return true;
    }
    return false;
}

void MainWindow::MainContent::timerCallback()
{
    if (audioEngine.isPlaying())
    {
        waveformView.setPlaybackPosition(audioEngine.getPosition());
    }
}

void MainWindow::MainContent::refreshLibraries()
{
    if (db == nullptr) return;

    auto libs = db->getLibraries();
    for (auto& lib : libs)
    {
        lib.fileCount = db->getFileCount(lib.id);
    }
    sidebar.setLibraries(libs);
}

void MainWindow::MainContent::performSearch()
{
    if (searchEngine == nullptr) return;

    std::vector<AudioFileInfo> results;
    if (activeQuery.trim().isEmpty())
    {
        results = searchEngine->getAllFiles(activeLibraryId, 5000);
    }
    else
    {
        results = searchEngine->search(activeQuery, activeLibraryId, 2000);
    }

    resultsTable.setResults(results);
}

// ---- LibrarySidebar::Listener ----
void MainWindow::MainContent::librarySelected(int64_t libraryId)
{
    activeLibraryId = libraryId;
    performSearch();
}

void MainWindow::MainContent::addLibraryRequested()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio Folder to Add to Library",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto result = fc.getResult();
        if (result.isDirectory())
        {
            auto folderName = result.getFileName();
            auto libId = db->createLibrary(folderName);
            if (libId > 0)
            {
                db->addRootPath(libId, result.getFullPathName());
                activeLibraryId = libId;
                refreshLibraries();
                sidebar.setScanStatus("Scanning " + folderName + "...");
                scanner->startScan(libId, { result.getFullPathName() }, false);
            }
        }
    });
}

// ---- SearchBar::Listener ----
void MainWindow::MainContent::searchTextChanged(const juce::String& query)
{
    activeQuery = query;
    performSearch();
}

void MainWindow::MainContent::searchSubmitted(const juce::String& query)
{
    activeQuery = query;
    performSearch();
}

// ---- ResultsTable::Listener ----
void MainWindow::MainContent::fileSelected(const AudioFileInfo& info)
{
    activeFileInfo = info;
    juce::File audioFile(info.filePath);

    waveformView.loadFile(audioFile);
    metadataPanel.displayMetadata(info);
    audioEngine.loadFile(audioFile);
}

void MainWindow::MainContent::fileDoubleClicked(const AudioFileInfo& info)
{
    activeFileInfo = info;
    juce::File audioFile(info.filePath);

    waveformView.loadFile(audioFile);
    metadataPanel.displayMetadata(info);
    audioEngine.loadFile(audioFile);
    audioEngine.play();
}

// ---- WaveformView::Listener ----
void MainWindow::MainContent::positionClicked(double normalizedPosition)
{
    audioEngine.seekTo(normalizedPosition);
    if (!audioEngine.isPlaying())
        audioEngine.play();
}

// ---- MetadataPanel::Listener ----
void MainWindow::MainContent::metadataChanged(const juce::String& /*field*/,
                                              const juce::String& /*newValue*/)
{
}

void MainWindow::MainContent::metadataSaveRequested()
{
    if (!activeFileInfo.has_value()) return;

    metadataPanel.populateUpdatedInfo(*activeFileInfo);

    // 1. Write metadata chunks back to disk
    metadataWriter.writeMetadata(*activeFileInfo);

    // 2. Update SQLite database
    db->upsertFile(*activeFileInfo);

    // 3. Refresh table
    performSearch();
}

// ---- Scanner::Listener ----
void MainWindow::MainContent::scanProgressUpdated(const Scanner::Progress& progress)
{
    juce::MessageManager::callAsync([this, progress]()
    {
        sidebar.setScanStatus("Indexed: " + juce::String(progress.filesProcessed) +
                              " (" + juce::String(progress.filesFound) + " found)");
    });
}

void MainWindow::MainContent::scanCompleted(const Scanner::Progress& finalProgress)
{
    juce::MessageManager::callAsync([this, finalProgress]()
    {
        sidebar.setScanStatus("Ready: " + juce::String(finalProgress.filesProcessed) + " files");
        refreshLibraries();
        performSearch();
    });
}

void MainWindow::MainContent::scanCancelled()
{
    juce::MessageManager::callAsync([this]()
    {
        sidebar.setScanStatus("Scan cancelled");
    });
}

} // namespace librairy

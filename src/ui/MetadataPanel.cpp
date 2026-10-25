#include "MetadataPanel.h"

namespace librairy
{

MetadataPanel::MetadataPanel()
{
    titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    saveButton.onClick = [this]()
    {
        listeners.call([](Listener& l) { l.metadataSaveRequested(); });
    };
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E66B2));
    addAndMakeVisible(saveButton);

    savedNotificationLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    savedNotificationLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF4CAF50)); // Green
    savedNotificationLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(savedNotificationLabel);

    viewport.setViewedComponent(&fieldsContainer, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    // Create standard metadata fields
    createField("filename",     "Filename");
    createField("description",  "Description");
    createField("category",     "Category");
    createField("subcategory",  "Subcategory");
    createField("originator",   "Originator");
    createField("originatorRef","Originator Ref");
    createField("date",         "Date");
    createField("timeRef",      "Time Reference");
    createField("notes",        "Notes");
    createField("keywords",     "Keywords");
}

void MetadataPanel::createField(const juce::String& name,
                                 const juce::String& displayName)
{
    auto field = std::make_unique<MetadataField>();
    field->fieldName = name;

    field->label.setText(displayName, juce::dontSendNotification);
    field->label.setColour(juce::Label::textColourId,
                           juce::Colour(0xFFAAAAAA));
    field->label.setFont(juce::FontOptions(12.0f));
    fieldsContainer.addAndMakeVisible(field->label);

    field->editor.setMultiLine(false);
    field->editor.setColour(juce::TextEditor::backgroundColourId,
                            juce::Colour(0xFF2D2D2D));
    field->editor.setColour(juce::TextEditor::textColourId,
                            juce::Colours::white);
    field->editor.onTextChange = [this, raw = field.get()]()
    {
        listeners.call([&](Listener& l)
        {
            l.metadataChanged(raw->fieldName, raw->editor.getText());
        });
    };
    fieldsContainer.addAndMakeVisible(field->editor);

    fields.push_back(std::move(field));
}

void MetadataPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Top border
    g.setColour(juce::Colour(0xFF3D3D3D));
    g.drawLine(0.f, 0.f,
               static_cast<float>(getWidth()), 0.f);
}

MetadataPanel::~MetadataPanel()
{
    stopTimer();
}

void MetadataPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);

    auto header = bounds.removeFromTop(28);
    saveButton.setBounds(header.removeFromRight(70));
    header.removeFromRight(8);
    savedNotificationLabel.setBounds(header.removeFromRight(220));
    titleLabel.setBounds(header);

    bounds.removeFromTop(4);
    viewport.setBounds(bounds);

    // Layout fields vertically inside the viewport container
    int yPos = 0;
    int fieldHeight = 28;
    int labelWidth = 110;
    int containerWidth = bounds.getWidth() - 16; // Account for scrollbar

    for (auto& field : fields)
    {
        field->label.setBounds(0, yPos, labelWidth, fieldHeight);
        field->editor.setBounds(labelWidth + 4, yPos,
                                containerWidth - labelWidth - 4,
                                fieldHeight);
        yPos += fieldHeight + 4;
    }

    fieldsContainer.setSize(containerWidth, yPos);
}

void MetadataPanel::showSaveConfirmation()
{
    saveButton.setButtonText("Saved!");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E7D32));
    savedNotificationLabel.setText("Saved successfully!", juce::dontSendNotification);
    startTimer(2500);
}

void MetadataPanel::timerCallback()
{
    stopTimer();
    saveButton.setButtonText("Save");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E66B2));
    savedNotificationLabel.setText("", juce::dontSendNotification);
}

void MetadataPanel::displayMetadata(const AudioFileInfo& info)
{
    for (auto& field : fields)
    {
        juce::String value;

        if      (field->fieldName == "filename")      value = info.filename;
        else if (field->fieldName == "description")   value = info.description;
        else if (field->fieldName == "category")      value = info.category;
        else if (field->fieldName == "subcategory")   value = info.subcategory;
        else if (field->fieldName == "originator")    value = info.originator;
        else if (field->fieldName == "originatorRef") value = info.originatorRef;
        else if (field->fieldName == "date")          value = info.dateModified;
        else if (field->fieldName == "notes")         value = info.notes;
        else if (field->fieldName == "keywords")      value = info.keywords;

        field->editor.setText(value, juce::dontSendNotification);
    }
}

void MetadataPanel::populateUpdatedInfo(AudioFileInfo& info) const
{
    for (const auto& field : fields)
    {
        auto val = field->editor.getText().trim();
        if      (field->fieldName == "filename")      info.filename = val;
        else if (field->fieldName == "description")   info.description = val;
        else if (field->fieldName == "category")      info.category = val;
        else if (field->fieldName == "subcategory")   info.subcategory = val;
        else if (field->fieldName == "originator")    info.originator = val;
        else if (field->fieldName == "originatorRef") info.originatorRef = val;
        else if (field->fieldName == "notes")         info.notes = val;
        else if (field->fieldName == "keywords")      info.keywords = val;
    }
}

void MetadataPanel::clear()
{
    for (auto& field : fields)
        field->editor.clear();
}

void MetadataPanel::addListener(Listener* listener)    { listeners.add(listener); }
void MetadataPanel::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy


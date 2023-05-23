#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"



class TimeInputComponent : public juce::Component,
                           public juce::TextEditor::Listener
{
public:
    TimeInputComponent(tracktion::TimePosition defaultTimePosition)
    {
        addAndMakeVisible(m_minutesEditor);
        addAndMakeVisible(m_secondsEditor);
        addAndMakeVisible(m_millisecondsEditor);
        addAndMakeVisible(m_separator1);
        addAndMakeVisible(m_separator2);

        m_minutesEditor.setJustification(juce::Justification::centred);
        m_secondsEditor.setJustification(juce::Justification::centred);
        m_millisecondsEditor.setJustification(juce::Justification::centred);


        m_minutesEditor.setInputRestrictions(2, "0123456789");
        m_secondsEditor.setInputRestrictions(2, "0123456789");
        m_millisecondsEditor.setInputRestrictions(3, "0123456789");

        m_minutesEditor.addListener(this);
        m_secondsEditor.addListener(this);
        m_millisecondsEditor.addListener(this);

        m_separator1.setText(":", juce::dontSendNotification);
        m_separator2.setText(".", juce::dontSendNotification);

        setTimePos(defaultTimePosition);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto fieldWidth = area.getWidth() / 8; // Adjust according to your needs
        auto separatorWidth = area.getWidth() / 8; // Adjust according to your needs

        m_minutesEditor.setBounds(area.removeFromLeft(fieldWidth * 2));
        m_separator1.setBounds(area.removeFromLeft(separatorWidth).withSizeKeepingCentre(separatorWidth, area.getHeight()));
        m_secondsEditor.setBounds(area.removeFromLeft(fieldWidth * 2));
        m_separator2.setBounds(area.removeFromLeft(separatorWidth).withSizeKeepingCentre(separatorWidth, area.getHeight()));
        m_millisecondsEditor.setBounds(area.removeFromLeft(fieldWidth * 3));
    }

    void textEditorTextChanged(juce::TextEditor &editor) override
    {
        if (&editor == &m_secondsEditor || &editor == &m_millisecondsEditor) {
            auto value = editor.getText().getIntValue();
            if (value >= 60 && &editor == &m_secondsEditor) {
                m_secondsEditor.setText("59", juce::NotificationType::dontSendNotification);
            } else if (value > 999 && &editor == &m_millisecondsEditor) {
                m_millisecondsEditor.setText("999", juce::NotificationType::dontSendNotification);
            }
        }
    }

    void setTimePos(tracktion::TimePosition pos)
    {
        int totalSeconds = static_cast<int>(pos.inSeconds());
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        int milliseconds = static_cast<int>((pos.inSeconds() - totalSeconds) * 1000);

        m_minutesEditor.setText(juce::String::formatted("%02d", minutes), juce::dontSendNotification);
        m_secondsEditor.setText(juce::String::formatted("%02d", seconds), juce::dontSendNotification);
        m_millisecondsEditor.setText(juce::String::formatted("%03d", milliseconds), juce::dontSendNotification);
    }

    tracktion::TimePosition getCurrentTimePosition() const
    {
        int minutes = m_minutesEditor.getText().getIntValue();
        int seconds = m_secondsEditor.getText().getIntValue();
        int milliseconds = m_millisecondsEditor.getText().getIntValue();

        double totalSeconds = minutes * 60 + seconds + milliseconds / 1000.0;
        return tracktion::TimePosition::fromSeconds(totalSeconds);
    }

private:
    juce::TextEditor m_minutesEditor;
    juce::TextEditor m_secondsEditor;
    juce::TextEditor m_millisecondsEditor;
    juce::Label m_separator1, m_separator2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeInputComponent)
    };

class RenderDialog : public juce::Component,
                     private juce::FilenameComponentListener,
                     private juce::Button::Listener
{
public:
    RenderDialog(EditViewState& evs)
        : m_fileChooser ("Render File", juce::File(evs.m_applicationState.m_renderDir), false, true, false, "", {}, "Select folder for saving render to")
        , m_evs(evs)
        , m_rangeStart(evs.m_edit.getTransport().getLoopRange().getStart())
        , m_rangeEnd(evs.m_edit.getTransport().getLoopRange().getEnd())
    {
        setSize (400, 400);

        addAndMakeVisible (m_fileChooser);
        m_fileChooser.addListener (this);
        addAndMakeVisible(m_folderLabel);
        m_folderLabel.setText("Folder: ", juce::dontSendNotification);
        
        addAndMakeVisible(m_fileInput);
        m_fileInput.setMultiLine(false, false);
        m_fileInput.setInputRestrictions(256, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.");
        addAndMakeVisible(m_filenameLabel);
        m_filenameLabel.setText("Enter filename: ", juce::dontSendNotification);  
        addAndMakeVisible(m_suffixLabel);
        m_suffixLabel.setText(".wav", juce::dontSendNotification);  

        addAndMakeVisible(m_setRangeLabel);
        m_setRangeLabel.setJustificationType(juce::Justification::centred);
        m_setRangeLabel.setText("Enter range to render (min:sec.millisec):", juce::dontSendNotification);

        addAndMakeVisible (m_rangeStartLabel);
        m_rangeStartLabel.setText ("Start : ", juce::dontSendNotification);

        addAndMakeVisible (m_rangeEndLabel);
        m_rangeEndLabel.setJustificationType(juce::Justification::right);
        m_rangeEndLabel.setText ("End: ", juce::dontSendNotification);
        juce::String newString; newString.removeCharacters("0123456789:.");
        addAndMakeVisible (m_rangeStart);
        addAndMakeVisible (m_rangeEnd);

        addAndMakeVisible(m_specialRangeLabel);
        m_specialRangeLabel.setJustificationType(juce::Justification::centred);
        m_specialRangeLabel.setText("Render range from: ", juce::dontSendNotification);
        addAndMakeVisible(m_setLoopRangeAsRangeButton);
        m_setLoopRangeAsRangeButton.setButtonText ("Loop range");
        m_setLoopRangeAsRangeButton.addListener (this);

        addAndMakeVisible(m_setEditRangeAsRangeButton);
        m_setEditRangeAsRangeButton.setButtonText ("Edit range");
        m_setEditRangeAsRangeButton.addListener (this);

        addAndMakeVisible (m_startButton);
        m_startButton.setButtonText ("Start Render");
        m_startButton.addListener (this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.reduce(10, 10);
                
        bounds.removeFromTop(15);

        auto folderRect = bounds.removeFromTop (20);
        m_folderLabel.setBounds(folderRect.removeFromLeft(getWidth() / 3));
        m_fileChooser.setBounds (folderRect);

        bounds.removeFromTop(15);

        auto filenameRect = bounds.removeFromTop(20);
        m_filenameLabel.setBounds(filenameRect.removeFromLeft(getWidth()/3));
        auto file = te::EditFileOperations(m_evs.m_edit).getEditFile();
        m_fileInput.setText(file.getFileNameWithoutExtension());
        m_fileInput.setBounds(filenameRect.removeFromLeft((filenameRect.getWidth() / 3)* 2));
        m_suffixLabel.setBounds(filenameRect);

        bounds.removeFromTop(30);

        m_setRangeLabel.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(15);
        auto rangeRect = bounds.removeFromTop(20);
        auto startRect = rangeRect.removeFromLeft(rangeRect.getWidth() / 2);

        m_rangeStartLabel.setBounds (startRect.removeFromLeft(startRect.getWidth() / 3));
        m_rangeStart.setBounds (startRect);

        m_rangeEndLabel.setBounds (rangeRect.removeFromLeft(rangeRect.getWidth() / 3));
        m_rangeEnd.setBounds (rangeRect);

        bounds.removeFromTop(15);

        m_specialRangeLabel.setBounds(bounds.removeFromTop(20));

        bounds.removeFromTop(15);

        auto buttonRect = bounds.removeFromTop(20);
        m_setLoopRangeAsRangeButton.setBounds(buttonRect.removeFromLeft(bounds.getWidth()/2).reduced(20, 0));
        m_setEditRangeAsRangeButton.setBounds(buttonRect.reduced(20,0));

        bounds.removeFromBottom(30);
        m_startButton.setBounds (bounds.removeFromBottom (30).reduced(100, 5));
    }

    tracktion::TimeRange getTimeRange() const
    {
        return { m_rangeStart.getCurrentTimePosition(), m_rangeEnd.getCurrentTimePosition() };
    }

    juce::File getRenderFile() const
    {
        return m_fileChooser.getCurrentFile().getNonexistentChildFile(m_fileInput.getText(), ".wav");
    }

private:
    void filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged) override
    {
        if (fileComponentThatHasChanged == &m_fileChooser)
        {
            // Do something when the file selection changes...
        }
    }

    void buttonClicked (juce::Button* button) override
    {
        if (button == &m_startButton)
        {
            GUIHelpers::log(getRenderFile().getFullPathName());
            if (getRenderFile().isDirectory())
            {
                GUIHelpers::log("no file choosen!");
                return;
            }
            if (getRenderFile().existsAsFile())
            {
                GUIHelpers::log("an existing file is choosen");
                return;
            }
            GUIHelpers::log("this is a new created file"); 
            EngineHelpers::renderEditToFile(m_evs, getRenderFile(), getTimeRange()); 
        }

        if (button == &m_setLoopRangeAsRangeButton)
        {
            m_rangeStart.setTimePos(m_evs.m_edit.getTransport().getLoopRange().getStart());
            m_rangeEnd.setTimePos(m_evs.m_edit.getTransport().getLoopRange().getEnd());
        }

        if (button == &m_setEditRangeAsRangeButton)
        {
            auto start = tracktion::TimePosition::fromSeconds(0.0);
            m_rangeStart.setTimePos(start);
            m_rangeEnd.setTimePos(start + m_evs.m_edit.getLength());
        }
    }

    EditViewState& m_evs;
    juce::FilenameComponent m_fileChooser;
    TimeInputComponent m_rangeStart, m_rangeEnd;
    juce::Label m_rangeStartLabel, m_rangeEndLabel, m_folderLabel, m_filenameLabel, m_suffixLabel, m_setRangeLabel, m_specialRangeLabel;
    juce::TextEditor m_fileInput;
    juce::TextButton m_startButton, m_setLoopRangeAsRangeButton, m_setEditRangeAsRangeButton;
};


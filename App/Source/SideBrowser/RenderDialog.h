
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities/ApplicationViewState.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

class TimeInputComponent
    : public juce::GroupComponent
    , public juce::TextEditor::Listener
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

        m_separator1.setJustificationType(juce::Justification::centred);
        m_separator2.setJustificationType(juce::Justification::centred);
        m_separator1.setText(":", juce::dontSendNotification);
        m_separator2.setText(".", juce::dontSendNotification);

        setTimePos(defaultTimePosition);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(5);
        auto separatorWidth = area.getWidth() / 12; // Adjust according to your needs

        auto fieldWidth = (area.reduced(15, 0).getWidth() / 3) - (2 * separatorWidth);
        int stripHeight = 20;
        int centerY = area.getCentreY();

        juce::Rectangle<int> centerStrip = area.withTrimmedTop(centerY - stripHeight / 2).withHeight(stripHeight);
        centerStrip.reduce(10, 0);

        m_minutesEditor.setBounds(centerStrip.removeFromLeft(fieldWidth * 2));
        m_separator1.setBounds(centerStrip.removeFromLeft(separatorWidth).withSizeKeepingCentre(separatorWidth, centerStrip.getHeight()));
        m_secondsEditor.setBounds(centerStrip.removeFromLeft(fieldWidth * 2));
        m_separator2.setBounds(centerStrip.removeFromLeft(separatorWidth).withSizeKeepingCentre(separatorWidth, centerStrip.getHeight()));
        m_millisecondsEditor.setBounds(centerStrip.removeFromLeft(fieldWidth * 2));
    }

    void textEditorTextChanged(juce::TextEditor &editor) override
    {
        if (&editor == &m_secondsEditor || &editor == &m_millisecondsEditor)
        {
            auto value = editor.getText().getIntValue();
            if (value >= 60 && &editor == &m_secondsEditor)
            {
                m_secondsEditor.setText("59", juce::NotificationType::dontSendNotification);
            }
            else if (value > 999 && &editor == &m_millisecondsEditor)
            {
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeInputComponent)
};

class FileGroup : public juce::GroupComponent
{
public:
    FileGroup(EditViewState &evs)
        : m_evs(evs),
          m_dirChooser("Render File", juce::File(evs.m_applicationState.m_renderDir), false, true, false, "", {}, "Select folder for saving render to")
    {
        setText("set output file");
        addAndMakeVisible(m_dirChooser);
        addAndMakeVisible(m_folderLabel);
        m_folderLabel.setText("Folder: ", juce::dontSendNotification);

        addAndMakeVisible(m_fileInput);
        m_fileInput.setMultiLine(false, false);
        m_fileInput.setInputRestrictions(256, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.");
        addAndMakeVisible(m_filenameLabel);
        m_filenameLabel.setText("Enter filename: ", juce::dontSendNotification);
        addAndMakeVisible(m_suffixLabel);
        m_suffixLabel.setText(".wav", juce::dontSendNotification);
    }
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.reduce(10, 10);

        bounds.removeFromTop(15);

        auto folderRect = bounds.removeFromTop(25);
        m_folderLabel.setBounds(folderRect.removeFromLeft(getWidth() / 3));
        m_dirChooser.setBounds(folderRect);

        bounds.removeFromTop(15);

        auto filenameRect = bounds.removeFromTop(25);
        m_filenameLabel.setBounds(filenameRect.removeFromLeft(getWidth() / 3));
        auto file = te::EditFileOperations(m_evs.m_edit).getEditFile();
        m_fileInput.setText(file.getFileNameWithoutExtension());
        m_fileInput.setBounds(filenameRect.removeFromLeft((filenameRect.getWidth() / 3) * 2));
        m_suffixLabel.setBounds(filenameRect);
    }

    juce::File getCurrentFile() const { return m_dirChooser.getCurrentFile().getNonexistentChildFile(m_fileInput.getText(), ".wav"); }

private:
    EditViewState &m_evs;
    juce::FilenameComponent m_dirChooser;
    juce::Label m_folderLabel, m_filenameLabel, m_suffixLabel;
    juce::TextEditor m_fileInput;
};

class RangeGroup
    : public juce::GroupComponent

    , private juce::Button::Listener
{
public:
    RangeGroup(EditViewState &evs)
        : m_evs(evs),
          m_rangeStart(evs.m_edit.getTransport().getLoopRange().getStart()),
          m_rangeEnd(evs.m_edit.getTransport().getLoopRange().getEnd())
    {
        setText("range to render");
        addAndMakeVisible(m_setRangeLabel);
        m_setRangeLabel.setJustificationType(juce::Justification::centred);
        m_setRangeLabel.setText("Enter range (min:sec.millisec):", juce::dontSendNotification);

        juce::String newString;
        newString.removeCharacters("0123456789:.");
        addAndMakeVisible(m_rangeStart);
        m_rangeStart.setText("Start");
        addAndMakeVisible(m_rangeEnd);
        m_rangeEnd.setText("End");

        addAndMakeVisible(m_specialRangeLabel);
        m_specialRangeLabel.setJustificationType(juce::Justification::centred);
        m_specialRangeLabel.setText("Render range from: ", juce::dontSendNotification);
        addAndMakeVisible(m_setLoopRangeAsRangeButton);
        m_setLoopRangeAsRangeButton.setButtonText("Loop range");
        m_setLoopRangeAsRangeButton.addListener(this);

        addAndMakeVisible(m_setEditRangeAsRangeButton);
        m_setEditRangeAsRangeButton.setButtonText("Edit range");
        m_setEditRangeAsRangeButton.addListener(this);
    }
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.reduce(10, 10);

        bounds.removeFromTop(15);
        auto rangeRect = bounds.removeFromTop(120);

        m_rangeStart.setBounds(rangeRect.removeFromTop((rangeRect.getHeight() / 2) - 5));
        rangeRect.removeFromTop(5);
        m_rangeEnd.setBounds(rangeRect);

        m_specialRangeLabel.setBounds(bounds.removeFromTop(25));

        auto buttonRect = bounds.removeFromTop(25);
        m_setLoopRangeAsRangeButton.setBounds(buttonRect.removeFromLeft(bounds.getWidth() / 2).reduced(20, 0));
        m_setEditRangeAsRangeButton.setBounds(buttonRect.reduced(20, 0));
    }

    TimeInputComponent m_rangeStart, m_rangeEnd;

private:
    void buttonClicked(juce::Button *button) override
    {
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
    EditViewState &m_evs;

    juce::Label m_setRangeLabel, m_specialRangeLabel;
    juce::TextButton m_setLoopRangeAsRangeButton, m_setEditRangeAsRangeButton;
};

class RenderDialog
    : public juce::Component
    , private juce::Button::Listener
{
public:
    RenderDialog(EditViewState &evs)
        : m_evs(evs),
          m_fileGroup(evs),
          m_rangeGroup(evs)
    {
        setSize(400, 400);
        addAndMakeVisible(m_fileGroup);
        addAndMakeVisible(m_rangeGroup);

        addAndMakeVisible(m_startButton);
        m_startButton.setButtonText("Start Render");
        m_startButton.addListener(this);
    }

    void paint(juce::Graphics &g) override { g.fillAll(m_evs.m_applicationState.getBackgroundColour1()); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        if (bounds.getWidth() < 190)
            bounds = bounds.withWidth(190);
        if (bounds.getWidth() > 350)
            bounds = bounds.withWidth(350);
        if (bounds.getHeight() < 435)
            bounds = bounds.withHeight(435);

        bounds.reduce(10, 10);
        m_fileGroup.setBounds(bounds.removeFromTop(120));
        bounds.removeFromTop(15);
        m_rangeGroup.setBounds(bounds.removeFromTop(240));

        bounds.removeFromTop(15);
        m_startButton.setBounds(bounds.removeFromTop(25).reduced(bounds.getWidth() / 4, 0));
    }

    tracktion::TimeRange getTimeRange() const { return {m_rangeGroup.m_rangeStart.getCurrentTimePosition(), m_rangeGroup.m_rangeEnd.getCurrentTimePosition()}; }

    juce::File getRenderFile() const { return m_fileGroup.getCurrentFile(); }

private:
    void buttonClicked(juce::Button *button) override
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
    }

    EditViewState &m_evs;
    FileGroup m_fileGroup;
    RangeGroup m_rangeGroup;
    juce::TextButton m_startButton;
};

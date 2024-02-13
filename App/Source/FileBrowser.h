//
// /*
//  * Copyright 2023 Steffen Baranowsky
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program. If not, see <http://www.gnu.org/licenses/>.
//  */
//
#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"
#include "PreviewComponent.h"
#include "Browser_Base.h"

namespace te = tracktion_engine;
class FileBrowserComponent;
class PathComponent : public juce::Component
                    , public juce::ChangeBroadcaster
{
public:
    PathComponent(juce::File dir, ApplicationViewState& appState) ;

    void paint (juce::Graphics& g) override
    {
        g.fillAll(m_appState.getBackgroundColour());
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
    }

    void resized() override;

    void setDir(juce::File file);
    juce::File getCurrentPath();

private:


    juce::TextEditor m_currentPathField;
    juce::TextButton m_button{"^"};
    juce::File m_currentPath;

    ApplicationViewState& m_appState;
};

class FileBrowserComponent : public BrowserBaseComponent
{
public:
    FileBrowserComponent(ApplicationViewState& avs, te::Engine&, SamplePreviewComponent& spc);
    ~FileBrowserComponent() override;
    void resized() override;
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;

    juce::var getDragSourceDescription (const juce::SparseSet<int>& /*rowsToDescribe*/) override;

    void setDirecory(const juce::File& dir);
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void previewSampleFile(const juce::File& file);

private:
    void sortList(bool forward=true) override;
    struct CompareNameForward{
        static int compareElements (const juce::File& first, 
                                              const juce::File& second)
        {   
            return first.getFileName().compareNatural(second.getFileName());
        }
    };

    struct CompareNameBackwards{
        static int compareElements(const juce::File& first, 
                                               const juce::File& second)
        {
            return second.getFileName().compareNatural(first.getFileName());
        }
    };
    void sortByName(juce::Array<juce::File>& list, bool forward);

    SamplePreviewComponent&     m_samplePreviewComponent;
    PathComponent               m_currentPathField; 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileBrowserComponent)
};

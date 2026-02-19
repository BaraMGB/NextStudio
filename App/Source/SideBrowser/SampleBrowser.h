
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
#include "SideBrowser/Browser_Base.h"
#include "SideBrowser/SearchFieldComponent.h"
#include "UI/PreviewComponent.h"
#include "Utilities/ApplicationViewState.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;
class SampleBrowserComponent : public BrowserBaseComponent
{
public:
    SampleBrowserComponent(ApplicationViewState &avs, SamplePreviewComponent &spc);
    void resized() override;
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;

    juce::var getDragSourceDescription(const juce::SparseSet<int> & /*rowsToDescribe*/) override;

    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;

    void previewSampleFile(const juce::File &file);

private:
    void sortList(int selectedID) override;

    struct CompareNameForward
    {
        static int compareElements(const juce::File &first, const juce::File &second) { return first.getFileName().compareNatural(second.getFileName()); }
    };

    struct CompareNameBackwards
    {
        static int compareElements(const juce::File &first, const juce::File &second) { return second.getFileName().compareNatural(first.getFileName()); }
    };
    void sortByName(juce::Array<juce::File> &list, bool forward);
    void shuffleFileArray(juce::Array<juce::File> &fileList);
    SamplePreviewComponent &m_samplePreviewComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowserComponent)
};

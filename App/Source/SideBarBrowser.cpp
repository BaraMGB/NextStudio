
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

﻿/*
  ==============================================================================

    SideBarBrowser.cpp
    Created: 2 Mar 2020 10:26:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SideBarBrowser.h"
#include "MainComponent.h"

void SideBarBrowser::fileDoubleClicked(const juce::File &)
{
    auto selectedFile = m_DirTreeViewBox.getSelectedFile();
    tracktion_engine::AudioFile audioFile(m_edit.engine, selectedFile);
    if (selectedFile.getFileExtension () == ".tracktionedit")
    {
        if (auto mc = dynamic_cast<MainComponent*>(getParentComponent ()))
        {
            mc->setupEdit(selectedFile);
        }
    }
    else if (audioFile.isValid ())
    {
        m_samplePreviewComponent.stop();
        auto trackColours = m_applicationState.m_trackColours;
        auto colour = trackColours[
                m_editViewState.m_edit.getTrackList().size()
              % trackColours.size()];
        EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, selectedFile, colour);
    }
}

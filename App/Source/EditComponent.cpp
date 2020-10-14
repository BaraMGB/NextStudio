#include "EditComponent.h"

static inline const char* getInternalPluginFormatName()     { return "TracktionInternal"; }

//==============================================================================
class PluginTreeBase
{
public:
    virtual ~PluginTreeBase() = default;
    virtual String getUniqueName() const = 0;
    
    void addSubItem (PluginTreeBase* itm)   { subitems.add (itm);       }
    int getNumSubItems()                    { return subitems.size();   }
    PluginTreeBase* getSubItem (int idx)    { return subitems[idx];     }
    
private:
    OwnedArray<PluginTreeBase> subitems;
};

//==============================================================================
class PluginTreeItem : public PluginTreeBase
{
public:
    PluginTreeItem (const PluginDescription&);
    PluginTreeItem (const String& uniqueId, const String& name, const String& xmlType, bool isSynth, bool isPlugin);

    te::Plugin::Ptr create (te::Edit&);
    
    String getUniqueName() const override
    {
        if (desc.fileOrIdentifier.startsWith (te::RackType::getRackPresetPrefix()))
            return desc.fileOrIdentifier;

        return desc.createIdentifierString();
    }

    PluginDescription desc;
    String xmlType;
    bool isPlugin = true;

    JUCE_LEAK_DETECTOR (PluginTreeItem)
};

//==============================================================================
class PluginTreeGroup : public PluginTreeBase
{
public:
    PluginTreeGroup (te::Edit&, KnownPluginList::PluginTree&, te::Plugin::Type);
    PluginTreeGroup (const String&);
    
    String getUniqueName() const override           { return name; }

    String name;

private:
    void populateFrom (KnownPluginList::PluginTree&);
    void createBuiltInItems (int& num, te::Plugin::Type);

    JUCE_LEAK_DETECTOR (PluginTreeGroup)
};

//==============================================================================
PluginTreeItem::PluginTreeItem (const PluginDescription& d)
    : desc (d), xmlType (te::ExternalPlugin::xmlTypeName), isPlugin (true)
{
    jassert (xmlType.isNotEmpty());
}

PluginTreeItem::PluginTreeItem (const String& uniqueId, const String& name,
                                const String& xmlType_, bool isSynth, bool isPlugin_)
    : xmlType (xmlType_), isPlugin (isPlugin_)
{
    jassert (xmlType.isNotEmpty());
    desc.name = name;
    desc.fileOrIdentifier = uniqueId;
    desc.pluginFormatName = (uniqueId.endsWith ("_trkbuiltin") || xmlType == te::RackInstance::xmlTypeName)
                                ? getInternalPluginFormatName() : String();
    desc.category = xmlType;
    desc.isInstrument = isSynth;
}

te::Plugin::Ptr PluginTreeItem::create (te::Edit& ed)
{
    return ed.getPluginCache().createNewPlugin (xmlType, desc);
}

//==============================================================================
PluginTreeGroup::PluginTreeGroup (te::Edit& edit, KnownPluginList::PluginTree& tree, te::Plugin::Type types)
    : name ("Plugins")
{
    {
        int num = 1;

        auto builtinFolder = new PluginTreeGroup (TRANS("Builtin Plugins"));
        addSubItem (builtinFolder);
        builtinFolder->createBuiltInItems (num, types);
    }

    {
        auto racksFolder = new PluginTreeGroup (TRANS("Plugin Racks"));
        addSubItem (racksFolder);

        racksFolder->addSubItem (new PluginTreeItem (String (te::RackType::getRackPresetPrefix()) + "-1",
                                                     TRANS("Create New Empty Rack"),
                                                     te::RackInstance::xmlTypeName, false, false));

        int i = 0;
        for (auto rf : edit.getRackList().getTypes())
            racksFolder->addSubItem (new PluginTreeItem ("RACK__" + String (i++), rf->rackName,
                                                         te::RackInstance::xmlTypeName, false, false));
    }

    populateFrom (tree);
}

PluginTreeGroup::PluginTreeGroup (const String& s)  : name (s)
{
    jassert (name.isNotEmpty());
}

void PluginTreeGroup::populateFrom (KnownPluginList::PluginTree& tree)
{
    for (auto subTree : tree.subFolders)
    {
        if (subTree->plugins.size() > 0 || subTree->subFolders.size() > 0)
        {
            auto fs = new PluginTreeGroup (subTree->folder);
            addSubItem (fs);

            fs->populateFrom (*subTree);
        }
    }

    for (const auto& pd : tree.plugins)
        addSubItem (new PluginTreeItem (pd));
}


template<class FilterClass>
void addInternalPlugin (PluginTreeBase& item, int& num, bool synth = false)
{
    item.addSubItem (new PluginTreeItem (String (num++) + "_trkbuiltin",
                                         TRANS (FilterClass::getPluginName()),
                                         FilterClass::xmlTypeName, synth, false));
}

void PluginTreeGroup::createBuiltInItems (int& num, te::Plugin::Type types)
{
    addInternalPlugin<te::VolumeAndPanPlugin> (*this, num);
    addInternalPlugin<te::LevelMeterPlugin> (*this, num);
    addInternalPlugin<te::EqualiserPlugin> (*this, num);
    addInternalPlugin<te::ReverbPlugin> (*this, num);
    addInternalPlugin<te::DelayPlugin> (*this, num);
    addInternalPlugin<te::ChorusPlugin> (*this, num);
    addInternalPlugin<te::PhaserPlugin> (*this, num);
    addInternalPlugin<te::CompressorPlugin> (*this, num);
    addInternalPlugin<te::PitchShiftPlugin> (*this, num);
    addInternalPlugin<te::LowPassPlugin> (*this, num);
    addInternalPlugin<te::MidiModifierPlugin> (*this, num);
    addInternalPlugin<te::MidiPatchBayPlugin> (*this, num);
    addInternalPlugin<te::PatchBayPlugin> (*this, num);
    addInternalPlugin<te::AuxSendPlugin> (*this, num);
    addInternalPlugin<te::AuxReturnPlugin> (*this, num);
    addInternalPlugin<te::TextPlugin> (*this, num);
    addInternalPlugin<te::FreezePointPlugin> (*this, num);

   #if TRACKTION_ENABLE_REWIRE
    addInternalPlugin<te::ReWirePlugin> (*this, num, true);
   #endif

    if (types == te::Plugin::Type::allPlugins)
    {
        addInternalPlugin<te::SamplerPlugin> (*this, num, true);
        addInternalPlugin<te::FourOscPlugin> (*this, num, true);
    }

    addInternalPlugin<te::InsertPlugin> (*this, num);

   #if ENABLE_INTERNAL_PLUGINS
    for (auto& d : PluginTypeBase::getAllPluginDescriptions())
        if (isPluginAuthorised (d))
            addSubItem (new PluginTreeItem (d));
   #endif
}

//==============================================================================
class PluginMenu : public PopupMenu
{
public:
    PluginMenu() = default;

    PluginMenu (PluginTreeGroup& node)
    {
        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subNode = dynamic_cast<PluginTreeGroup*> (node.getSubItem (i)))
                addSubMenu (subNode->name, PluginMenu (*subNode), true);

        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subType = dynamic_cast<PluginTreeItem*> (node.getSubItem (i)))
                addItem (subType->getUniqueName().hashCode(), subType->desc.name, true, false);
    }

    static PluginTreeItem* findType (PluginTreeGroup& node, int hash)
    {
        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subNode = dynamic_cast<PluginTreeGroup*> (node.getSubItem (i)))
                if (auto* t = findType (*subNode, hash))
                    return t;

        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto t = dynamic_cast<PluginTreeItem*> (node.getSubItem (i)))
                if (t->getUniqueName().hashCode() == hash)
                    return t;

        return nullptr;
    }

    PluginTreeItem* runMenu (PluginTreeGroup& node)
    {
        int res = show();

        if (res == 0)
            return nullptr;

        return findType (node, res);
    }
};

//==============================================================================
te::Plugin::Ptr showMenuAndCreatePlugin (te::Edit& edit)
{
    if (auto tree = EngineHelpers::createPluginTree (edit.engine))
    {
        PluginTreeGroup root (edit, *tree, te::Plugin::Type::allPlugins);
        PluginMenu m (root);

        if (auto type = m.runMenu (root))
            return type->create (edit);
    }
    
    return {};
}

//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : editViewState (evs), clip (c)
{
}


void ClipComponent::paint (Graphics& g)
{
    auto alpha = 1.0f;
    if (m_isDragging)
    {
        alpha = 0.2f;
    }

    g.fillAll (clip->getColour());
    g.setColour (Colours::black);
    if (m_isDragging)
    {
        g.setColour(Colours::grey);
    }

    if (editViewState.selectionManager.isSelected (clip.get()))
    {
        g.setColour (Colours::white);
    }

    g.drawRect (getLocalBounds());
}

void ClipComponent::mouseDown (const MouseEvent&event)
{
    if(!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown())
            {
                PopupMenu m;
                m.addItem(1, "Delete clip");
                m.addItem(2, "Copy clip");

                const int result = m.show();

                if (result == 0)
                {
                    // user dismissed the menu without picking anything
                }
                else if (result == 1)
                {
                    clip->removeFromParentTrack();
                    return;
                    // user picked item 1
                }
                else if (result == 2)
                {
                    auto clipContent = std::make_unique<te::Clipboard::Clips>();
                    clipContent->addClip(0, clip->state);
                    te::Clipboard::getInstance()->setContent(std::move(clipContent));
                    // (clip.get());
                }
            }
            else
            {
                editViewState.selectionManager.selectOnly (getClip ());
                editViewState.selectionManager.addToSelection(getClip().getClipTrack());
                m_clipPosAtMouseDown = clip->edit.tempoSequence.timeToBeats(clip->getPosition().getStart());
                setMouseCursor (MouseCursor::DraggingHandCursor);
            }
        }

    /*tracktion_engine::Clipboard::ContentType::pasteIntoEdit(editViewState.edit, insertPoint, editViewState.selectionManager);*/
    m_isDragging = true;
}

void ClipComponent::mouseDrag(const MouseEvent & event)
{
    editViewState.edit.getTransport ().setUserDragging (true);
    DragAndDropContainer* dragC = DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {

        dragC->startDragging("Clip", this,juce::Image(Image::ARGB,1,1,true),
                             false);
    }

    auto newPos = jmax(
                0.0,
                editViewState.beatToTime(
                    (m_clipPosAtMouseDown - editViewState.viewX1)
                    + editViewState.xToBeats(event.getDistanceFromDragStartX(), getParentWidth())
                    )
                );
    if (!event.mods.isShiftDown())
    {
        newPos = clip->edit.getTimecodeFormat().getSnapType(editViewState.snapType)
                                             .roundTimeNearest(newPos, clip->edit.tempoSequence);
    }
    clip->setStart(newPos, false, true);


}

void ClipComponent::mouseUp(const MouseEvent &)
{
    editViewState.edit.getTransport ().setUserDragging (false);
    m_isDragging = false;
    setMouseCursor (MouseCursor::NormalCursor);
}

//==============================================================================
AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
    , thumbnailComponent (evs)
{
    addAndMakeVisible (thumbnailComponent);
    thumbnailComponent.setFile (getWaveAudioClip ()->getOriginalFile ());
}

void AudioClipComponent::paint (Graphics& g)
{
    ClipComponent::paint (g);
    auto area = getLocalBounds();
    area.reduce(1,1);
    g.setColour(getClip().getColour().darker());
    g.fillRect(area.removeFromTop(10));
}

void AudioClipComponent::resized()
{
    auto leftOffset = 0;
    if(getBoundsInParent ().getX () < 0)
    {
        leftOffset = 0 - getBoundsInParent ().getX ();
    }

    auto rightOffset = 0;
    if (getBoundsInParent().getRight() > getParentWidth())
    {
        rightOffset = getBoundsInParent().getRight() - getParentWidth();
    }

    thumbnailComponent.setBounds (0 + leftOffset,0,(getWidth() - leftOffset) - rightOffset, getHeight ());
}





//==============================================================================
MidiClipComponent::MidiClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
{
}

void MidiClipComponent::paint (Graphics& g)
{
    ClipComponent::paint (g);
    
    if (auto mc = getMidiClip())
    {
        auto& seq = mc->getSequence();
        for (auto n : seq.getNotes())
        {
            double sBeat = /*mc->getStartBeat() +*/ n->getStartBeat();
            double eBeat = /*mc->getStartBeat() + */n->getEndBeat();
            if (auto p = getParentComponent())
            {
                double y = (1.0 - double (n->getNoteNumber()) / 127.0) * getHeight();
                
                auto x1 =  editViewState.beatsToX (sBeat + editViewState.viewX1, p->getWidth ());
                auto x2 =  editViewState.beatsToX (eBeat + editViewState.viewX1, p->getWidth ());


                g.setColour (Colours::white);
                g.drawLine (float (x1), float (y), float (x2), float (y));
            }
        }
    }
}

//==============================================================================
RecordingClipComponent::RecordingClipComponent (te::Track::Ptr t, EditViewState& evs)
    : track (t), editViewState (evs)
{
    startTimerHz (10);
    initialiseThumbnailAndPunchTime();
}

void RecordingClipComponent::initialiseThumbnailAndPunchTime()
{
    if (auto at = dynamic_cast<te::AudioTrack*> (track.get()))
    {
        for (auto* idi : at->edit.getEditInputDevices().getDevicesForTargetTrack (*at))
        {
            punchInTime = idi->getPunchInTime();
            
            if (idi->getRecordingFile().exists())
                thumbnail = at->edit.engine.getRecordingThumbnailManager().getThumbnailFor (idi->getRecordingFile());
        }
    }
}

void RecordingClipComponent::paint (Graphics& g)
{
    g.fillAll (Colours::red.withAlpha (0.5f));
    g.setColour (Colours::black);
    g.drawRect (getLocalBounds());
    
    if (editViewState.drawWaveforms)
        drawThumbnail (g, Colours::black.withAlpha (0.5f));
}

void RecordingClipComponent::drawThumbnail (Graphics& g, Colour waveformColour) const
{
    if (thumbnail == nullptr)
        return;
    
    Rectangle<int> bounds;
    Range<double> times;
    getBoundsAndTime (bounds, times);
    auto w = bounds.getWidth();
    
    if (w > 0 && w < 10000)
    {
        g.setColour (waveformColour);
        thumbnail->thumb.drawChannels (g, bounds, w, times, 1.0f);
    }
}

bool RecordingClipComponent::getBoundsAndTime (Rectangle<int>& bounds, Range<double>& times) const
{
    auto editTimeToX = [this] (double t)
    {
        if (auto p = getParentComponent())
            return editViewState.edit.tempoSequence.beatsToTime(editViewState.beatsToX (t, p->getWidth()) - getX());
        return 0.0;
    };
    
    auto xToEditTime = [this] (int x)
    {
        if (auto p = getParentComponent())
            return editViewState.edit.tempoSequence.beatsToTime(editViewState.xToBeats (x + getX(), p->getWidth()));
        return 0.0;
    };
    
    bool hasLooped = false;
    auto& edit = track->edit;
    
    if (auto* playhead = edit.getTransport().getCurrentPlayhead())
    {
        auto localBounds = getLocalBounds();
        
        auto timeStarted = thumbnail->punchInTime;
        auto unloopedPos = timeStarted + thumbnail->thumb.getTotalLength();
        
        auto t1 = timeStarted;
        auto t2 = unloopedPos;
        
        if (playhead->isLooping() && t2 >= playhead->getLoopTimes().end)
        {
            hasLooped = true;
            
            t1 = jmin (t1, playhead->getLoopTimes().start);
            t2 = playhead->getPosition();
            
            t1 = jmax (editViewState.viewX1.get(), t1);
            t2 = jmin (editViewState.viewX2.get(), t2);
        }
        else if (edit.recordingPunchInOut)
        {
            const double in  = thumbnail->punchInTime;
            const double out = edit.getTransport().getLoopRange().getEnd();
            
            t1 = jlimit (in, out, t1);
            t2 = jlimit (in, out, t2);
        }
        
        bounds = localBounds.withX (jmax (localBounds.getX(), static_cast<int>(editTimeToX (t1))))
                 .withRight (jmin (localBounds.getRight(), static_cast<int>(editTimeToX (t2))));
        
        auto loopRange = playhead->getLoopTimes();
        const double recordedTime = unloopedPos - playhead->getLoopTimes().start;
        const int numLoops = (int) (recordedTime / loopRange.getLength());
        
        const Range<double> editTimes (xToEditTime (bounds.getX()),
                                       xToEditTime (bounds.getRight()));
        
        times = (editTimes + (numLoops * loopRange.getLength())) - timeStarted;
    }
    
    return hasLooped;
}

void RecordingClipComponent::timerCallback()
{
    updatePosition();
}

void RecordingClipComponent::updatePosition()
{
    auto& edit = track->edit;
    
    if (auto playhead = edit.getTransport().getCurrentPlayhead())
    {
        double t1 = punchInTime >= 0 ? punchInTime : edit.getTransport().getTimeWhenStarted();
        double t2 = jmax (t1, playhead->getUnloopedPosition());
        
        if (playhead->isLooping())
        {
            auto loopTimes = playhead->getLoopTimes();
            
            if (t2 >= loopTimes.end)
            {
                t1 = jmin (t1, loopTimes.start);
                t2 = loopTimes.end;
            }
        }
        else if (edit.recordingPunchInOut)
        {
            auto mr = edit.getTransport().getLoopRange();
            auto in  = mr.getStart();
            auto out = mr.getEnd();
            
            t1 = jlimit (in, out, t1);
            t2 = jlimit (in, out, t2);
        }
        
        t1 = jmax (t1, editViewState.viewX1.get());
        t2 = jmin (t2, editViewState.viewX2.get());
    
        if (auto p = getParentComponent())
        {
            int x1 = editViewState.beatsToX (t1, p->getWidth());
            int x2 = editViewState.beatsToX (t2, p->getWidth());
            
            setBounds (x1, 0, x2 - x1, p->getHeight());
            return;
        }
    }
    
    setBounds ({});
}

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_armButton,
                                         &m_muteButton,
                                         &m_soloButton
                                         });
    

    m_trackName.setText(m_track->getName(), NotificationType::dontSendNotification);
    m_trackName.setColour(Label::textColourId, Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);

    if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        m_armButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        };

        m_volumeKnob.setOpaque(false);
        addAndMakeVisible(m_volumeKnob);
        m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
        m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob.getValueObject().referTo(audioTrack->getVolumePlugin()->volume.getPropertyAsValue());
            m_volumeKnob.setValue(audioTrack->getVolumePlugin()->volume);

        }
        m_volumeKnob.setSliderStyle(Slider::RotaryVerticalDrag);
        m_volumeKnob.setTextBoxStyle(Slider::NoTextBox, 0, 0, false);
        addAndMakeVisible(m_peakDisplay);
    }
    else
    {
        m_armButton.setVisible (false);
        m_muteButton.setVisible (false);
        m_soloButton.setVisible (false);
    }
    
    m_track->state.addListener (this);
    inputsState = m_track->edit.state.getChildWithName (te::IDs::INPUTDEVICES);
    inputsState.addListener (this);
    
    valueTreePropertyChanged (m_track->state, te::IDs::mute);
    valueTreePropertyChanged (m_track->state, te::IDs::solo);
    valueTreePropertyChanged (inputsState, te::IDs::targetIndex);
}

TrackHeaderComponent::~TrackHeaderComponent()
{
    m_track->state.removeListener (this);
}

void TrackHeaderComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::TrackList::isTrack (v))
    {
        if (i == te::IDs::mute)
            m_muteButton.setToggleState ((bool)v[i], dontSendNotification);
        else if (i == te::IDs::solo)
            m_soloButton.setToggleState ((bool)v[i], dontSendNotification);
    }
    else if (v.hasType (te::IDs::INPUTDEVICES)
             || v.hasType (te::IDs::INPUTDEVICE)
             || v.hasType (te::IDs::INPUTDEVICEDESTINATION))
    {
        if (auto at = dynamic_cast<te::AudioTrack*> (m_track.get()))
        {
            m_armButton.setEnabled (EngineHelpers::trackHasInput (*at));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*at), dontSendNotification);
        }
    }
}

void TrackHeaderComponent::paint (Graphics& g)
{
    Rectangle<float> area = getLocalBounds().toFloat();

        g.setColour(m_track->getColour());
        Rectangle<float> trackColorIndicator = area.removeFromLeft(18);
        g.fillRect(trackColorIndicator);
        g.setColour(Colour(0xff343434));
        g.drawRect(trackColorIndicator);
        g.drawRect(area);
        area.reduce(1, 1);
        if (editViewState.selectionManager.isSelected(m_track))
        {
            g.setColour(Colour(0xff383838));
        }
        else
        {
            g.setColour(Colour(0xff181818));
        }

        g.fillRect(area);
}

void TrackHeaderComponent::mouseDown (const MouseEvent& event)
{                       

    if (!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown ())
            {
                if (auto at = dynamic_cast<te::AudioTrack*>(m_track.get()))
                {
                    PopupMenu m;
                    m.addItem(2000, "delete Track");
                    m.addSeparator();

                    if (EngineHelpers::trackHasInput(*at))
                    {
                        bool ticked = EngineHelpers::isInputMonitoringEnabled(*at);
                        m.addItem(1000, "Input Monitoring", true, ticked);
                        m.addSeparator();
                    }

                    int id = 1;
                    for (auto instance: at->edit.getAllInputDevices())
                    {
                        if (instance->getInputDevice().getDeviceType()
                            == te::InputDevice::waveDevice)
                        {
                            bool ticked = instance->getTargetTracks().getFirst() == at;
                            m.addItem(id++,
                                      instance->getInputDevice().getName(),
                                      true,
                                      ticked);
                        }
                    }

                    m.addSeparator();

                    id = 100;

                    at->edit.playInStopEnabled = true;
                    auto& dm = at->edit.engine.getDeviceManager();
                    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
                    {
                        if (auto wip = dm.getMidiInDevice(i))
                        {
                            wip->setEndToEndEnabled(true);
                            wip->setEnabled(true);
                        }
                    }
                    at->edit.restartPlayback();

                    for (auto instance: at->edit.getAllInputDevices())
                    {
                        if (instance->getInputDevice().getDeviceType()
                            == te::InputDevice::physicalMidiDevice)
                        {
                            bool ticked = instance->getTargetTracks().getFirst() == at;
                            m.addItem(id++,
                                      instance->getInputDevice().getName(),
                                      true,
                                      ticked);
                        }
                    }

                    const int res = m.show();

                    if (res == 2000)
                    {
                        m_track->deselect();
                        m_track->edit.deleteTrack(m_track);
                        auto i = tracktion_engine::getAllTracks(editViewState.edit).getLast();
                        
                        if (!(i->isArrangerTrack()
                            || i->isTempoTrack()
                            || i->isMarkerTrack()
                            || i->isChordTrack()))
                        {
                            editViewState.selectionManager.selectOnly(i);
                        }
                        else
                        {
                            editViewState.selectionManager.deselectAll();
                        }
                    }
                    else if (res == 1000)
                    {
                        EngineHelpers::enableInputMonitoring(
                            *at, !EngineHelpers::isInputMonitoringEnabled(*at));
                    }
                    else if (res >= 100)
                    {
                        int id = 100;
                        
                        for (auto instance: at->edit.getAllInputDevices())
                        {
                            if (instance->getInputDevice().getDeviceType()
                                == te::InputDevice::physicalMidiDevice)
                            {
                                if (id == res)
                                {
                                    if (instance->getTargetTracks().getFirst() == at)
                                    {
                                        instance->removeTargetTrack(*at);
                                    }
                                    else
                                    {
                                        instance->setTargetTrack(*at, 0, true);
                                    }
                                }
                                id++;
                            }
                        }
                    }
                    else if (res >= 1)
                    {
                        int id = 1;
                        for (auto instance: at->edit.getAllInputDevices())
                        {
                            if (instance->getInputDevice().getDeviceType()
                                == te::InputDevice::waveDevice)
                            {
                                if (id == res)
                                {
                                    if (instance->getTargetTracks().getFirst() == at)
                                    {
                                        instance->removeTargetTrack(*at);
                                    }
                                    else
                                    {
                                        instance->setTargetTrack(*at, 0, true);
                                    }
                                }
                                id++;
                            }
                        }
                    }



                }
            }
            else if (event.mods.isShiftDown())
            {
                if (editViewState.selectionManager.getNumObjectsSelected())
                {
                    editViewState.selectionManager.addToSelection(m_track);
                }
            }
            else
            {
                editViewState.selectionManager.selectOnly(m_track);
            }
        }
}

void TrackHeaderComponent::resized()
{
    auto area = getLocalBounds();
    auto peakDisplay = area.removeFromRight(20);
    peakDisplay.reduce(2, 2);
    m_peakDisplay.setBounds(peakDisplay);
    auto volSlider = area.removeFromRight(area.getHeight());
    m_volumeKnob.setBounds(volSlider);

    auto buttonGroup = area.removeFromRight(area.getHeight());
    auto buttonwidth = buttonGroup.getWidth() / 2;
    auto buttonHeight = buttonGroup.getHeight() / 2;
    m_soloButton.setBounds(buttonGroup.getX(), buttonGroup.getY(), buttonwidth, buttonHeight);
    m_muteButton.setBounds(buttonGroup.getX(), buttonGroup.getY() + buttonHeight, buttonwidth, buttonHeight);
    m_armButton.setBounds(buttonGroup.getX() + buttonwidth, buttonGroup.getY(), buttonwidth, buttonHeight);

    area.removeFromLeft(20);
    m_trackName.setBounds(area);
}

//==============================================================================
PluginComponent::PluginComponent (EditViewState& evs, te::Plugin::Ptr p)
    : editViewState (evs), plugin (p)
{
    name.setText(plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
}

PluginComponent::~PluginComponent()
{
}
void PluginComponent::paint (Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(Colour(0xff242424));
    g.fillRect(area);
    if (plugin.getObject()->getOwnerTrack())
    {
        g.setColour(plugin.getObject()->getOwnerTrack()->getColour());
    }
    auto header = area.removeFromLeft(20);
    g.fillRect(header);
}
void PluginComponent::mouseDown (const MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        PopupMenu m;
        m.addItem ("Delete", [this] { plugin->deleteFromParent(); });
        m.show();
    }
    else
    {
        //std::cout << plugin->state.toXmlString() << std::endl;
        plugin->showWindowExplicitly();
        
    }
}

void PluginComponent::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX(), area.getHeight() - 20, area.getHeight(), 20);
    name.setBounds(nameLabelRect);
    name.setTransform(AffineTransform::rotation (-MathConstants<float>::halfPi, 
                                                nameLabelRect.getX() + 10.0 , 
                                                nameLabelRect.getY() + 10.0 ));
}
//==============================================================================
TrackFooterComponent::TrackFooterComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    addAndMakeVisible (addButton);
    
    buildPlugins();
    
    track->state.addListener (this);
    
    addButton.onClick = [this]
    {
        if (auto plugin = showMenuAndCreatePlugin (track->edit))
            track->pluginList.insertPlugin (plugin, 0, &editViewState.selectionManager);
        editViewState.selectionManager.selectOnly (track);
    };
}

TrackFooterComponent::~TrackFooterComponent()
{
    track->state.removeListener (this);
}

void TrackFooterComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void TrackFooterComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void TrackFooterComponent::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    markAndUpdate (updatePlugins);
}

void TrackFooterComponent::paint (Graphics& g)
{
    g.setColour (Colour(0x181818));
    g.fillRect (getLocalBounds().withTrimmedLeft (2));
}

void TrackFooterComponent::mouseDown (const MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void TrackFooterComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
   
    addButton.setBounds (area.removeFromLeft(15));

    
    for (auto p : plugins)
    {
        area.removeFromLeft (5);
        p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));
    }
}

void TrackFooterComponent::handleAsyncUpdate()
{
    if (compareAndReset (updatePlugins))
        buildPlugins();
}

void TrackFooterComponent::buildPlugins()
{
    plugins.clear();
    
    for (auto plugin : track->pluginList)
    {
        //Hier zum richtigen Plugin die richtige Componente laden.
        if(plugin->getPluginType() == "volume")
        {
            auto volumePlugin = dynamic_cast<tracktion_engine::VolumeAndPanPlugin*>(plugin);
            //auto vp = new VolumePluginComponent( editViewState, volumePlugin);
           
        }
        auto p = new PluginComponent (editViewState, plugin);
        addAndMakeVisible (p);
        plugins.add (p);
    }
    resized();
}

//==============================================================================
TrackComponent::TrackComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    editViewState.state.addListener (this);
    track->state.addListener(this);
    track->edit.getTransport().addChangeListener (this);

    markAndUpdate (updateClips);
}

TrackComponent::~TrackComponent()
{
    track->state.removeListener (this);
    editViewState.state.removeListener(this);
    track->edit.getTransport().removeChangeListener (this);
}

void TrackComponent::paint (Graphics& g)
{
    g.fillAll ();
    g.setColour(Colour(0xff111111));
    g.drawRect(0,0, getWidth(), getHeight() );
    double x2 = editViewState.viewX2;
    double x1 = editViewState.viewX1;
    g.setColour(Colour(0xff333333));
    double zoom = x2 -x1;
    int firstBeat = static_cast<int>(x1);
    if(editViewState.beatsToX(firstBeat,getWidth()) < 0)
    {
        firstBeat++;
    }

    if (editViewState.selectionManager.isSelected (track.get()))
    {
        g.setColour (Colour(0xff111111));

        auto rc = getLocalBounds();
        if (editViewState.showHeaders) rc = rc.withTrimmedLeft (-4);
        if (editViewState.showFooters) rc = rc.withTrimmedRight (-4);

        g.fillRect (rc);
        g.setColour(Colour(0xff333333));
    }

    auto pixelPerBeat = getWidth() / zoom;
    //std::cout << zoom << std::endl;
    for (int beat = firstBeat - 1; beat <= editViewState.viewX2; beat++)
    {
        int BeatX = editViewState.beatsToX(beat, getWidth());

        auto zBars = 16;

        if (zoom < 240)
        {
            zBars /= 2;
        }
        if (zoom < 120)
        {
            zBars /=2;
        }
        if (beat % zBars == 0)
        {
            g.drawLine(BeatX, 0, BeatX, getHeight());
        }

        if (zoom < 60)
        {
            g.drawLine(BeatX,0, BeatX, getHeight());
        }
        if (zoom < 25)
        {
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5)
            {
                g.drawLine(BeatX + quarterBeat * i ,0,
                           BeatX + quarterBeat * i ,getHeight());
                i++;
            }
        }


    }
}

void TrackComponent::mouseDown (const MouseEvent&event)
{
    editViewState.selectionManager.selectOnly (track.get()); 
    if (event.mods.isRightButtonDown())
    {
        PopupMenu m;
        m.addItem(1, "Paste");


        const int result = m.show();

        if (result == 0)
        {
            // user dismissed the menu without picking anything
        }
        else if(result == 1)
        {
           auto ip = te::EditInsertPoint(editViewState.edit);
           ip.setNextInsertPoint(editViewState.beatToTime(editViewState.xToBeats(event.x, getWidth()))  ,track);
           te::Clipboard::getInstance()->getContentWithType<te::Clipboard::Clips>()->pasteIntoEdit(te::Clipboard::ContentType::EditPastingOptions (editViewState.edit, ip));
        }
    }

}

void TrackComponent::changeListenerCallback (ChangeBroadcaster*)
{
    markAndUpdate (updateRecordClips);
}

void TrackComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::Clip::isClipState (v))
    {
        if (i == te::IDs::start
            || i == te::IDs::length)
        {
            markAndUpdate (updatePositions);
        }
    }
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            repaint();
            markAndUpdate (updatePositions);
        }
    }
    if(i.toString() == "bpm")
    {
         markAndUpdate(updateClips);
    }
}

void TrackComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::Clip::isClipState (c))
        markAndUpdate (updateClips);
}

void TrackComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::Clip::isClipState (c))
        markAndUpdate (updateClips);
}

void TrackComponent::valueTreeChildOrderChanged (juce::ValueTree& v, int a, int b)
{
    if (te::Clip::isClipState (v.getChild (a)))
        markAndUpdate (updatePositions);
    else if (te::Clip::isClipState (v.getChild (b)))
        markAndUpdate (updatePositions);
}

void TrackComponent::handleAsyncUpdate()
{
    if (compareAndReset (updateClips))
        buildClips();
    if (compareAndReset (updatePositions))
    {
        resized();
        for (auto &cc : clips)
        {
            cc->resized ();
        }
    }

    if (compareAndReset (updateRecordClips))
        buildRecordClips();
}

void TrackComponent::resized()
{
    for (auto cc : clips)
    {
        auto& c = cc->getClip();
        auto pos = c.getPosition();
        int x1 = editViewState.beatsToX (editViewState.edit.tempoSequence.timeToBeats(pos.getStart()), getWidth());
        int x2 = editViewState.beatsToX (editViewState.edit.tempoSequence.timeToBeats(pos.getEnd()), getWidth());
        
        cc->setBounds (x1, 0, x2 - x1, getHeight());
    }
}

void TrackComponent::itemDropped(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{

    auto dropPos = dragSourceDetails.localPosition;
    auto dropTime = editViewState.beatToTime(editViewState.xToBeats(dropPos.getX(), getWidth()));

    if (dragSourceDetails.description == "Clip")
        {
            auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
            if (clipComp)
            {
                clipComp->getClip().moveToTrack(*track);
            }
    }

    auto fileTreeComp = dynamic_cast<FileTreeComponent*>(dragSourceDetails.sourceComponent.get());
    if (fileTreeComp)
    {
        auto f = fileTreeComp->getSelectedFile();
        tracktion_engine::AudioFile audioFile(editViewState.edit.engine, f);
        if (audioFile.isValid())
        {
            if (auto audioTrack = dynamic_cast<tracktion_engine::AudioTrack*>(track.get()))
            {
                if (auto newClip = audioTrack->insertWaveClip(f.getFileNameWithoutExtension()
                                                         ,f
                                                         ,{ { dropTime, dropTime + audioFile.getLength() }, 0.0 }
                                                         , false))
                {
                    newClip->setColour(track->getColour());
                }
            }

        }
    }
}

void TrackComponent::itemDragMove(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "Clip")
        {
            auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
            if (clipComp)
            {
                addAndMakeVisible( clipComp);
            }
    }

}



void TrackComponent::buildClips()
{
    clips.clear();
    
    if (auto ct = dynamic_cast<te::ClipTrack*> (track.get()))
    {
        for (auto c : ct->getClips())
        {
            ClipComponent* cc = nullptr;
            
            if (dynamic_cast<te::WaveAudioClip*> (c))
                cc = new AudioClipComponent (editViewState, c);
            else if (dynamic_cast<te::MidiClip*> (c))
                cc = new MidiClipComponent (editViewState, c);
            else
                cc = new ClipComponent (editViewState, c);
            
            clips.add (cc);
            addAndMakeVisible (cc);
        }
    }
    
    resized();
}

void TrackComponent::buildRecordClips()
{
    bool needed = false;
    if (track->edit.getTransport().isRecording())
    {
        for (auto in : track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive() && track == *(in->getTargetTracks().getFirst()))
            {
                needed = true;
                break;
            }
        }
    }
    
    if (needed)
    {
        recordingClip = std::make_unique<RecordingClipComponent> (track, editViewState);
        addAndMakeVisible (*recordingClip);
    }
    else
    {
        recordingClip = nullptr;
    }
}

//==============================================================================
PlayheadComponent::PlayheadComponent (te::Edit& e , EditViewState& evs)
    : edit (e), editViewState (evs)
{
    startTimerHz (30);
}

void PlayheadComponent::paint (Graphics& g)
{
    g.setColour (Colours::yellow);
    g.drawRect (xPosition, 0, 2, getHeight());
}

bool PlayheadComponent::hitTest (int x, int)
{
    if (std::abs (x - xPosition) <= 3)
        return true;
    
    return false;
}

void PlayheadComponent::mouseDown (const MouseEvent&)
{
    //edit.getTransport().setUserDragging (true);
}

void PlayheadComponent::mouseUp (const MouseEvent&)
{
    edit.getTransport().setUserDragging (false);
}

void PlayheadComponent::mouseDrag (const MouseEvent& e)
{
    double t = editViewState.beatToTime(editViewState.xToBeats (e.x, getWidth()));
    edit.getTransport().setCurrentPosition (t);
    timerCallback();
}

void PlayheadComponent::timerCallback()
{
    if (firstTimer)
    {
        // On Linux, don't set the mouse cursor until after the Component has appeared
        firstTimer = false;
        setMouseCursor (MouseCursor::LeftRightResizeCursor);
    }

    int newX = editViewState.beatsToX (edit.tempoSequence.timeToBeats(edit.getTransport().getCurrentPosition()) , getWidth());
    if (newX != xPosition)
    {
        repaint (jmin (newX, xPosition) - 1, 0, jmax (newX, xPosition) - jmin (newX, xPosition) + 3, getHeight());
        xPosition = newX;
    }
}

//==============================================================================
EditComponent::EditComponent (te::Edit& e, te::SelectionManager& sm)
    : edit (e), editViewState (e, sm)
{
    edit.state.addListener (this);
    editViewState.selectionManager.addChangeListener (this);

    
    addAndMakeVisible (playhead);
    addAndMakeVisible (timeLine);
    addAndMakeVisible (pluginRack);
    pluginRack.setAlwaysOnTop(true);
    
    markAndUpdate (updateTracks);
    editViewState.selectionManager.selectOnly (te::getAllTracks (edit).getLast ());
    // auto& dm = edit.engine.getDeviceManager();
    // dm.getMidiInDevice(0)->getName();
}

EditComponent::~EditComponent()
{
    editViewState.selectionManager.removeChangeListener (this);
    edit.state.removeListener (this);
}

void EditComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            markAndUpdate (updateZoom);
        }
        else if (i == IDs::showHeaders
                 || i == IDs::showFooters)
        {
            markAndUpdate (updateZoom);
        }
        else if (i == IDs::drawWaveforms)
        {
            repaint();
        }
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::TrackList::isTrack (c))
        markAndUpdate (updateTracks);
}

void EditComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::TrackList::isTrack (c))
        markAndUpdate (updateTracks);
}

void EditComponent::valueTreeChildOrderChanged (juce::ValueTree& v, int a, int b)
{
    if (te::TrackList::isTrack (v.getChild (a)))
        markAndUpdate (updateTracks);
    else if (te::TrackList::isTrack (v.getChild (b)))
        markAndUpdate (updateTracks);
}

void EditComponent::mouseDown(const MouseEvent &event)
{
    if (event.mods.isPopupMenu())
    {
//        std::cout << "rechts! " << std::endl;
        PopupMenu m;
        m.addItem (10, "Add track");

        m.addSeparator();

        const int res = m.show();

        if (res == 10)
        {
            auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
            auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
            auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
            if (auto track = EngineHelpers::getOrInsertAudioTrackAt (edit, tracktion_engine::getAudioTracks(edit).size()))
            {

                 track->setName("Track " + String(tracktion_engine::getAudioTracks(edit).size()));
                 track->setColour(Colour(red, gre, blu));
                 editViewState.selectionManager.selectOnly(track);


            }
        }
    }
    //editViewState.selectionManager.deselectAll();
}


void EditComponent::paint(Graphics &g)
{
    auto rect = getLocalBounds();
    g.setColour(Colour(0xff181818));
    g.fillRect(rect);

    g.setColour(Colours::white);
    g.drawRect(editViewState.headerWidth, 0, 1, getHeight());
}

void EditComponent::handleAsyncUpdate()
{
    if (compareAndReset (updateTracks))
        buildTracks();
    if (compareAndReset (updateZoom))
    {
        resized();
        timeLine.repaint ();
    }
}

void EditComponent::resized()
{
    jassert (headers.size() == tracks.size());
    
    const int timelineHeight = 50;
    const int trackHeight = editViewState.headerHeight, trackGap = 0;
    const int headerWidth = editViewState.showHeaders ? editViewState.headerWidth : 0;
    const int footerWidth = editViewState.showFooters ? 150 : 0;
    const int pluginRackHeight = 250;

    auto area = getLocalBounds();
    auto pluginRackRect = area.removeFromBottom(pluginRackHeight);
    playhead.setBounds (area.withTrimmedLeft (headerWidth).withTrimmedRight (footerWidth));
    
    timeLine.setBounds(playhead.getBounds().removeFromTop(timelineHeight));
    int y = roundToInt (editViewState.viewY.get()) + timelineHeight;
    for (int i = 0; i < jmin (headers.size(), tracks.size()); i++)
    {
        auto h = headers[i];
        auto t = tracks[i];
        auto f = footers[i];
        
        h->setBounds (0, y, headerWidth, trackHeight);
        t->setBounds (headerWidth, y, getWidth() - headerWidth - footerWidth, trackHeight);
        f->setBounds (getWidth() - footerWidth, y, footerWidth, trackHeight);
        
        y += trackHeight + trackGap;
    }
    
    for (auto t : tracks)
        t->resized();
    
    
    pluginRack.setBounds (pluginRackRect);
    //pluginRack.toFront(false);
}

void EditComponent::buildTracks()
{
    tracks.clear();
    headers.clear();
    footers.clear();
    
    for (auto t : getAllTracks (edit))
    {
        TrackComponent* c = nullptr;
        
        if (t->isTempoTrack())
        {
            if (editViewState.showGlobalTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isMarkerTrack())
        {
            if (editViewState.showMarkerTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isChordTrack())
        {
            if (editViewState.showChordTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isArrangerTrack())
        {
            if (editViewState.showArrangerTrack)
                c = new TrackComponent (editViewState, t);
        }
        else
        {
            c = new TrackComponent (editViewState, t);
        }
        
        if (c != nullptr)
        {
            tracks.add (c);
            addAndMakeVisible (c);
            
            auto h = new TrackHeaderComponent (editViewState, t);
            headers.add (h);
            addAndMakeVisible (h);
            
            auto f = new TrackFooterComponent (editViewState, t);
            footers.add (f);
            addAndMakeVisible (f);
        }
    }
    
    playhead.toFront (false);
    resized();
}


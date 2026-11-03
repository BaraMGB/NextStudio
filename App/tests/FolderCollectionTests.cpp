#include "FolderCollection.h"
#include "FolderCollectionDrawing.h"
#include "ClipOverwriteCommand.h"
#include "SharedSelectionSnapshot.h"
#include "TrackInsertion.h"
#include <iostream>
#include <stdexcept>

namespace te = tracktion_engine;
namespace
{
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
auto time(double seconds) { return tracktion::TimePosition::fromSeconds(seconds); }
struct Fixture
{
    te::Engine engine{"NextStudioFolderCollectionTests"};
    std::unique_ptr<te::Edit> edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager selection{engine};
    te::FolderTrack::Ptr folder = edit->insertNewFolderTrack(TrackInsertion::atEndOfEdit(*edit), nullptr, true);
    te::AudioTrack::Ptr first = edit->insertNewAudioTrack(TrackInsertion::atEndOfFolder(*folder), nullptr);
    te::FolderTrack::Ptr nested = edit->insertNewFolderTrack(TrackInsertion::atEndOfFolder(*folder), nullptr, true);
    te::AudioTrack::Ptr second = edit->insertNewAudioTrack(TrackInsertion::atEndOfFolder(*nested), nullptr);
    te::MidiClip::Ptr a, b, c;
    Fixture()
    {
        first->state.setProperty(::IDs::isMidiTrack, true, nullptr);
        second->state.setProperty(::IDs::isMidiTrack, true, nullptr);
        a = first->insertMIDIClip("A", {time(2), time(6)}, nullptr);
        b = second->insertMIDIClip("B", {time(4), time(8)}, nullptr);
        c = first->insertMIDIClip("C", {time(8), time(10)}, nullptr);
        refresh();
        edit->getUndoManager().clearUndoHistory();
    }
    void refresh() { nested->generateCollectionClips(selection); folder->generateCollectionClips(selection); }
};

void aggregationAndSelection()
{
    Fixture f;
    require(f.folder->getNumCollectionClips() == 2, "overlaps merge, touching clips stay separate");
    auto* cc = f.folder->getCollectionClip(0);
    require(cc->getNumClips() == 2 && cc->range == tracktion::TimeRange(time(2), time(8)), "nested contents and union range");
    require(FolderCollection::valid(*cc), "valid folder collection");
    f.selection.addToSelection(cc);
    f.selection.addToSelection(f.a.get());
    f.selection.addToSelection(f.nested->getCollectionClip(0));
    require(FolderCollection::clips(f.selection).size() == 2, "deduplicate parent, child collection and explicit clip");
    FolderCollection::deselectContents(*f.nested->getCollectionClip(0), f.selection);
    require(FolderCollection::clips(f.selection) == juce::Array<te::Clip*>{f.a.get()}, "toggle child of selected parent without losing siblings");
    f.selection.selectOnly(cc);
    require(FolderCollection::selected(*cc, f.selection, FolderCollection::clips(f.selection)), "collection selection");
    f.selection.selectOnly(f.a.get());
    require(!FolderCollection::selected(*cc, f.selection, FolderCollection::clips(f.selection)), "partial selection not full highlight");
    f.selection.addToSelection(f.b.get());
    require(FolderCollection::selected(*cc, f.selection, FolderCollection::clips(f.selection)), "expanded selection highlighted");
    f.selection.selectOnly(cc);
    f.b->setStart(time(5), false, true);
    f.refresh();
    require(FolderCollection::clips(f.selection).size() == 3, "regeneration preserves selection when components merge");
    require(f.folder->getNumCollectionClips() == 1, "changed child propagates to ancestor");
}

void knifeSplitUndo()
{
    Fixture f;
    auto* collection = f.folder->getCollectionClip(0);
    require(FolderCollection::canSplitAt(*collection, time(5)), "overlapping collection can split");
    require(!FolderCollection::canSplitAt(*collection, time(8)), "clip boundaries are not split targets");
    f.edit->getUndoManager().beginNewTransaction("Split folder contents");
    const auto created = FolderCollection::splitAt(*collection, time(5));
    require(created.size() == 2, "knife splits every represented clip crossing the pointer");
    require(f.first->getClips().size() == 3 && f.second->getClips().size() == 2, "collection knife retains child tracks");
    require(f.edit->getUndoManager().undo(), "collection knife undo");
    require(f.first->getClips().size() == 2 && f.second->getClips().size() == 1, "single undo restores all split clips");
    require(!f.edit->getUndoManager().canUndo(), "collection knife is one transaction");
}

void guardAndCancellation()
{
    Fixture f;
    auto* original = f.folder->getCollectionClip(0);
    f.selection.selectOnly(original);
    SharedSelectionSnapshot snapshot;
    snapshot.capture(f.selection);
    {
        FolderCollection::DragGuard guard;
        guard.add(original);
        guard.add(original);
        f.folder->setDirtyClips();
        f.refresh();
        require(f.folder->contains(original) && original->isDragging(), "drag preserves virtual identity");
        require(guard.matches(), "unchanged gesture sources");
        f.selection.deselectAll();
        snapshot.restore(*f.edit, f.selection);
        require(f.selection.isSelected(original), "cancel restores pinned selection");
    }
    require(!original->isDragging(), "guard clears dragging");
    f.refresh();
    snapshot.restore(*f.edit, f.selection);
    require(FolderCollection::clips(f.selection).size() == 2, "cancel restores regenerated collection contents");
    require(!f.edit->getUndoManager().canUndo(), "selection and preview lifecycle create no undo");
    auto* current = f.folder->getCollectionClip(0);
    FolderCollection::DragGuard guard;
    guard.add(current);
    f.b->setStart(time(5), false, true);
    require(!guard.matches(), "competing timing edit invalidates gesture");
    f.b->removeFromParent();
    require(!FolderCollection::valid(*current), "removed child invalidates active gesture");
}

void collectionDrawing()
{
    namespace drawing = FolderCollectionDrawing;
    Fixture f;
    auto* cc = f.folder->getCollectionClip(0);
    const auto tracks = f.folder->getAllAudioSubTracks(true);
    const auto before = f.edit->state.createXml()->toString();
    // Built-in Dark/Light and a genuinely pale custom arrangement background.
    for (const auto theme : {std::pair{juce::Colour(0xff3a3a3a), juce::Colour(0xffe5cf03)},
                             std::pair{juce::Colour(0xff515f74), juce::Colour(0xff6e6521)},
                             std::pair{juce::Colour(0xffeeeeee), juce::Colour(0xffe5cf03)}})
    {
        for (auto folderColour : {juce::Colour(0xff812336), juce::Colours::black, juce::Colours::white})
        {
            const auto palette = drawing::palette(theme.first, folderColour, theme.second);
            require(drawing::contrast(palette.border, palette.body) >= 3.0, "collection frame/body contrast");
            require(drawing::contrast(palette.border, theme.first) >= 3.0, "collection frame/lane contrast");
            require(drawing::contrast(palette.accent, palette.body) >= 3.0, "selection/body contrast");
            require(drawing::contrast(palette.accent, theme.first) >= 3.0, "selection/lane contrast");
            require(drawing::contrast(palette.text, palette.body) >= 4.5, "caption contrast");
        }
        const auto render = [&](int shift, bool selected, bool hovered, float width = 120.0f, int extraRows = 0, float scale = 1.0f)
        {
            juce::Image image(juce::Image::RGB, int(200 * scale), int(40 * scale), true, juce::SoftwareImageType{});
            juce::Graphics g(image);
            g.fillAll(theme.first);
            g.addTransform(juce::AffineTransform::scale(scale));
            g.reduceClipRegion(juce::Rectangle<int>(5, 4, 180, 26));
            auto rows = tracks;
            for (int i = 0; i < extraRows; ++i) rows.add(nullptr);
            drawing::draw(g, *cc, rows, {20.0f + shift, 4.0f, width, 26.0f},
                [=](auto t) { return float(20.0 + shift + (t.inSeconds() - 2.0) * width / 6.0); },
                theme.first, theme.second, selected, hovered);
            return image;
        };
        const auto normal = render(0, false, false);
        const auto hover = render(0, false, true);
        const auto selected = render(0, true, false);
        const auto ghost = render(30, true, false);
        require(normal.getPixelAt(20, 14) != selected.getPixelAt(20, 14), "selection changes frame");
        require(normal.getPixelAt(40, 15) != hover.getPixelAt(40, 15), "hover changes body");
        require(selected.getPixelAt(40, 20) != selected.getPixelAt(120, 20), "first child strip retains its time span");
        require(selected.getPixelAt(40, 25) != selected.getPixelAt(120, 25), "nested child uses its own row");
        for (int y = 4; y < 30; ++y)
            for (int x = 20; x < 140; ++x)
                require(selected.getPixelAt(x, y) == ghost.getPixelAt(x + 30, y), "ghost translates caption and contents with frame");
        for (float scale : {1.0f, 1.25f, 1.5f, 2.0f})
            for (float width : {0.5f, 1.0f, 15.5f, 120.25f})
                for (int extraRows : {0, 30})
                {
                    const auto clipped = render(-30, true, false, width, extraRows, scale);
                    for (int y = 0; y < clipped.getHeight(); ++y)
                        for (int x = 0; x < clipped.getWidth(); ++x)
                            if (x < int(5 * scale) || y < int(4 * scale) || y >= int(std::ceil(30 * scale)))
                                require(clipped.getPixelAt(x, y) == theme.first, "narrow/dense/offscreen collection is lane-clipped at every scale");
                }
    }
    require(f.edit->state.createXml()->toString() == before, "collection painting must not modify the edit");
}

void copyMoveUndoPersistence()
{
    Fixture f;
    f.selection.selectOnly(f.folder->getCollectionClip(0));
    f.selection.addToSelection(f.a.get());
    const auto contents = FolderCollection::clips(f.selection);
    std::vector<ClipEditing::Placement> placements;
    for (auto* clip : contents)
    {
        auto position = clip->getPosition();
        position.time = position.time + tracktion::TimeDuration::fromSeconds(12);
        placements.push_back({ClipEditing::PlacementMode::copy, clip, {}, clip->getClipTrack(), position});
    }
    ClipEditing::Options options;
    options.undoName = "Copy folder contents";
    const auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, placements, options);
    require(result.succeeded && result.clips.size() == 2, "atomic collection copy");
    require(f.first->getClips().size() == 3 && f.second->getClips().size() == 2, "copied once to original child tracks");
    require(f.a->getPosition().getStart() == time(2) && f.b->getPosition().getStart() == time(4), "copy retains originals");
    require(f.edit->getUndoManager().undo(), "copy undo");
    require(f.first->getClips().size() == 2 && f.second->getClips().size() == 1, "single undo removes all copies");
    require(!f.edit->getUndoManager().canUndo(), "one transaction");
    require(f.edit->getUndoManager().redo(), "copy redo");
    f.refresh();
    require(f.folder->getNumCollectionClips() == 3, "copy appears in folder");
    for (auto& p : placements) { p.mode = ClipEditing::PlacementMode::move; p.finalPosition.time = p.finalPosition.time + tracktion::TimeDuration::fromSeconds(12); }
    require(ClipEditing::applyOverwrite(*f.edit, f.selection, placements, options).succeeded, "collection move");
    require(f.a->getPosition().getStart() == time(26) && f.b->getPosition().getStart() == time(28), "move relative timing");
    require(f.a->getClipTrack() == f.first.get() && f.b->getClipTrack() == f.second.get(), "move preserves hierarchy");
    require(f.edit->getUndoManager().undo(), "move undo");
    require(f.a->getPosition().getStart() == time(2) && f.b->getPosition().getStart() == time(4), "move undo restores group");
    auto loaded = te::Edit::createEditForExamining(f.engine, juce::ValueTree::fromXml(*f.edit->state.createXml()));
    auto* folder = dynamic_cast<te::FolderTrack*>(te::findTrackForID(*loaded, f.folder->itemID));
    te::SelectionManager loadedSelection(f.engine);
    folder->generateCollectionClips(loadedSelection);
    require(folder->getNumCollectionClips() == 3, "collections reconstruct from saved child state");
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        aggregationAndSelection(); knifeSplitUndo(); guardAndCancellation(); copyMoveUndoPersistence(); collectionDrawing();
        std::cout << "Folder collection tests passed\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

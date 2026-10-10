# Documentation reorganization plan

- Type: historical record
- Audience: contributors and maintainer
- Status: implemented and accepted; maintainer authorized commit/push after review fixes.
- Current reference: [Documentation maintenance schema](../../development/documentation-policy.md)

The navigation/timeline pilot was accepted with “sieht gut aus” before the remaining migration. The batch descriptions below retain the migration's original execution context.

## Progress and review point

- [x] Structural inventory and per-document [migration map](documentation-migration-map.md).
- [x] Task-oriented navigation, adopted maintenance schema and extracted contribution workflow.
- [x] Timeline pilot: current MIDI/Song cursor contracts, shared snap/feedback/view references, seven archived records and compatibility redirects.
- [x] `Todo.md` reduced to open work, with original workflow/history preserved in the guide and Git.
- [x] Checked-in native cursor suites, tested link/index checker and independent documentation CI.
- [x] Maintainer review of navigation and timeline pilot.
- [x] Remaining eleven legacy records, numeric-property consolidation, Wine separation and focused non-pilot contract/boundary audits.
- [x] Final migration acceptance and archival of this plan/map after maintainer-authorized commit/push.

See the migration map for evidence/coverage boundaries. Pilot acceptance authorized continuing the agreed migration, not final acceptance or permission to commit/push.

## Goal

Make current behavior, implementation contracts, development procedures and historical decisions easy to distinguish and find. Reduce duplicated maintenance without losing design rationale, safety rules or validation limits. This is a documentation migration, not a software behavior change.

## Starting point

At planning time the repository had 65 Markdown files under `docs/`, including 18 change documents. `docs/README.md` already separates user, UI, plugin, component, architecture and development documentation. Keep this useful foundation rather than introducing a documentation framework or renaming every directory.

Specific problems to address:

- `docs/changes/` mixes completed plans, current implementation explanations and historical validation reports.
- `Todo.md` mixes an active queue, binding development rules and extensive completed work logs.
- Cursor, snapping and property-editing behavior is repeated across several documents.
- Some long documents reproduce internal implementation details instead of explaining durable contracts.
- Local `/tmp/` evidence and shared-machine paths are not portable, durable validation references.
- Historical "pending" statements can be mistaken for current status.
- Navigation is uneven: `components/timeline-snapping.md` is listed under Change documentation, and `changes/piano-roll-splitter-resize-fix.md` is not listed in the main index.

## Target responsibilities and future maintenance

The adopted [documentation maintenance schema](../../development/documentation-policy.md) defines authoritative locations, document types, minimal outlines, change/update rules, lifecycle, evidence handling and review responsibilities. It is the durable maintenance reference; this plan only defines the migration batches.

## Batch 1 — Inventory and migration map

- [x] Classify every starting document as current reference, procedure, active proposal, historical record or mixed content.
- [x] Record its audience, authoritative topic, related source/tests, overlaps and proposed destination in the migration map.
- [x] Identify internal incoming links and inspect GitHub issue bodies/#90 acceptance references before pilot moves. Preserve old paths because external link coverage is not exhaustive.
- [x] Verify pilot claims against source/tests and issue/approval status against GitHub; do not infer completion merely from a filename or old passing log.
- [x] Audit/extract the remaining mixed-record contracts against corresponding production sources/tests before migration. Untouched reference pages retain their existing scope; structural classification is not exhaustive certification of every sentence.
- [x] Identify unique knowledge requiring extraction from change records; all original change-record knowledge integrated, with destinations and retained limitations in the map.

Deliverable: a complete source-to-destination map with retain/merge/extract/archive actions. No content is removed in this batch.

## Batch 2 — Navigation and maintenance rules

- [x] Reorganize `docs/README.md` around entry tasks: use the application, look up a control/plugin, understand implementation, build/test/debug, consult history.
- [x] Keep the detailed current-document index, put timeline snapping under Components, and make historical records discoverable through one archive index.
- [x] Add a short `docs/development/contributing.md` with the existing mandatory implementation workflow extracted from `Todo.md`. Preserve approval, regression, build, delivery, maintainer acceptance and commit/push gates; keep public portable build defaults and project-local job-count instructions distinct.
- [x] Review and adopt `docs/development/documentation-policy.md` as the maintenance schema; link to it from the contribution guide rather than duplicating its rules.
- [x] Apply the schema's document types, minimal outlines, update matrix, lifecycle and review checklist to new/substantially revised pages. Do not require a metadata-only rewrite of every existing document.
- [x] Use existing architecture pages for design rationale. Create a separate decision record only when a substantial decision cannot be explained clearly there; do not require one for each patch.

Deliverable: clear entry points and one contribution/maintenance workflow. This plan remains visibly active until the remaining migration and final acceptance are complete.

## Batch 3 — Consolidate current knowledge, starting with the timeline

Work topic by topic, retaining necessary detail rather than applying arbitrary line limits.

- [x] Consolidate/read the timeline pilot: MIDI and Song cursors, snap profiles/feedback/replay, shared geometry/bands, and native validation. Remove the stale six-pixel user-guide claim and correct native-versus-software raster-test descriptions.
- [x] Obtain pilot review before applying this pattern to the remaining topics below.

| Topic | Authoritative destinations | Action |
|---|---|---|
| Piano Roll tools/cursors | `user/piano-roll.md`, `components/piano-roll-editor.md` | Keep operation instructions in the user guide and MIDI cursor/gesture contracts in the component reference; remove historical test-run details from current prose. |
| Song Editor cursors/tools | `ui/song-editor.md`; existing component reference if suitable, otherwise one scoped `components/song-editor.md` | Give Song Editor implementation its own appropriate home rather than embedding it only in the Piano Roll component document. |
| Snapping, preview geometry and live feedback | `components/timeline-snapping.md`, `components/timeline-view-transform.md` | Integrate surviving decisions from plans; link editor-specific pages to the shared contracts instead of repeating curve/coordinate rules. |
| Note/clip properties and numeric formats | `components/note-properties-bar.md`, `components/clip-properties-bar.md`, scoped shared `components/property-field-input.md` | Keep validation, coordinate, undo and focus-loss constraints. Replace method-by-method descriptions and duplicated shared parsing rules with focused source/shared-reference links. |
| Native and console validation | `development/testing.md`, `agent-debug.md`, reusable test scripts | Keep executable procedures and coverage boundaries, not delivery transcripts, in the current testing guide. |
| Wine/Bottles | `development/wine-bottles.md` | Separate supported procedure/current limitations from environment-specific historical observations; archive observations only after retaining useful rationale. |

- [x] Extract splitter/activation, MIDI lighting/controller, browser and built-in effect contracts before archiving records; separate their source/test ownership from user instructions.
- [x] Review affected user/UI/plugin boundaries, removing obsolete modal-project claims, user-facing internal layout constants/source details and historical rack-width comparisons. Peak Limiter remains indexed at its existing path.
- [x] Review consolidated topics for contradictions/limits. Preserve note/clip parser/planner differences, pitch-name mismatch, property versus lane velocity ranges, graph versus knob undo, batched input and platform coverage boundaries.

Deliverable: current usage and implementation can be understood without reading a completed plan or issue discussion.

## Batch 4 — Separate history and simplify the work queue

- [x] Move seven completed timeline pilot records to `docs/archive/changes/` using `git mv`, after preserving their current knowledge.
- [x] Extract and migrate all eleven remaining records after pilot acceptance, preserving original text/status and old-path redirects.
- [x] Add `docs/archive/README.md` with topic/issue links and explicit historical status. Preserve original validation scope and known snapshots; do not present past "pending" text as current project status.
- [x] Keep genuinely active proposals under `docs/plans/` and link them from their open issue/queue entry.
- [x] Update affected internal links and current-reference pointers across all migrated topics. For old paths linked from published issues or other external locations, retain a short redirect stub or update the external reference where possible; avoid retaining two full copies.
- [x] Remove completed step-by-step logs from `Todo.md` after extracting unique workflow/rationale. Git history retains the original text; GitHub issues hold issue status and acceptance. Keep only prioritized open work, blockers and links to active proposals.
- [x] Archive this plan and its migration map after final acceptance, leaving the maintenance schema current and preserving compatibility paths.

Deliverable: active work, current documentation and historical records have distinct locations and meanings.

## Batch 5 — Make validation references durable

- [x] Audit local-path, execution-time, checksum and screenshot references; current procedures use configurable/repository-relative paths, while archive headers/index label historical evidence availability.
- [x] Convert selected cursor working-area/transition checks into maintained tools with isolated settings, configurable binary/display/output/settling delay, and explicit fixed-layout prerequisites. Replace the HTTP/fixed-port bridge with private JSON lines; validate the converted scripts natively.
- [x] Resolve evidence handling without a bulk upload: record unknown/unverified historical availability and require issue/release/CI retention information for future noteworthy runs. Old local images/logs are not prerequisites or promised downloads; no new hosted validation/artifact is claimed.
- [x] Preserve historical reports/scope while explicitly separating accessible/reusable procedures from unavailable or unverified original artifacts.
- [x] Keep pilot environment, tested revision, procedure, result and untested scope concise. Avoid repeatedly propagating local delivery checksums and elapsed times into current reference pages.

Deliverable: current validation procedures run without the original agent's temporary directories; historical evidence availability is honest and explicit.

## Batch 6 — Lightweight checks and acceptance

- [x] Add a documentation check for local Markdown file links, supported heading anchors and index coverage. Cover root documentation and current pages; distinguish archive and redirect pages. Exclude external network checks and code-block examples from mandatory checks.
- [x] Test the checker on valid links, missing files/anchors, fragments, relative paths and intentional archive/redirect cases. Choose a small, maintained implementation rather than a documentation-site toolchain.
- [x] Integrate checks into CI with an independently runnable documentation job and appropriate path filters. Documentation-only changes need not rebuild the application; executable regression-tool changes receive their own relevant validation.
- [x] Manually verify representative reading paths: edit MIDI notes, find a Song Editor tool, understand snap ownership, run a native cursor regression, and find the rationale/validation history for #90. Each should have a clear entry from the documentation index, normally within two navigation steps.
- [x] Confirm no internal broken links or unintended unindexed current pages; review pilot contracts for contradictions and retained safety/coverage limits.
- [x] Complete focused contract/content review for the remaining consolidated topics; verify original record preservation and no application-source changes.
- [x] Obtain maintainer navigation/timeline pilot acceptance before the remaining migration.
- [x] Obtain final review of the implemented migration and fix both checker findings with regression coverage; hosted CI remains unrun until the authorized push.

Deliverable: a checked documentation tree plus maintainable rules preventing renewed duplication.

## Execution boundaries

1. Review and approve this plan before restructuring existing documentation.
2. Execute the inventory/navigation batches first, then one timeline consolidation pilot. Apply the agreed pattern to the remaining topics only after pilot review.
3. Use small, topic-scoped diffs. Separate content consolidation from mass moves where practical to keep review and history readable.
4. Do not change application behavior, close unrelated issues, discard unfinished work or infer broader validation from documentation acceptance.
5. Commit/push only on explicit request. No documentation website, generated API reference or new documentation dependency stack is required by this plan.

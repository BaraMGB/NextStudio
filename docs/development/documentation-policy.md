# Documentation maintenance schema

- Type: procedure
- Audience: contributors and reviewers
- Scope: documentation maintenance for repository changes
- Status: active; adopted with the approved reorganization

## Purpose

Define where knowledge belongs, when documentation must change, how documents are structured and when they become historical. These rules complement the implementation workflow; they do not replace maintainer approval, validation, delivery or commit/push gates.

This schema is the durable maintenance reference. The [archived reorganization plan](../archive/changes/documentation-reorganization-plan.md) describes the one-time migration. Existing documents adopt the schema as their topics are reviewed; no metadata-only rewrite of the entire tree is required.

## 1. Document types and authoritative locations

| Type/topic | Authoritative location | Content |
|---|---|---|
| User workflow | `docs/user/` | How to complete a task, observable results and relevant limitations |
| UI reference | `docs/ui/` | Controls, modes and screen-specific behavior; links to workflows |
| Plugin reference | `docs/plugins/` | Built-in instrument/effect behavior, controls and examples |
| Architecture reference | `docs/architecture/` | Ownership, boundaries, invariants, lifecycle and durable design rationale |
| Component reference | `docs/components/` | Subsystem contracts, integration, source/test links and implementation constraints |
| Development procedure | `docs/development/` | Repeatable build, contribution, test, debug and compatibility procedures |
| Active proposal | `docs/plans/` | Unimplemented scope, alternatives, approved approach and acceptance criteria |
| Historical record | `docs/archive/changes/` | Past plans, important decisions and validation snapshots, not current instructions |
| Release note | Root `CHANGELOG.md` | Concise user-visible changes grouped by release |
| Work queue | Root `Todo.md` | Prioritized open work and links to GitHub issues/proposals |

`docs/README.md` is the navigation index, not a second reference manual. Root `README.md` is the project entry and quick start. Existing `docs/agent-debug.md` and `docs/logging.md` remain authoritative until the migration explicitly decides their destinations; do not create parallel copies.

GitHub issues are authoritative for issue state, acceptance discussions and blockers. Git preserves implementation history. Neither belongs as a running transcript in current reference pages.

**One authoritative home per topic and audience.** User instructions and a technical explanation may coexist. Repeated detailed rules for the same audience should instead become links to the authoritative section. Shared contracts belong in a shared reference, not repeated in every consumer page.

## 2. Decide what to update for each change

| Change | Required documentation review | Additional record |
|---|---|---|
| User-visible behavior or control | Relevant user/UI/plugin page and user-facing changelog entry | No separate change report by default |
| Ownership, lifecycle, persistence, undo or integration contract | Relevant architecture/component reference; user page if observable | Explain significant rationale in that reference |
| Build/test/debug procedure | Relevant development procedure and reusable commands/scripts | No delivery transcript |
| Internal refactor with no documented contract change | Check source links and technical descriptions for invalidation | No artificial documentation entry |
| Substantial design choice or implementation needing advance approval | Issue and, where necessary, one scoped proposal | Archive the proposal after resolution and extract durable knowledge |
| Documentation-only correction | Authoritative page and affected links | No application changelog entry unless the correction itself warrants a release note |

Update only affected pages, not every documentation layer. Make documentation changes alongside the implementation that invalidates them. Review existing pages before creating new ones.

Create a new page only for a distinct audience/topic or a self-contained procedure that cannot be understood conveniently within an existing page. Do not create a page merely because an issue was closed. Do not split or shorten pages solely to meet a word/line quota.

## 3. Common writing rules

- Write repository documentation in English and use descriptive kebab-case filenames.
- Identify type, audience and scope near the top of new or substantially revised documents. Scope distinguishes current repository behavior, a proposal or a historical snapshot.
- Describe current reference behavior in the present tense. Keep unfinished ideas out of the current contract; link to their issue/proposal.
- Keep source paths and durable API/invariant explanations where useful. Prefer source/test links over copied method inventories or large source excerpts.
- Use repository-relative commands and paths. Label genuine platform/environment differences. Current procedures must not require the original agent's `/tmp/` directory or home folder.
- State limitations and untested cases explicitly. Distinguish a recommended test from a test actually performed.
- Do not add routine "last updated" dates, rolling test counts, elapsed times, binary hashes or approval quotations to current references. Git records edits; validation records identify tested snapshots.
- Keep internal links relative and update incoming links when moving or renaming pages. Preserve an external-facing old path with a short redirect when necessary, not a second complete document.
- Keep safety, ownership, undo, cancellation and platform constraints even when condensing text.

## 4. Minimal document shapes

Use the sections applicable to the topic; omit empty or irrelevant sections. The following are outlines, not requirements to produce boilerplate for a small page.

### Current reference

```markdown
# Topic
Type: reference
Audience: users | contributors
Scope: current repository behavior

## Purpose and boundaries
## Behavior or contract
## Usage or integration
## Limitations
## Related documentation, source and tests
```

For user/UI/plugin references, exclude C++ internals and focus on actions, controls and results. For component/architecture references, explain ownership, state/coordinate boundaries, event flow and invariants only where relevant. Link common behavior rather than copying it.

### Repeatable procedure

```markdown
# Procedure
Type: procedure
Audience: users | contributors
Scope: supported environment/task

## Prerequisites
## Steps and commands
## Expected result and verification
## Failure handling and limitations
## Related references
```

Commands should be runnable from a documented working directory. Explain isolation, destructive operations and cleanup where applicable. An environment-specific workaround must identify that environment.

### Active proposal

```markdown
# Proposed change
Type: proposal
Audience: contributors and maintainer
Status: draft | approved
Related issue: link, when applicable

## Problem and scope
## Constraints and non-goals
## Alternatives and proposed approach
## Implementation batches
## Acceptance criteria and validation
```

Approval applies to a defined scope. Substantial revisions require renewed review. Routine progress, feedback and approval discussion belong in the issue, not appended indefinitely to the proposal.

### Historical record

```markdown
# Historical topic
Type: historical record
Audience: contributors
Status: implemented | rejected | superseded
Related issue/commit: links where known
Current reference: link, if applicable

## Context and outcome
## Decision and important rationale
## Validation snapshot and limits
```

Identify the tested commit/environment and evidence location when available. Mark missing information as unknown; do not invent it. Historical records are not updated to describe every subsequent implementation.

## 5. Lifecycle

1. **Current reference/procedure:** maintained when related behavior or commands change. It describes the repository revision containing it, not a manually synchronized version number.
2. **Draft proposal:** describes unimplemented work and is linked from its issue/queue entry.
3. **Approved proposal:** records the agreed scope; approval does not imply implementation or validation.
4. **Resolved proposal:** mark implemented, rejected or superseded only after confirming the outcome. For an implementation, first transfer surviving behavior, rationale and constraints to current references.
5. **Archive:** move the resolved proposal/record into the historical archive, update links/indexes and identify the current reference. Keep unfinished proposals active.
6. **Obsolete current page:** merge useful information into its successor, update incoming links, and remove or redirect the obsolete page. Do not archive routine duplicate prose simply to preserve another copy; Git already retains it.

Historical records remain clearly separated from current documentation in navigation. When a later decision supersedes one, add a successor pointer without rewriting the old record as though it described the new outcome.

## 6. Validation and evidence

Current testing documentation owns repeatable procedures and coverage boundaries. Executable test cases belong in `App/tests/` or suitable reusable tools, not embedded only in an issue report or temporary workspace.

Record a noteworthy validation run in the issue or one historical record using: **tested commit, environment, procedure/script, result, untested scope, evidence availability**. Avoid duplicating that snapshot across the work queue, component reference and release notes.

Keep bulky logs/images outside normal reference pages. Use an appropriate issue attachment, release asset or CI artifact and state retention limits where relevant. Local paths can describe a historical run, but must not be the only prerequisite for a current procedure. If artifacts disappear, distinguish the reported historical outcome from independently reproducible evidence.

## 7. Responsibility and review checklist

The change author updates the affected authoritative documentation. The reviewer checks placement, accuracy, links and retained constraints; the maintainer approves product/design claims where required. No separate rotating documentation owner or project-wide rewrite is needed for each patch.

Before completion:

- [ ] Identify the authoritative topic/page and review whether the change invalidates it.
- [ ] Update affected behavior/procedure/source links, or state in the change review why no documentation update is needed.
- [ ] Remove new duplication; link shared rules instead of copying them.
- [ ] Separate current behavior, proposed work, historical results and known limitations.
- [ ] Update navigation for new/moved pages; check incoming links and necessary redirects.
- [ ] Add a concise changelog entry only when appropriate for users.
- [ ] Keep issue state/acceptance in GitHub and the work queue focused on open work.
- [ ] Check applicable commands, prerequisites and evidence claims. Documentation-only work does not require an application rebuild; executable changes still require their relevant validation.
- [ ] Run `python3 tools/check-docs.py` and its regression tests, and manually check the affected reading path.

[Contributing](contributing.md#documentation-only-changes-and-development-tools) documents the checker commands, supported syntax and independent documentation CI job. Extracted historical records are indexed separately in the archive. The accepted migration plan/map are [archived historical records](../archive/README.md#documentation-migration), not additional permanent maintenance manuals.

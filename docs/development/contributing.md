# Contributing to NextStudio

- Type: procedure
- Audience: contributors and maintainer
- Scope: repository changes and implementation acceptance

## Prerequisites

Read the [source layout](source-layout.md), relevant subsystem references and [documentation maintenance schema](documentation-policy.md). GitHub issues own issue status, discussions and acceptance; [Todo](../../Todo.md) is a prioritized open-work queue, not a second tracker.

## Mandatory workflow for application implementation batches

The following sequence is binding and repeats for every batch. It is not a global completion checklist. Track actual batch progress in the affected issue or active proposal; completed issues do not become incomplete when a new batch starts.

1. Refresh the current open issues and milestones from GitHub with `gh`; review the repository state, release state, changelog, source layout, and existing tests. Confirm release priority and dependencies.
2. Reproduce or verify each issue against current `main` before changing code.
3. Document the root-cause analysis and present a concrete solution proposal before implementation.
4. Obtain the maintainer's approval for the proposed solution; discuss and revise it where necessary. Do not implement before approval.
5. Add or update automated regression coverage where the behavior can be isolated; otherwise document why focused runtime or visual validation is appropriate.
6. Implement only the approved, smallest coherent change with undo, persistence, and platform behavior considered where applicable.
7. Update technical documentation under `docs/components/`, `docs/architecture/`, or `docs/development/` and user documentation under `docs/user/` or `docs/ui/` as appropriate. Update `CHANGELOG.md` for user-visible changes. Apply the maintenance schema rather than creating a report for every small fix.
8. Build with `BUILD_JOBS=12 ./build.sh rd` in the project-local agent environment.
9. Run `BUILD_JOBS=12 ./test.sh rd` there.
10. Perform focused UI/runtime validation, using the debug shell where practical.
11. Produce the shared test artifact with `BUILD_JOBS=12 ./build_and_copy_shared.sh` there and let the maintainer test the software. Repeat relevant validation and artifact steps after any resulting changes.
12. Receive maintainer validation and re-check the affected GitHub issue acceptance criteria before marking the item complete. Commit or push only when explicitly requested.

These job-count/shared-directory instructions preserve the project-local implementation workflow previously in `Todo.md`. Public [build instructions](building.md) retain the scripts' portable defaults; other machines choose an appropriate `BUILD_JOBS` count and delivery destination. Do not make current documentation depend on one contributor's home directory.

## Documentation-only changes and development tools

Documentation-only changes require review of affected reading paths, contracts and links, not an application rebuild or a fresh unchanged executable. For executable tooling changes, run the relevant tool regression tests and actual procedure where practical. Application code or build/runtime changes still follow the application validation/artifact gates above. Never infer software acceptance or expanded native/platform coverage from acceptance of a documentation edit.

From the repository root, with Python 3.10 or newer:

```bash
python3 -m unittest discover -s tools/tests -p 'test_check_docs.py'
python3 tools/check-docs.py
```

The checker validates local Markdown links, supported heading anchors and current-page reachability within two links from `docs/README.md`. It also checks historical/redirect links, but excludes historical pages and redirect stubs from current-index coverage. External URLs are not fetched. See its module docstring for supported Markdown syntax; it is not a complete Markdown renderer.

The dedicated [documentation workflow](../../.github/workflows/docs.yml) runs these checks without application dependencies or submodules. Build/release CI skips changes limited to Markdown and checker infrastructure; release tags and manual builds retain their build workflow. Mixed code/documentation changes still run the relevant build pipeline. If branch protection requires the Build workflow, its path-filter behavior must be considered when configuring required checks.

## Verification and evidence

Use [Testing](testing.md) for reproducible validation procedures and coverage boundaries. Record a noteworthy run once, in its issue or historical record, with tested revision, environment, procedure, result and untested scope. Temporary logs are not a durable substitute for a checked-in regression script.

## Failure handling

- A failed regression or broken link blocks completion until corrected or explicitly scoped as an existing limitation.
- Reconsider an approved proposal with the maintainer when scope or semantics materially change.
- Keep active proposals discoverable under [Plans](../plans/README.md); archive them only after resolution and extraction of durable knowledge.
- Keep project/global instruction files authoritative for the agent environment; this guide does not authorize commits or pushes.

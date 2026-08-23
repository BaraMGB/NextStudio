# Logging Guide

## Goal

Logging in NextStudio should primarily make the application's own behavior visible:

- GUI state
- user actions
- view state changes
- project/edit workflow
- plugin, file, and render actions
- relevant edit/engine context

The goal is **not** to fully mirror internal `tracktion_engine` details.

## Central API

Only use the central logger from:

- `App/include/Logging.h`

Macros:

- `NS_LOG_DEBUG(category, message)`
- `NS_LOG_INFO(category, message)`
- `NS_LOG_WARN(category, message)`
- `NS_LOG_ERROR(category, message)`

## Output and build behavior

The logger emits one line through `juce::Logger::writeToLog(...)` with this shape:

```text
<local timestamp> [<level>] [<category>] <message> (<source-file>:<line>)
```

The active JUCE logger decides the final sink. NextStudio does not install a permanent application-wide file logger; JUCE therefore uses its platform-default diagnostic output unless a diagnostic launcher supplies a custom sink. Logging must never be used as the debug-shell response transport; shell responses remain on stdout and diagnostic output should be routed separately by launchers.

`debug` messages are compiled in but filtered unless the build defines `DEBUG`, `_DEBUG`, or `DEBUG_OR_RELWITHDEBINFO`. The project defines `DEBUG_OR_RELWITHDEBINFO` for Debug and RelWithDebInfo builds. `info`, `warn`, and `error` are emitted in all build types.

## Categories

Currently available categories:

- `app`
- `ui`
- `viewstate`
- `selection`
- `workflow`
- `project`
- `edit`
- `plugins`
- `filesystem`
- `autosave`
- `transport`
- `engine`
- `setup`

## Ground Rules

### 1. Log app-first

Prefer logs that explain:

- what the user did
- which visible state changed
- which action NextStudio derived from it

### 2. Use engine information as context

Only log `engine` or `edit` information when it helps explain app or GUI behavior.

### 3. Write semantic messages

Bad:

- `"button"`
- `"play"`
- `"Liste changed"`

Better:

- `"browser path up button pressed"`
- `"play requested"`
- `"known plugin list changed; refreshing browser"`

### 4. Choose the right log level

- `debug`: technical flow details, frequent state changes, or large diagnostic state
- `info`: normal, important actions
- `warn`: unusual but tolerable situations
- `error`: failures or broken operations

Never emit complete edit XML, plugin state, credentials, environment contents, or user media data at `info` level. Prefer identifiers and compact summaries. Large state dumps belong in explicitly requested artifacts and may only be referenced by path in logs.

## What should be logged

- transport actions
- file and render operations
- preset and plugin actions
- selection and view state transitions
- recovery / autosave
- failed load operations
- unexpected but recoverable situations

## What should be avoided

- unstructured debug text
- repeated redundant logs without added value
- deep Tracktion internals without app relevance
- very frequent hot-path logs without real diagnostic value

## Examples

```cpp
NS_LOG_INFO(transport, "play requested");
NS_LOG_WARN(filesystem, "render aborted: output file already exists");
NS_LOG_ERROR(plugins, "failed to parse preset XML: " + presetFile.getFullPathName());
NS_LOG_DEBUG(viewstate, "rebuilding track height cache from edit state");
```

## Threading and performance

`log(...)` may be called from application worker threads, subject to the active JUCE logger's threading contract. Do not add logging to real-time audio callbacks or other hot paths: formatting allocates strings and the sink may block. Avoid high-frequency duplicate messages and never use logging as synchronization.

## Adding a category

Add the enum value in `App/include/Logging.h`, add its stable lowercase spelling in `Logging.cpp`, document it here, and add or update a focused test when logging tests are introduced. Existing category spellings are part of diagnostics consumed by users and tools and should not be renamed casually.

## Legacy code

Do not introduce again:

- `std::cout`
- `DBG(...)`
- direct `juce::Logger::writeToLog(...)` calls outside `Logging.cpp`
- new ad-hoc `GUIHelpers::log(...)` usage

## Note

`GUIHelpers::log(...)` only remains as a compatibility path for older code. New or refactored code should directly use the `NS_LOG_*` macros.

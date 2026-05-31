#!/usr/bin/env node

const { spawn, spawnSync } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const defaultBinaryPath = path.resolve(process.cwd(), 'autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio');
const debugShellSingleInstanceRejectionFile = path.resolve(os.tmpdir(), 'NextStudio', 'debug', 'launch-rejections', 'debug-shell-last-rejection.json');

function detectLikelySingleInstanceConflict(binaryPath) {
  if (process.platform === 'win32')
    return null;

  const processName = path.basename(binaryPath);
  const diagnostics = [];

  const pgrep = spawnSync('pgrep', ['-a', '-x', processName], { encoding: 'utf8' });
  if (!pgrep.error && pgrep.status !== 127) {
    const runningProcesses = pgrep.stdout
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter((line) => line.length > 0)
      .filter((line) => !line.includes('<defunct>'));

    if (runningProcesses.length > 0)
      return { processName, runningProcesses, diagnostics };

    diagnostics.push(`pgrep status=${String(pgrep.status)}`);
  } else if (pgrep.error) {
    diagnostics.push(`pgrep error=${pgrep.error.message}`);
  }

  const ps = spawnSync('ps', ['-A', '-o', 'pid=,comm='], { encoding: 'utf8' });
  if (!ps.error && ps.status === 0) {
    const runningProcesses = ps.stdout
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter((line) => line.length > 0)
      .filter((line) => line.split(/\s+/, 2)[1] === processName);

    if (runningProcesses.length > 0)
      return { processName, runningProcesses, diagnostics };
  } else if (ps.error) {
    diagnostics.push(`ps error=${ps.error.message}`);
  }

  return null;
}

function readSingleInstanceRejectionMarker() {
  if (!fs.existsSync(debugShellSingleInstanceRejectionFile))
    return null;

  try {
    return JSON.parse(fs.readFileSync(debugShellSingleInstanceRejectionFile, 'utf8'));
  } catch {
    return null;
  }
}

function isMatchingSingleInstanceRejectionMarker(marker, requestId, startTimeMs) {
  if (!marker)
    return false;
  if (marker.reason !== 'single-instance-conflict' || marker.requestId !== requestId)
    return false;

  const unixMs = Number(marker.unixMs);
  return Number.isFinite(unixMs) && unixMs >= startTimeMs - 5000;
}

async function waitForMatchingSingleInstanceRejectionMarker(requestId, startTimeMs, timeoutMs = 1500, pollIntervalMs = 50) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() <= deadline) {
    const marker = readSingleInstanceRejectionMarker();
    if (isMatchingSingleInstanceRejectionMarker(marker, requestId, startTimeMs))
      return marker;
    await new Promise((resolve) => setTimeout(resolve, pollIntervalMs));
  }

  return null;
}

function tokenizeResponseLine(line) {
  const tokens = [];
  const input = line.trim();
  let current = '';
  let inQuotes = false;

  for (let i = 0; i < input.length; ++i) {
    const ch = input[i];

    if (inQuotes && ch === '\\' && i + 1 < input.length) {
      current += ch;
      current += input[++i];
      continue;
    }

    if (ch === '"') {
      inQuotes = !inQuotes;
      current += ch;
      continue;
    }

    if (ch === ' ' && !inQuotes) {
      if (current.length > 0) {
        tokens.push(current);
        current = '';
      }
      continue;
    }

    current += ch;
  }

  if (current.length > 0)
    tokens.push(current);

  return tokens;
}

function parseResponseLine(line) {
  const result = { raw: line, fields: {} };
  const tokens = tokenizeResponseLine(line);

  result.status = tokens.shift() || '';

  for (const token of tokens) {
    const eq = token.indexOf('=');
    if (eq < 0) continue;
    const key = token.slice(0, eq);
    let value = token.slice(eq + 1);
    if (value.startsWith('"') && value.endsWith('"'))
      value = value.slice(1, -1).replace(/\\"/g, '"');
    result.fields[key] = value;
  }

  return result;
}

function requireOkResponse(response, context) {
  if (!response || !response.parsed)
    throw new Error(`${context}: missing parsed response`);
  if (response.parsed.status !== 'ok')
    throw new Error(`${context}: expected ok response, got ${response.responseLine || response.parsed.raw || '<none>'}`);
}

function requireErrorResponse(response, context) {
  if (!response || !response.parsed)
    throw new Error(`${context}: missing parsed response`);
  if (response.parsed.status !== 'error')
    throw new Error(`${context}: expected error response, got ${response.responseLine || response.parsed.raw || '<none>'}`);
}

function requireErrorCode(response, key, context) {
  requireErrorResponse(response, context);
  const actual = response.parsed.fields.code;
  if (actual !== key)
    throw new Error(`${context}: expected error code ${key}, got ${String(actual)}`);
}

function requireBooleanField(response, key, context) {
  const value = response?.parsed?.fields?.[key];
  if (value !== 'true' && value !== 'false')
    throw new Error(`${context}: expected boolean field ${key}, got ${String(value)}`);
  return value === 'true';
}

function requireNumberField(response, key, context) {
  const raw = response?.parsed?.fields?.[key];
  const value = Number(raw);
  if (!Number.isFinite(value))
    throw new Error(`${context}: expected numeric field ${key}, got ${String(raw)}`);
  return value;
}

function requireFileExists(filePath, context) {
  if (!filePath || !fs.existsSync(filePath))
    throw new Error(`${context}: file is missing: ${String(filePath)}`);
}

function readJsonFile(filePath, context) {
  requireFileExists(filePath, context);
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function createTempArtifactPath(prefix, extension) {
  const stamp = `${Date.now()}-${Math.random().toString(16).slice(2, 10)}`;
  return path.join(os.tmpdir(), `${prefix}-${stamp}${extension}`);
}

class NextStudioDebugShellClient {
  constructor(options = {}) {
    this.binaryPath = path.resolve(options.binaryPath || defaultBinaryPath);
    this.binaryArgs = Array.isArray(options.binaryArgs) ? [...options.binaryArgs] : ['--debug-shell'];
    this.injectDebugShellRequestId = !Array.isArray(options.binaryArgs);
    this.cwd = path.resolve(options.cwd || process.cwd());
    this.defaultTimeoutMs = options.timeoutMs || 10000;
    this.process = null;
    this.launchRequestId = null;
    this.lines = [];
    this.lineEntries = [];
    this.nextLineId = 1;
    this.waiters = new Set();
    this.exited = false;
    this.exitCode = null;
    this.exitSignal = null;
  }

  async start(timeoutMs = 30000) {
    if (!fs.existsSync(this.binaryPath))
      throw new Error(`NextStudio binary not found: ${this.binaryPath}`);

    const startTimeMs = Date.now();
    this.launchRequestId = this.injectDebugShellRequestId ? `${Date.now()}-${Math.random().toString(16).slice(2, 10)}` : null;
    const spawnArgs = this.launchRequestId
      ? [...this.binaryArgs, `--debug-shell-request-id=${this.launchRequestId}`]
      : this.binaryArgs;

    this.process = spawn(this.binaryPath, spawnArgs, {
      cwd: this.cwd,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    this._attachStream(this.process.stdout, false);
    this.process.on('error', (error) => {
      this.exited = true;
      this.exitCode = null;
      this.exitSignal = null;
      this._pushLine(`[process] error: ${error.message}`);
      this._rejectAllWaiters(error);
    });
    this.process.on('exit', (code, signal) => {
      this.exited = true;
      this.exitCode = code;
      this.exitSignal = signal;
      this._pushLine(`[process] exited code=${code} signal=${signal || 'none'}`);
      this._resolveAllWaitersWithNull();
    });
    this._attachStream(this.process.stderr, true);

    const readyLine = await this.waitForLine((line) => line.startsWith('ok code=ready'), timeoutMs);
    if (readyLine !== null)
      return readyLine;

    const rejectionMarker = this.launchRequestId
      ? await waitForMatchingSingleInstanceRejectionMarker(this.launchRequestId, startTimeMs)
      : null;
    if (rejectionMarker) {
      throw new Error('NextStudio debug shell exited before readiness was reported. Another NextStudio instance is already running and JUCE single-instance protection rejected the launch. Close the existing instance and retry.');
    }

    const possibleRunningProcesses = detectLikelySingleInstanceConflict(this.binaryPath);
    const suffix = possibleRunningProcesses
      ? ` Existing NextStudio processes were detected, but no explicit JUCE rejection marker matched this launch request: ${possibleRunningProcesses.runningProcesses.join(' | ')}`
      : '';
    throw new Error(`NextStudio debug shell exited before readiness was reported.${suffix}`);
  }

  async waitForSystemReady(timeoutMs = 30000, pollIntervalMs = 200) {
    const startedAt = Date.now();

    while (Date.now() - startedAt < timeoutMs) {
      const response = await this.command('system-state', Math.max(1000, timeoutMs - (Date.now() - startedAt)));
      if (response.parsed.fields.readyForPlayback === 'true')
        return response;
      await this.sleep(pollIntervalMs);
    }

    throw new Error(`Timed out after ${timeoutMs} ms waiting for system-state readyForPlayback=true`);
  }

  async command(commandLine, timeoutMs = this.defaultTimeoutMs) {
    if (!this.process || !this.process.stdin.writable)
      throw new Error('Debug shell process is not running');

    const startLineId = this.nextLineId;
    this.process.stdin.write(commandLine + '\n');
    const responseLine = await this.waitForLine((line) => line.startsWith('ok ') || line.startsWith('error '), timeoutMs, startLineId);

    if (responseLine === null)
      throw new Error(`No response received for command: ${commandLine}`);

    return {
      command: commandLine,
      responseLine,
      parsed: parseResponseLine(responseLine),
      newLines: this.getLinesSince(startLineId),
    };
  }

  async stop(force = false) {
    if (!this.process || this.exited)
      return;

    this.process.kill(force ? 'SIGKILL' : 'SIGTERM');
    await this.waitForExit(5000).catch(() => {});
  }

  waitForExit(timeoutMs = 5000) {
    if (this.exited)
      return Promise.resolve({ code: this.exitCode, signal: this.exitSignal });

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error(`Timed out after ${timeoutMs} ms waiting for process exit`)), timeoutMs);
      const interval = setInterval(() => {
        if (!this.exited)
          return;
        clearTimeout(timeout);
        clearInterval(interval);
        resolve({ code: this.exitCode, signal: this.exitSignal });
      }, 50);
    });
  }

  sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  copyFile(sourcePath, destinationPath) {
    fs.mkdirSync(path.dirname(destinationPath), { recursive: true });
    fs.copyFileSync(sourcePath, destinationPath);
    return destinationPath;
  }

  getLinesSince(minLineId) {
    return this.lineEntries
      .filter((entry) => entry.id >= minLineId)
      .map((entry) => entry.line);
  }

  getRecentLines(count = 20) {
    return this.lines.slice(-count);
  }

  waitForLine(matcher, timeoutMs, minLineId = 1) {
    for (let i = this.lineEntries.length - 1; i >= 0; --i) {
      const entry = this.lineEntries[i];
      if (entry.id < minLineId)
        break;
      if (matcher(entry.line, entry))
        return Promise.resolve(entry.line);
    }

    if (this.exited)
      return Promise.resolve(null);

    return new Promise((resolve, reject) => {
      const waiter = {
        matcher,
        minLineId,
        resolve,
        reject,
        timeout: setTimeout(() => {
          this.waiters.delete(waiter);
          reject(new Error(`Timed out after ${timeoutMs} ms while waiting for shell output`));
        }, timeoutMs),
      };
      this.waiters.add(waiter);
    });
  }

  _attachStream(stream, isStderr) {
    let pending = '';
    stream.on('data', (chunk) => {
      pending += chunk.toString();
      while (true) {
        const newlineIndex = pending.indexOf('\n');
        if (newlineIndex < 0)
          break;
        const rawLine = pending.slice(0, newlineIndex).replace(/\r$/, '');
        pending = pending.slice(newlineIndex + 1);
        this._pushLine(isStderr ? `[stderr] ${rawLine}` : rawLine);
      }
    });
  }

  _pushLine(line) {
    const entry = { id: this.nextLineId++, line };
    this.lineEntries.push(entry);
    this.lines.push(line);

    if (this.lineEntries.length > 500)
      this.lineEntries.splice(0, this.lineEntries.length - 500);
    if (this.lines.length > 500)
      this.lines.splice(0, this.lines.length - 500);

    for (const waiter of [...this.waiters]) {
      if (entry.id < waiter.minLineId || !waiter.matcher(line, entry))
        continue;
      clearTimeout(waiter.timeout);
      this.waiters.delete(waiter);
      waiter.resolve(line);
    }
  }

  _rejectAllWaiters(error) {
    for (const waiter of this.waiters) {
      clearTimeout(waiter.timeout);
      waiter.reject(error);
    }
    this.waiters.clear();
  }

  _resolveAllWaitersWithNull() {
    for (const waiter of this.waiters) {
      clearTimeout(waiter.timeout);
      waiter.resolve(null);
    }
    this.waiters.clear();
  }
}

async function runTransportSmokeTest(options = {}) {
  const client = new NextStudioDebugShellClient(options);
  const screenshotCopyPath = path.resolve(options.screenshotCopyPath || '/tmp/nextstudio-transport-smoke.png');
  const minPositionDeltaSeconds = options.minPositionDeltaSeconds ?? 0.25;

  try {
    const readyLine = await client.start(options.startTimeoutMs || 30000);
    const systemState = await client.waitForSystemReady(options.readyTimeoutMs || 30000);
    const before = await client.command('transport-state');
    const play = await client.command('play');
    await client.sleep(options.playDurationMs || 3000);
    const during = await client.command('transport-state');
    const screenshot = await client.command('screenshot');
    const screenshotPath = screenshot.parsed.fields.path;
    if (!screenshotPath)
      throw new Error('Screenshot command did not return a path');
    client.copyFile(screenshotPath, screenshotCopyPath);
    const stop = await client.command('stop');
    const after = await client.command('transport-state');
    const quit = await client.command('quit');
    await client.waitForExit(5000).catch(() => {});

    requireOkResponse(systemState, 'system-state');
    if (!requireBooleanField(systemState, 'readyForPlayback', 'system-state'))
      throw new Error('system-state: readyForPlayback did not become true');

    requireOkResponse(before, 'transport-state before play');
    requireOkResponse(play, 'play');
    requireOkResponse(during, 'transport-state during play');
    requireOkResponse(screenshot, 'screenshot');
    requireOkResponse(stop, 'stop');
    requireOkResponse(after, 'transport-state after stop');
    requireOkResponse(quit, 'quit');

    const beforePlaying = requireBooleanField(before, 'playing', 'transport-state before play');
    const duringPlaying = requireBooleanField(during, 'playing', 'transport-state during play');
    const afterPlaying = requireBooleanField(after, 'playing', 'transport-state after stop');
    const playAcknowledged = requireBooleanField(play, 'playing', 'play');
    const stopAcknowledged = requireBooleanField(stop, 'playing', 'stop');
    const quitAcknowledged = requireBooleanField(quit, 'quitting', 'quit');

    if (beforePlaying)
      throw new Error('transport-state before play unexpectedly reported playing=true');
    if (!playAcknowledged)
      throw new Error('play response did not acknowledge playing=true');
    if (!duringPlaying)
      throw new Error('transport-state during play did not report playing=true');
    if (stopAcknowledged)
      throw new Error('stop response unexpectedly reported playing=true');
    if (afterPlaying)
      throw new Error('transport-state after stop still reported playing=true');
    if (!quitAcknowledged)
      throw new Error('quit response did not acknowledge quitting=true');

    const beforePosition = requireNumberField(before, 'positionSeconds', 'transport-state before play');
    const duringPosition = requireNumberField(during, 'positionSeconds', 'transport-state during play');
    const afterPosition = requireNumberField(after, 'positionSeconds', 'transport-state after stop');

    if (duringPosition <= beforePosition + minPositionDeltaSeconds)
      throw new Error(`transport position did not advance enough during playback: before=${beforePosition}, during=${duringPosition}, minDelta=${minPositionDeltaSeconds}`);

    if (!fs.existsSync(screenshotCopyPath))
      throw new Error(`Copied screenshot is missing: ${screenshotCopyPath}`);

    if (client.exitCode !== null && client.exitCode !== 0)
      throw new Error(`Debug shell exited with non-zero code: ${client.exitCode}`);

    return {
      readyLine,
      systemState,
      before,
      play,
      during,
      screenshot,
      screenshotCopyPath,
      stop,
      after,
      quit,
      validation: {
        readyForPlayback: true,
        beforePlaying,
        duringPlaying,
        afterPlaying,
        beforePosition,
        duringPosition,
        afterPosition,
        minPositionDeltaSeconds,
        screenshotCopied: true,
        quitAcknowledged,
      },
      exitCode: client.exitCode,
      exitSignal: client.exitSignal,
      recentLines: client.getRecentLines(30),
    };
  } finally {
    await client.stop(true).catch(() => {});
  }
}

async function runCommandErrorSmokeTest(options = {}) {
  const client = new NextStudioDebugShellClient(options);

  try {
    const readyLine = await client.start(options.startTimeoutMs || 30000);
    const systemState = await client.waitForSystemReady(options.readyTimeoutMs || 30000);
    const screenshotZero = await client.command('screenshot 0');
    const screenshotNegative = await client.command('screenshot -1');
    const unknown = await client.command('definitely-not-a-command');
    const repeatedTransport = [];
    for (let i = 0; i < (options.repeatedTransportStateCount || 5); ++i)
      repeatedTransport.push(await client.command('transport-state'));
    const quit = await client.command('quit');
    await client.waitForExit(5000).catch(() => {});

    requireOkResponse(systemState, 'system-state');
    requireErrorCode(screenshotZero, 'invalid-argument', 'screenshot 0');
    requireErrorCode(screenshotNegative, 'invalid-argument', 'screenshot -1');
    requireErrorCode(unknown, 'unknown-command', 'unknown command');
    for (const [index, response] of repeatedTransport.entries())
      requireOkResponse(response, `transport-state repeat ${index + 1}`);
    requireOkResponse(quit, 'quit');

    if (client.exitCode !== null && client.exitCode !== 0)
      throw new Error(`Debug shell exited with non-zero code: ${client.exitCode}`);

    return {
      readyLine,
      systemState,
      screenshotZero,
      screenshotNegative,
      unknown,
      repeatedTransport,
      quit,
      exitCode: client.exitCode,
      exitSignal: client.exitSignal,
      recentLines: client.getRecentLines(30),
    };
  } finally {
    await client.stop(true).catch(() => {});
  }
}

async function runStateDumpSmokeTest(options = {}) {
  const client = new NextStudioDebugShellClient(options);
  const stateDumpCopyPath = path.resolve(options.stateDumpCopyPath || createTempArtifactPath('nextstudio-state-dump-smoke', '.json'));

  try {
    const readyLine = await client.start(options.startTimeoutMs || 30000);
    const systemState = await client.waitForSystemReady(options.readyTimeoutMs || 30000);
    const stateDump = await client.command('state-dump');
    requireOkResponse(systemState, 'system-state');
    requireOkResponse(stateDump, 'state-dump');

    const sourceStateDumpPath = stateDump.parsed.fields.path;
    client.copyFile(sourceStateDumpPath, stateDumpCopyPath);
    const state = readJsonFile(stateDumpCopyPath, 'state-dump');
    const tracks = Array.isArray(state?.edit?.tracks) ? state.edit.tracks : [];
    const selectedTracks = Array.isArray(state?.edit?.selection?.selectedTracks) ? state.edit.selection.selectedTracks : [];

    if (state?.application !== 'NextStudio')
      throw new Error(`state-dump: expected application=NextStudio, got ${String(state?.application)}`);

    const quit = await client.command('quit');
    requireOkResponse(quit, 'quit');
    await client.waitForExit(5000).catch(() => {});

    if (client.exitCode !== null && client.exitCode !== 0)
      throw new Error(`Debug shell exited with non-zero code: ${client.exitCode}`);

    return {
      readyLine,
      systemState,
      stateDump,
      stateDumpPath: stateDumpCopyPath,
      sourceStateDumpPath,
      stateSummary: {
        application: state?.application,
        version: state?.version,
        trackCount: tracks.length,
        selectedTrackCount: state?.edit?.selection?.selectedTrackCount ?? null,
        selectedTracks,
      },
      quit,
      exitCode: client.exitCode,
      exitSignal: client.exitSignal,
      recentLines: client.getRecentLines(30),
    };
  } finally {
    await client.stop(true).catch(() => {});
  }
}

function createFakeProtocolShell() {
  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'nextstudio-debug-shell-protocol-'));
  const scriptPath = path.join(tempDir, 'fake-debug-shell.js');
  const script = `#!/usr/bin/env node
let pending = '';
process.stdout.write('ok code=ready message="fake ready"\\n');
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => {
  pending += chunk;
  while (true) {
    const newlineIndex = pending.indexOf('\\n');
    if (newlineIndex < 0)
      break;
    const line = pending.slice(0, newlineIndex).replace(/\\r$/, '');
    pending = pending.slice(newlineIndex + 1);
    if (line === 'same') {
      process.stdout.write('ok code=ok stable=true\\n');
    } else if (line === 'hang-then-exit') {
      setTimeout(() => process.exit(7), 200);
    } else {
      process.stdout.write('error code=unknown-command message="Unknown command"\\n');
    }
  }
});
`;
  fs.writeFileSync(scriptPath, script);
  fs.chmodSync(scriptPath, 0o755);
  return { tempDir, scriptPath };
}

async function runClientProtocolRegressionTest(options = {}) {
  const { tempDir, scriptPath } = createFakeProtocolShell();
  const client = new NextStudioDebugShellClient({
    ...options,
    binaryPath: process.execPath,
    binaryArgs: [scriptPath],
    cwd: tempDir,
  });

  try {
    const readyLine = await client.start(options.startTimeoutMs || 5000);
    const identicalResponses = [];
    const repeatCount = options.identicalResponseCount || 10;

    for (let i = 0; i < repeatCount; ++i) {
      const response = await client.command('same', options.commandTimeoutMs || 5000);
      requireOkResponse(response, `same command ${i + 1}`);
      if (response.responseLine !== 'ok code=ok stable=true')
        throw new Error(`same command ${i + 1}: unexpected response line ${response.responseLine}`);
      identicalResponses.push(response);
    }

    let exitWhileWaitingError = null;
    try {
      await client.command('hang-then-exit', options.commandTimeoutMs || 5000);
    } catch (error) {
      exitWhileWaitingError = error instanceof Error ? error.message : String(error);
    }

    if (exitWhileWaitingError === null)
      throw new Error('Expected hang-then-exit to fail while waiting for a response');

    const exitState = await client.waitForExit(5000);
    if (exitState.code !== 7)
      throw new Error(`Expected fake shell to exit with code 7, got ${String(exitState.code)}`);

    return {
      readyLine,
      identicalResponses,
      exitWhileWaitingError,
      exitCode: client.exitCode,
      exitSignal: client.exitSignal,
      recentLines: client.getRecentLines(30),
    };
  } finally {
    await client.stop(true).catch(() => {});
    fs.rmSync(tempDir, { recursive: true, force: true });
  }
}

async function runAllSmokeTests(options = {}) {
  return {
    transport: await runTransportSmokeTest(options),
    commandErrors: await runCommandErrorSmokeTest(options),
    stateDump: await runStateDumpSmokeTest(options),
    protocolRegression: await runClientProtocolRegressionTest(options),
  };
}

module.exports = {
  NextStudioDebugShellClient,
  parseResponseLine,
  runTransportSmokeTest,
  runCommandErrorSmokeTest,
  runStateDumpSmokeTest,
  runClientProtocolRegressionTest,
  runAllSmokeTests,
};

if (require.main === module) {
  const args = process.argv.slice(2);
  const command = args[0] || 'smoke-transport';

  const runners = {
    'smoke-transport': runTransportSmokeTest,
    'smoke-errors': runCommandErrorSmokeTest,
    'smoke-state': runStateDumpSmokeTest,
    'smoke-protocol': runClientProtocolRegressionTest,
    'smoke-all': runAllSmokeTests,
  };

  const runner = runners[command];
  if (!runner) {
    console.error(`Unknown command: ${command}`);
    process.exitCode = 1;
  } else {
    runner()
      .then((result) => {
        console.log(JSON.stringify(result, null, 2));
      })
      .catch((error) => {
        console.error(error.stack || String(error));
        process.exitCode = 1;
      });
  }
}

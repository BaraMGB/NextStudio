#!/usr/bin/env node

const { spawn } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const defaultBinaryPath = path.resolve(process.cwd(), 'autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio');

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

class NextStudioDebugShellClient {
  constructor(options = {}) {
    this.binaryPath = path.resolve(options.binaryPath || defaultBinaryPath);
    this.cwd = path.resolve(options.cwd || process.cwd());
    this.defaultTimeoutMs = options.timeoutMs || 10000;
    this.process = null;
    this.lines = [];
    this.lineEntries = [];
    this.nextLineId = 1;
    this.waiters = new Set();
    this.exited = false;
    this.exitCode = null;
    this.exitSignal = null;
  }

  start(timeoutMs = 30000) {
    if (!fs.existsSync(this.binaryPath))
      throw new Error(`NextStudio binary not found: ${this.binaryPath}`);

    this.process = spawn(this.binaryPath, ['--debug-shell'], {
      cwd: this.cwd,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    this._attachStream(this.process.stdout, false);
    this._attachStream(this.process.stderr, true);
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

    return this.waitForLine((line) => line.startsWith('ok code=ready'), timeoutMs);
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

module.exports = {
  NextStudioDebugShellClient,
  parseResponseLine,
  runTransportSmokeTest,
};

if (require.main === module) {
  const args = process.argv.slice(2);
  const command = args[0] || 'smoke-transport';

  if (command === 'smoke-transport') {
    runTransportSmokeTest()
      .then((result) => {
        console.log(JSON.stringify(result, null, 2));
      })
      .catch((error) => {
        console.error(error.stack || String(error));
        process.exitCode = 1;
      });
  } else {
    console.error(`Unknown command: ${command}`);
    process.exitCode = 1;
  }
}

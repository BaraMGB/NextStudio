#!/usr/bin/env node
// Private JSON-lines adapter for native-cursor-regression.py. No HTTP port.
const readline = require('node:readline');
const { NextStudioDebugShellClient } = require('./debug-shell-client.js');

const client = new NextStudioDebugShellClient({ binaryPath: process.argv[2] });
const input = readline.createInterface({ input: process.stdin });
const reply = (value) => process.stdout.write(`${JSON.stringify(value)}\n`);
let failed = false;

async function run() {
  try {
    await client.start();
    const ready = (await client.waitForSystemReady()).parsed;
    if (ready.status !== 'ok') throw new Error(ready.message);
    reply({ ...ready, pid: client.process.pid });
    for await (const line of input) {
      const command = JSON.parse(line);
      const response = await client.command(typeof command === 'string' ? command : JSON.stringify(command));
      reply(response.parsed);
      if (command === 'quit') break;
    }
  } catch (error) {
    failed = true;
    reply({ status: 'error', message: error.message });
    console.error(client.lines.slice(-30).join('\n'));
  } finally {
    input.close();
    await client.stop();
    if (failed) process.exitCode = 1;
  }
}

process.on('SIGTERM', () => { input.close(); client.stop(true).finally(() => process.exit(1)); });
process.on('SIGINT', () => { input.close(); client.stop(true).finally(() => process.exit(1)); });
run();

import { randomUUID } from "node:crypto";
import { spawn, spawnSync, type ChildProcessWithoutNullStreams } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { basename, resolve } from "node:path";

import { Type } from "typebox";

type LineEntry = {
	id: number;
	line: string;
};

type ParsedResponse = {
	raw: string;
	status: "ok" | "error";
	code: string;
	message?: string;
	fields: Record<string, string>;
};

type Waiter = {
	matcher: (line: string, entry: LineEntry) => boolean;
	minLineId: number;
	resolve: (value: string | null) => void;
	reject: (error: Error) => void;
	timeout: NodeJS.Timeout;
};

type DebugSession = {
	id: string;
	process: ChildProcessWithoutNullStreams;
	launchRequestId: string | null;
	lineEntries: LineEntry[];
	nextLineId: number;
	waiters: Set<Waiter>;
	commandQueue: Promise<void>;
	closing: boolean;
	desynchronised: boolean;
	desynchronisedReason: string | null;
	exited: boolean;
	exitCode: number | null;
	exitSignal: NodeJS.Signals | null;
};

type SingleInstanceRejectionMarker = {
	reason?: unknown;
	requestId?: unknown;
	unixMs?: unknown;
	commandLine?: unknown;
	timestamp?: unknown;
	pid?: unknown;
};

const defaultBinaryPath = resolve(process.cwd(), "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio");
const maxBufferedLines = 500;
const debugShellSingleInstanceRejectionFile = resolve(tmpdir(), "NextStudio", "debug", "launch-rejections", "debug-shell-last-rejection.json");

function detectLikelySingleInstanceConflict(binaryPath: string) {
	if (process.platform === "win32")
		return null;

	const processName = basename(binaryPath);
	const diagnostics: string[] = [];

	const pgrep = spawnSync("pgrep", ["-a", "-x", processName], { encoding: "utf8" });
	if (pgrep.error == null && pgrep.status !== 127) {
		const runningProcesses = pgrep.stdout
			.split(/\r?\n/)
			.map((line) => line.trim())
			.filter((line) => line.length > 0)
			.filter((line) => !line.includes("<defunct>"));

		if (runningProcesses.length > 0) {
			return {
				processName,
				runningProcesses,
				diagnostics,
			};
		}

		diagnostics.push(`pgrep status=${String(pgrep.status)}`);
	}
	else if (pgrep.error != null) {
		diagnostics.push(`pgrep error=${pgrep.error.message}`);
	}

	const ps = spawnSync("ps", ["-A", "-o", "pid=,comm="], { encoding: "utf8" });
	if (ps.error == null && ps.status === 0) {
		const runningProcesses = ps.stdout
			.split(/\r?\n/)
			.map((line) => line.trim())
			.filter((line) => line.length > 0)
			.filter((line) => line.split(/\s+/, 2)[1] === processName);

		if (runningProcesses.length > 0) {
			return {
				processName,
				runningProcesses,
				diagnostics,
			};
		}
	}
	else if (ps.error != null) {
		diagnostics.push(`ps error=${ps.error.message}`);
	}

	return null;
}

function readSingleInstanceRejectionMarker() {
	if (!existsSync(debugShellSingleInstanceRejectionFile))
		return null;

	try {
		return JSON.parse(readFileSync(debugShellSingleInstanceRejectionFile, "utf8")) as SingleInstanceRejectionMarker;
	}
	catch {
		return null;
	}
}

function isMatchingSingleInstanceRejectionMarker(marker: SingleInstanceRejectionMarker | null, requestId: string, startTimeMs: number) {
	if (marker == null)
		return false;

	if (marker.reason !== "single-instance-conflict" || marker.requestId !== requestId)
		return false;

	const unixMs = Number(marker.unixMs);
	return Number.isFinite(unixMs) && unixMs >= startTimeMs - 5000;
}

async function waitForMatchingSingleInstanceRejectionMarker(requestId: string, startTimeMs: number, timeoutMs = 1500, pollIntervalMs = 50) {
	const deadline = Date.now() + timeoutMs;

	while (Date.now() <= deadline) {
		const marker = readSingleInstanceRejectionMarker();
		if (isMatchingSingleInstanceRejectionMarker(marker, requestId, startTimeMs))
			return marker;
		await new Promise((resolve) => setTimeout(resolve, pollIntervalMs));
	}

	return null;
}

function parseResponseLine(line: string): ParsedResponse {
	const parsed = JSON.parse(line) as Partial<ParsedResponse>;
	if (parsed == null || typeof parsed !== "object" || (parsed.status !== "ok" && parsed.status !== "error"))
		throw new Error("Invalid debug-shell JSON response");

	return {
		raw: line,
		status: parsed.status,
		code: typeof parsed.code === "string" ? parsed.code : "",
		message: typeof parsed.message === "string" ? parsed.message : undefined,
		fields: parsed.fields != null && typeof parsed.fields === "object" ? parsed.fields : {},
	};
}

function isResponseLine(line: string) {
	try {
		const parsed = parseResponseLine(line);
		return parsed.status === "ok" || parsed.status === "error";
	}
	catch {
		return false;
	}
}

function createLineCollector(session: DebugSession, source: "stdout" | "stderr") {
	let pending = "";

	return (chunk: Buffer | string) => {
		pending += chunk.toString();

		while (true) {
			const newlineIndex = pending.indexOf("\n");
			if (newlineIndex < 0)
				break;

			const rawLine = pending.slice(0, newlineIndex).replace(/\r$/, "");
			pending = pending.slice(newlineIndex + 1);
			const line = rawLine.length > 0 ? rawLine : "";
			pushLine(session, source === "stderr" ? `[stderr] ${line}` : line);
		}
	};
}

function pushLine(session: DebugSession, line: string) {
	const entry: LineEntry = { id: session.nextLineId++, line };
	session.lineEntries.push(entry);
	if (session.lineEntries.length > maxBufferedLines)
		session.lineEntries.splice(0, session.lineEntries.length - maxBufferedLines);

	for (const waiter of [...session.waiters]) {
		if (entry.id < waiter.minLineId || !waiter.matcher(line, entry))
			continue;

		clearTimeout(waiter.timeout);
		session.waiters.delete(waiter);
		waiter.resolve(line);
	}
}

function getRecentLines(session: DebugSession, count = 20) {
	return session.lineEntries.slice(-count).map((entry) => entry.line);
}

function getLinesSince(session: DebugSession, minLineId: number) {
	return session.lineEntries.filter((entry) => entry.id >= minLineId).map((entry) => entry.line);
}

function waitForLine(
	session: DebugSession,
	matcher: (line: string, entry: LineEntry) => boolean,
	timeoutMs: number,
	minLineId = 1,
) {
	for (let i = session.lineEntries.length - 1; i >= 0; --i) {
		const entry = session.lineEntries[i];
		if (entry.id < minLineId)
			break;
		if (matcher(entry.line, entry))
			return Promise.resolve(entry.line);
	}

	if (session.exited)
		return Promise.resolve<string | null>(null);

	return new Promise<string | null>((resolve, reject) => {
		const waiter: Waiter = {
			matcher,
			minLineId,
			resolve,
			reject,
			timeout: setTimeout(() => {
				session.waiters.delete(waiter);
				reject(new Error(`Timed out after ${timeoutMs} ms while waiting for shell output`));
			}, timeoutMs),
		};

		session.waiters.add(waiter);
	});
}

function rejectAllWaiters(session: DebugSession, error: Error) {
	for (const waiter of session.waiters) {
		clearTimeout(waiter.timeout);
		waiter.reject(error);
	}
	session.waiters.clear();
}

function flushWaitersOnExit(session: DebugSession, reason: string) {
	for (const waiter of session.waiters) {
		clearTimeout(waiter.timeout);
		waiter.resolve(null);
	}
	session.waiters.clear();
	pushLine(session, `[process] ${reason}`);
}

function enqueueCommand<T>(session: DebugSession, task: () => Promise<T>) {
	const previous = session.commandQueue.catch(() => {});
	let release = () => {};
	session.commandQueue = new Promise<void>((resolve) => {
		release = resolve;
	});

	return previous.then(task).finally(() => {
		release();
	});
}

function markSessionDesynchronised(session: DebugSession, reason: string) {
	session.desynchronised = true;
	session.desynchronisedReason = reason;
}

function serialiseSession(session: DebugSession) {
	return {
		sessionId: session.id,
		launchRequestId: session.launchRequestId,
		closing: session.closing,
		desynchronised: session.desynchronised,
		desynchronisedReason: session.desynchronisedReason,
		exited: session.exited,
		exitCode: session.exitCode,
		exitSignal: session.exitSignal,
		bufferedLines: session.lineEntries.length,
	};
}

function createSessionUnavailableResponse(session: DebugSession, sessionId: string) {
	if (session.closing) {
		return {
			content: [{ type: "text", text: `NextStudio debug session is closing: ${sessionId}` }],
			details: { ok: false, code: "session-closing", ...serialiseSession(session), recentLines: getRecentLines(session) },
		};
	}

	if (session.desynchronised) {
		return {
			content: [{ type: "text", text: `NextStudio debug session is desynchronised and must be restarted: ${sessionId}` }],
			details: { ok: false, code: "session-desynchronised", ...serialiseSession(session), recentLines: getRecentLines(session) },
		};
	}

	if (session.exited || !session.process.stdin.writable) {
		return {
			content: [{ type: "text", text: `NextStudio debug session is no longer writable: ${sessionId}` }],
			details: { ok: false, code: "session-exited", ...serialiseSession(session), recentLines: getRecentLines(session) },
		};
	}

	return null;
}

function validateCommandLine(command: string) {
	if (command.includes("\n") || command.includes("\r"))
		return "Debug shell commands must be a single line";

	if (command.trim().length === 0)
		return "Debug shell commands must not be empty";

	return null;
}

export default function (pi) {
	const sessions = new Map<string, DebugSession>();

	pi.on("session_shutdown", () => {
		for (const session of sessions.values()) {
			session.closing = true;
			rejectAllWaiters(session, new Error("NextStudio debug extension session is shutting down"));
			if (!session.exited)
				session.process.kill("SIGTERM");
		}
		sessions.clear();
	});

	pi.registerTool({
		name: "nextstudio_debug_start",
		label: "NextStudio Debug Start",
		description: "Start NextStudio in --debug-shell mode and wait for the ready response",
		parameters: Type.Object({
			binaryPath: Type.Optional(Type.String({ description: "Path to the NextStudio binary" })),
			cwd: Type.Optional(Type.String({ description: "Working directory for the NextStudio process" })),
			timeoutMs: Type.Optional(Type.Number({ description: "Startup timeout in milliseconds", minimum: 100, default: 20000 })),
		}),
		async execute(_toolCallId, params) {
			const binaryPath = resolve(params.binaryPath ?? defaultBinaryPath);
			const cwd = resolve(params.cwd ?? process.cwd());
			const timeoutMs = Math.max(100, Math.floor(params.timeoutMs ?? 20000));
			const launchRequestId = randomUUID();
			const startTimeMs = Date.now();

			if (!existsSync(binaryPath)) {
				return {
					content: [{ type: "text", text: `NextStudio binary not found: ${binaryPath}` }],
					details: { ok: false, code: "binary-not-found", binaryPath },
				};
			}

			const child = spawn(binaryPath, ["--debug-shell", `--debug-shell-request-id=${launchRequestId}`], {
				cwd,
				stdio: ["pipe", "pipe", "pipe"],
			});

			const session: DebugSession = {
				id: randomUUID(),
				process: child,
				launchRequestId,
				lineEntries: [],
				nextLineId: 1,
				waiters: new Set(),
				commandQueue: Promise.resolve(),
				closing: false,
				desynchronised: false,
				desynchronisedReason: null,
				exited: false,
				exitCode: null,
				exitSignal: null,
			};

			sessions.set(session.id, session);
			child.stdout.on("data", createLineCollector(session, "stdout"));
			child.stderr.on("data", createLineCollector(session, "stderr"));
			child.on("exit", (code, signal) => {
				session.exited = true;
				session.exitCode = code;
				session.exitSignal = signal;
				flushWaitersOnExit(session, `process exited code=${code} signal=${signal ?? "none"}`);
			});
			child.on("error", (error) => {
				pushLine(session, `[process-error] ${error.message}`);
				rejectAllWaiters(session, error instanceof Error ? error : new Error(String(error)));
			});

			try {
				const readyLine = await waitForLine(session, (line) => {
					if (!isResponseLine(line))
						return false;
					const parsed = parseResponseLine(line);
					return parsed.status === "ok" && parsed.code === "ready";
				}, timeoutMs);
				if (readyLine === null) {
					sessions.delete(session.id);
					const rejectionMarker = await waitForMatchingSingleInstanceRejectionMarker(launchRequestId, startTimeMs);
					const possibleRunningProcesses = detectLikelySingleInstanceConflict(binaryPath);
					const code = rejectionMarker ? "startup-single-instance-conflict" : "startup-exited";
					const message = rejectionMarker
						? "NextStudio is already running. JUCE single-instance protection rejected the debug-shell launch. Close the existing NextStudio instance and retry."
						: "Process exited before debug shell readiness was reported";
					return {
						content: [{ type: "text", text: `Failed to start NextStudio debug shell: ${message}` }],
						details: {
							ok: false,
							code,
							message,
							binaryPath,
							cwd,
							...serialiseSession(session),
							recentLines: getRecentLines(session),
							rejectionMarker,
							possibleRunningProcesses,
						},
					};
				}
				return {
					content: [{ type: "text", text: `Started NextStudio debug shell session ${session.id}` }],
					details: {
						ok: true,
						readyLine,
						parsed: parseResponseLine(readyLine),
						binaryPath,
						cwd,
						...serialiseSession(session),
						recentLines: getRecentLines(session),
					},
				};
			} catch (error) {
				child.kill();
				sessions.delete(session.id);
				const message = error instanceof Error ? error.message : String(error);
				const code = message.startsWith("Timed out after ") ? "startup-timeout" : "startup-failed";
				return {
					content: [{ type: "text", text: `Failed to start NextStudio debug shell: ${message}` }],
					details: { ok: false, code, message, binaryPath, cwd, recentLines: getRecentLines(session) },
				};
			}
		},
	});

	pi.registerTool({
		name: "nextstudio_debug_command",
		label: "NextStudio Debug Command",
		description: "Send one command line to a running NextStudio debug-shell session and wait for the next shell response",
		parameters: Type.Object({
			sessionId: Type.String({ description: "Session id returned by nextstudio_debug_start" }),
			command: Type.String({ description: "Single debug-shell command line" }),
			timeoutMs: Type.Optional(Type.Number({ description: "Response timeout in milliseconds", minimum: 100, default: 10000 })),
		}),
		async execute(_toolCallId, params) {
			const session = sessions.get(params.sessionId);
			const timeoutMs = Math.max(100, Math.floor(params.timeoutMs ?? 10000));
			const commandLine = String(params.command);
			const validationError = validateCommandLine(commandLine);

			if (!session) {
				return {
					content: [{ type: "text", text: `Unknown NextStudio debug session: ${params.sessionId}` }],
					details: { ok: false, code: "unknown-session", sessionId: params.sessionId },
				};
			}

			if (validationError !== null) {
				return {
					content: [{ type: "text", text: validationError }],
					details: { ok: false, code: "invalid-command", message: validationError, sessionId: params.sessionId },
				};
			}

			return enqueueCommand(session, async () => {
				const unavailableResponse = createSessionUnavailableResponse(session, params.sessionId);
				if (unavailableResponse !== null)
					return unavailableResponse;

				const startLineId = session.nextLineId;
				session.process.stdin.write(commandLine.trim() + "\n");

				try {
					const responseLine = await waitForLine(
						session,
						isResponseLine,
						timeoutMs,
						startLineId,
					);

					const parsed = responseLine ? parseResponseLine(responseLine) : null;
					if (commandLine.trim().toLowerCase() === "quit" && parsed?.status === "ok")
						session.closing = true;
					const newLines = getLinesSince(session, startLineId);
					return {
						content: [{ type: "text", text: responseLine ?? `No shell response received for command: ${commandLine}` }],
						details: {
							ok: parsed?.status === "ok",
							code: responseLine === null ? "session-exited" : parsed?.code,
							message: parsed?.message,
							command: commandLine,
							responseLine,
							parsed,
							newLines,
							...serialiseSession(session),
						},
					};
				} catch (error) {
					const message = error instanceof Error ? error.message : String(error);
					const code = message.startsWith("Timed out after ") ? "response-timeout" : "response-failed";
					if (code === "response-timeout")
						markSessionDesynchronised(session, `Timed out waiting for response to command: ${params.command}`);
					return {
						content: [{ type: "text", text: `Failed waiting for response to: ${commandLine}` }],
						details: {
							ok: false,
							code,
							message,
							command: commandLine,
							newLines: getLinesSince(session, startLineId),
							...serialiseSession(session),
							recentLines: getRecentLines(session),
						},
					};
				}
			});
		},
	});

	pi.registerTool({
		name: "nextstudio_debug_stop",
		label: "NextStudio Debug Stop",
		description: "Stop a running NextStudio debug-shell session",
		parameters: Type.Object({
			sessionId: Type.String({ description: "Session id returned by nextstudio_debug_start" }),
			force: Type.Optional(Type.Boolean({ description: "Use SIGKILL instead of SIGTERM", default: false })),
		}),
		async execute(_toolCallId, params) {
			const session = sessions.get(params.sessionId);
			if (!session) {
				return {
					content: [{ type: "text", text: `Unknown NextStudio debug session: ${params.sessionId}` }],
					details: { ok: false, code: "unknown-session", sessionId: params.sessionId },
				};
			}

			session.closing = true;
			sessions.delete(params.sessionId);

			if (!session.exited)
				session.process.kill(params.force ? "SIGKILL" : "SIGTERM");

			return {
				content: [{ type: "text", text: `Stopped NextStudio debug session ${params.sessionId}` }],
				details: { ok: true, force: Boolean(params.force), ...serialiseSession(session), recentLines: getRecentLines(session) },
			};
		},
	});
}

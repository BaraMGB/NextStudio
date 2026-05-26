import { randomUUID } from "node:crypto";
import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve } from "node:path";

import { Type } from "typebox";

type Waiter = {
	matcher: (line: string) => boolean;
	resolve: (value: string | null) => void;
	reject: (error: Error) => void;
	timeout: NodeJS.Timeout;
};

type DebugSession = {
	id: string;
	process: ChildProcessWithoutNullStreams;
	lines: string[];
	waiters: Set<Waiter>;
	exited: boolean;
	exitCode: number | null;
	exitSignal: NodeJS.Signals | null;
};

const defaultBinaryPath = resolve(process.cwd(), "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio");
const maxBufferedLines = 400;

function createLineCollector(session: DebugSession, source: "stdout" | "stderr") {
	let pending = "";

	return (chunk: Buffer | string) => {
		pending += chunk.toString();

		while (true) {
			const newlineIndex = pending.indexOf("\n");
			if (newlineIndex < 0) break;

			const rawLine = pending.slice(0, newlineIndex).replace(/\r$/, "");
			pending = pending.slice(newlineIndex + 1);
			const line = rawLine.length > 0 ? rawLine : "";
			pushLine(session, source === "stderr" ? `[stderr] ${line}` : line);
		}
	};
}

function pushLine(session: DebugSession, line: string) {
	session.lines.push(line);
	if (session.lines.length > maxBufferedLines)
		session.lines.splice(0, session.lines.length - maxBufferedLines);

	for (const waiter of [...session.waiters]) {
		if (!waiter.matcher(line))
			continue;

		clearTimeout(waiter.timeout);
		session.waiters.delete(waiter);
		waiter.resolve(line);
	}
}

function waitForLine(session: DebugSession, matcher: (line: string) => boolean, timeoutMs: number) {
	for (let i = session.lines.length - 1; i >= 0; --i)
		if (matcher(session.lines[i]))
			return Promise.resolve(session.lines[i]);

	if (session.exited)
		return Promise.resolve<string | null>(null);

	return new Promise<string | null>((resolve, reject) => {
		const waiter: Waiter = {
			matcher,
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

function flushWaitersOnExit(session: DebugSession, reason: string) {
	for (const waiter of session.waiters) {
		clearTimeout(waiter.timeout);
		waiter.resolve(null);
	}
	session.waiters.clear();
	pushLine(session, `[process] ${reason}`);
}

function serialiseSession(session: DebugSession) {
	return {
		sessionId: session.id,
		exited: session.exited,
		exitCode: session.exitCode,
		exitSignal: session.exitSignal,
		bufferedLines: session.lines.length,
	};
}

export default function (pi) {
	const sessions = new Map<string, DebugSession>();

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

				if (!existsSync(binaryPath)) {
					return {
						content: [{ type: "text", text: `NextStudio binary not found: ${binaryPath}` }],
						details: { ok: false, code: "binary-not-found", binaryPath },
					};
				}

				const child = spawn(binaryPath, ["--debug-shell"], {
					cwd,
					stdio: ["pipe", "pipe", "pipe"],
				});

				const session: DebugSession = {
					id: randomUUID(),
					process: child,
					lines: [],
					waiters: new Set(),
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
				});

				try {
					const readyLine = await waitForLine(session, (line) => line.startsWith("ok code=ready"), timeoutMs);
					return {
						content: [{ type: "text", text: `Started NextStudio debug shell session ${session.id}` }],
						details: {
							ok: readyLine !== null,
							readyLine,
							binaryPath,
							cwd,
							...serialiseSession(session),
							recentLines: session.lines.slice(-20),
						},
					};
				} catch (error) {
					child.kill();
					sessions.delete(session.id);
					const message = error instanceof Error ? error.message : String(error);
					return {
						content: [{ type: "text", text: `Failed to start NextStudio debug shell: ${message}` }],
						details: { ok: false, code: "startup-timeout", message, binaryPath, cwd, recentLines: session.lines.slice(-20) },
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

				if (!session) {
					return {
						content: [{ type: "text", text: `Unknown NextStudio debug session: ${params.sessionId}` }],
						details: { ok: false, code: "unknown-session", sessionId: params.sessionId },
					};
				}

				if (session.exited || !session.process.stdin.writable) {
					return {
						content: [{ type: "text", text: `NextStudio debug session is no longer writable: ${params.sessionId}` }],
						details: { ok: false, code: "session-exited", ...serialiseSession(session), recentLines: session.lines.slice(-20) },
					};
				}

				const startIndex = session.lines.length;
				session.process.stdin.write(params.command + "\n");

				try {
					const responseLine = await waitForLine(session, (line) => {
						if (!line.startsWith("ok ") && !line.startsWith("error "))
							return false;
						const responseIndex = session.lines.lastIndexOf(line);
						return responseIndex >= startIndex;
					}, timeoutMs);

					const newLines = session.lines.slice(startIndex);
					return {
						content: [{ type: "text", text: responseLine ?? `No shell response received for command: ${params.command}` }],
						details: {
							ok: responseLine !== null && responseLine.startsWith("ok "),
							command: params.command,
							responseLine,
							newLines,
							...serialiseSession(session),
						},
					};
				} catch (error) {
					const message = error instanceof Error ? error.message : String(error);
					return {
						content: [{ type: "text", text: `Timed out waiting for response to: ${params.command}` }],
						details: {
							ok: false,
							code: "response-timeout",
							message,
							command: params.command,
							newLines: session.lines.slice(startIndex),
							...serialiseSession(session),
						},
					};
				}
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

				if (!session.exited)
					session.process.kill(params.force ? "SIGKILL" : "SIGTERM");

				sessions.delete(params.sessionId);
				return {
					content: [{ type: "text", text: `Stopped NextStudio debug session ${params.sessionId}` }],
					details: { ok: true, force: Boolean(params.force), ...serialiseSession(session), recentLines: session.lines.slice(-20) },
				};
			},
		});
}

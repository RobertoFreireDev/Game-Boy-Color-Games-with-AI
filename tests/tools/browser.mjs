/*
 * Tiny headless-browser driver for the tool tests (no npm packages).
 * Launches Chrome or Edge with the DevTools protocol, opens one page and lets
 * tests run code inside it, send real mouse input and reload it between tests.
 * Override the browser with the CHROME_PATH environment variable.
 */
import { spawn } from 'node:child_process';
import { existsSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const CANDIDATES = [
  process.env.CHROME_PATH,
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
  '/usr/bin/google-chrome', '/usr/bin/google-chrome-stable', '/usr/bin/chromium', '/usr/bin/chromium-browser', '/usr/bin/microsoft-edge',
];

export function findBrowser() {
  return CANDIDATES.find(p => p && existsSync(p)) || null;
}

export async function launch({ width = 1280, height = 800 } = {}) {
  const exe = findBrowser();
  if (!exe) throw new Error('No Chrome/Edge found. Set CHROME_PATH to a Chromium-based browser.');
  const profile = mkdtempSync(join(tmpdir(), 'gbc-tool-tests-'));
  const proc = spawn(exe, [
    '--headless=new', '--remote-debugging-port=0', `--user-data-dir=${profile}`, `--window-size=${width},${height}`,
    '--no-first-run', '--no-default-browser-check', '--disable-extensions', '--allow-file-access-from-files',
    '--force-device-scale-factor=1', 'about:blank',
  ], { stdio: ['ignore', 'ignore', 'pipe'] });

  const wsBrowser = await new Promise((res, rej) => {
    let err = '';
    const timer = setTimeout(() => rej(new Error('Browser did not start:\n' + err)), 20000);
    proc.stderr.on('data', d => {
      err += d;
      const m = /DevTools listening on (ws:\/\/\S+)/.exec(err);
      if (m) { clearTimeout(timer); res(m[1]); }
    });
    proc.on('exit', code => { clearTimeout(timer); rej(new Error(`Browser exited (${code}):\n` + err)); });
  });
  const port = new URL(wsBrowser).port;
  const targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  const pageTarget = targets.find(t => t.type === 'page');
  const page = new Page(await connect(pageTarget.webSocketDebuggerUrl));
  await page.send('Page.enable');
  await page.send('Runtime.enable');
  page.close = async () => {
    try { page.ws.close(); } catch (_) { /* already closed */ }
    proc.kill();
    await new Promise(r => proc.once('exit', r).once('error', r));
    try { rmSync(profile, { recursive: true, force: true }); } catch (_) { /* profile still locked: leave it */ }
  };
  return page;
}

function connect(url) {
  return new Promise((res, rej) => {
    const ws = new WebSocket(url);
    ws.onopen = () => res(ws);
    ws.onerror = e => rej(new Error('DevTools connection failed: ' + (e.message || e.type)));
  });
}

class Page {
  constructor(ws) {
    this.ws = ws; this.id = 0; this.calls = new Map(); this.waiters = []; this.errors = [];
    ws.onmessage = ev => {
      const msg = JSON.parse(ev.data);
      if (msg.id && this.calls.has(msg.id)) {
        const { res, rej } = this.calls.get(msg.id); this.calls.delete(msg.id);
        msg.error ? rej(new Error(msg.error.message)) : res(msg.result);
      } else if (msg.method) {
        if (msg.method === 'Runtime.exceptionThrown') this.errors.push(msg.params.exceptionDetails);
        this.waiters = this.waiters.filter(w => (w.method === msg.method ? (w.res(msg.params), false) : true));
      }
    };
  }
  send(method, params = {}) {
    const id = ++this.id;
    return new Promise((res, rej) => { this.calls.set(id, { res, rej }); this.ws.send(JSON.stringify({ id, method, params })); });
  }
  once(method) { return new Promise(res => this.waiters.push({ method, res })); }

  /* Evaluate an expression in the page's global scope (sees the page's top-level const/let/functions). */
  async eval(expression) {
    const r = await this.send('Runtime.evaluate', { expression, awaitPromise: true, returnByValue: true, userGesture: true });
    if (r.exceptionDetails) {
      const d = r.exceptionDetails;
      throw new Error('In page: ' + ((d.exception && d.exception.description) || d.text));
    }
    return r.result.value;
  }
  /* Run a function inside the page: page.run((a, b) => S.W + a, 1). Arguments must be JSON. */
  run(fn, ...args) { return this.eval(`(${fn})(...${JSON.stringify(args)})`); }

  async goto(url) {
    const loaded = this.once('Page.loadEventFired');
    await this.send('Page.navigate', { url });
    await loaded;
  }
  async waitFor(expression, timeout = 5000) {
    const t0 = Date.now();
    for (;;) {
      if (await this.eval(expression)) return;
      if (Date.now() - t0 > timeout) throw new Error('Timed out waiting for: ' + expression);
      await new Promise(r => setTimeout(r, 10));
    }
  }

  /* Real mouse input (produces trusted pointer events). modifiers: alt, ctrl, meta, shift. */
  mouse(type, x, y, { button = 'left', buttons, alt, ctrl, meta, shift, deltaY = 0, deltaX = 0 } = {}) {
    const bits = { left: 1, right: 2, middle: 4, none: 0 };
    const modifiers = (alt ? 1 : 0) | (ctrl ? 2 : 0) | (meta ? 4 : 0) | (shift ? 8 : 0);
    const params = { type, x, y, modifiers, button, clickCount: type === 'mouseMoved' || type === 'mouseWheel' ? 0 : 1 };
    params.buttons = buttons !== undefined ? buttons : type === 'mouseReleased' ? 0 : bits[button];
    if (type === 'mouseWheel') Object.assign(params, { deltaX, deltaY, button: 'none', buttons: 0 });
    return this.send('Input.dispatchMouseEvent', params);
  }
}

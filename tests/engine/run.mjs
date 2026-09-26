/*
 * Engine unit tests: builds one ROM per tests/engine/test_*.c with GBDK and runs
 * each in the headless emulator (gbc.mjs). No npm packages.
 *
 *   node tests/engine/run.mjs              all suites
 *   node tests/engine/run.mjs map text     only test_map.c and test_text.c
 *   node tests/engine/run.mjs -v           also list passing tests
 *
 * ROMs + objects go to build/tests/engine/. GBDK is found through GBDK_HOME
 * (default C:\gbdk on Windows, ~/gbdk elsewhere), like build.bat / build.sh.
 */
import { spawn } from 'node:child_process';
import { readdirSync, readFileSync, mkdirSync, rmSync, existsSync, copyFileSync } from 'node:fs';
import { join, dirname, basename } from 'node:path';
import { fileURLToPath } from 'node:url';
import { homedir, cpus } from 'node:os';
import { GBC } from './gbc.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const ROOT = join(HERE, '..', '..');
const OUT = 'build/tests/engine';                 /* relative to ROOT: lcc splits paths with spaces */
const OBJ = `${OUT}/obj`;
const CFLAGS = ['-Isrc', '-Iassets', '-Itests/engine'];
const LFLAGS = ['-Wm-yC', '-Wl-yt0x1B', '-Wl-ya1', '-Wl-yoA', '-autobank'];
const MAX_SECONDS = 30;                           /* emulated time per suite before it counts as hung */
const CPU_HZ = 8388608;                           /* double speed */

const args = process.argv.slice(2);
const verbose = args.includes('-v');
const only = args.filter(a => !a.startsWith('-'));

const isWin = process.platform === 'win32';
const gbdk = process.env.GBDK_HOME || (isWin ? 'C:\\gbdk' : join(homedir(), 'gbdk'));
const LCC = join(gbdk, 'bin', isWin ? 'lcc.exe' : 'lcc');
if (!existsSync(LCC)) { console.error(`lcc not found at ${LCC} (set GBDK_HOME)`); process.exit(2); }

function lcc(argv) {
  return new Promise(res => {
    const p = spawn(LCC, argv, { cwd: ROOT });
    let out = '';
    p.stdout.on('data', d => out += d);
    p.stderr.on('data', d => out += d);
    p.on('close', code => res({ code, out: out.trim() }));
  });
}

async function pool(items, fn) {
  const results = new Array(items.length);
  let next = 0;
  const worker = async () => { while (next < items.length) { const i = next++; results[i] = await fn(items[i]); } };
  await Promise.all(Array.from({ length: Math.max(1, cpus().length) }, worker));
  return results;
}

const objName = src => `${OBJ}/${basename(src, '.c')}.o`;

/* ---------- build ---------- */

const engineSrc = readdirSync(join(ROOT, 'src/engine')).filter(f => f.endsWith('.c')).map(f => `src/engine/${f}`);
const testDir = readdirSync(HERE).filter(f => f.endsWith('.c'));
const suites = testDir.filter(f => f.startsWith('test_'))
  .map(f => f.slice(5, -2))
  .filter(n => !only.length || only.includes(n));
const supportSrc = testDir.filter(f => !f.startsWith('test_')).map(f => `tests/engine/${f}`);
const commonSrc = [...engineSrc, 'assets/fonts/font_main.c', ...supportSrc];

if (!suites.length) { console.error('no matching test suites'); process.exit(2); }

rmSync(join(ROOT, OUT), { recursive: true, force: true });
mkdirSync(join(ROOT, OBJ), { recursive: true });

const toCompile = [...commonSrc, ...suites.map(n => `tests/engine/test_${n}.c`)];
/* asserts on constant expressions make SDCC report folded branches (110) and dead code (126) */
const TEST_CFLAGS = ['-Wf--disable-warning', '-Wf110', '-Wf--disable-warning', '-Wf126'];
const compiled = await pool(toCompile, async src => {
  const flags = src.startsWith('tests/') ? [...CFLAGS, ...TEST_CFLAGS] : CFLAGS;
  return { src, ...(await lcc([...flags, '-c', '-o', objName(src), src])) };
});
let buildFailed = false;
for (const r of compiled) {
  if (r.out) console.log(`${r.src}:\n${r.out}`);
  if (r.code) buildFailed = true;
}
if (buildFailed) { console.error('BUILD FAILED'); process.exit(1); }

/* -autobank (bankpack) rewrites the object files it links, so each suite links its own copies */
const linked = await pool(suites, async n => {
  const dir = `${OUT}/link_${n}`;
  mkdirSync(join(ROOT, dir), { recursive: true });
  const objs = [...commonSrc, `tests/engine/test_${n}.c`].map(src => {
    const o = `${dir}/${basename(src, '.c')}.o`;
    copyFileSync(join(ROOT, objName(src)), join(ROOT, o));
    return o;
  });
  const rom = `${OUT}/test_${n}.gb`;
  return { n, rom, ...(await lcc([...LFLAGS, '-o', rom, ...objs])) };
});
for (const r of linked) {
  if (r.out) console.log(`link test_${r.n}:\n${r.out}`);
  if (r.code || !existsSync(join(ROOT, r.rom))) buildFailed = true;
}
if (buildFailed) { console.error('LINK FAILED'); process.exit(1); }

/* ---------- run ---------- */

let totalPass = 0, totalFail = 0;
const failedSuites = [];

for (const { n, rom } of linked) {
  if (!existsSync(join(ROOT, rom))) {      /* e.g. a second test run cleaned build/tests/engine meanwhile */
    console.log(`FAIL ${n}: ${rom} disappeared before it could run`);
    failedSuites.push(n); totalFail++;
    continue;
  }
  const gb = new GBC(readFileSync(join(ROOT, rom)));
  let text = '', done = false;
  gb.onSerial = b => { if (b === 4) done = true; else text += String.fromCharCode(b); };
  let error = null;
  try { gb.run(() => done, MAX_SECONDS * CPU_HZ); } catch (e) { error = e.message; }

  const lines = text.split('\n').filter(Boolean);
  const m = /^DONE (\d+) (\d+)$/.exec(lines[lines.length - 1] || '');
  let pass = 0, fail = 0;
  if (m) { pass = +m[1]; fail = +m[2]; }
  const ok = m && !fail && !error;
  console.log(`${ok ? 'PASS' : 'FAIL'} ${n}: ${pass} passed, ${fail} failed` +
    (error ? ` (emulator: ${error})` : !done ? ` (hung after ${MAX_SECONDS} s emulated, PC=0x${gb.pc.toString(16)})` : ''));
  for (const line of lines) {
    if (line.startsWith('DONE')) continue;
    if (line.startsWith('ok ') && !verbose) continue;
    console.log(`  ${line}`);
  }
  if (!m && lines.length) {
    const last = lines[lines.length - 1];
    if (last.startsWith('ok ')) console.log(`  (stopped after: ${last.slice(5)})`);
  }
  totalPass += pass; totalFail += fail;
  if (!ok) { failedSuites.push(n); if (!m) totalFail++; }
}

console.log(`\n${totalPass} passed, ${totalFail} failed` + (failedSuites.length ? ` — failing suites: ${failedSuites.join(', ')}` : ''));
process.exit(failedSuites.length ? 1 : 0);

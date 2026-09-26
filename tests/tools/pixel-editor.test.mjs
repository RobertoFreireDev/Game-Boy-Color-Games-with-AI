/*
 * Unit tests for tools/pixel-editor.html.
 * Run from the project root:   node --test tests/
 * Needs Node 22+ and Chrome or Edge (headless, see browser.mjs). No npm packages.
 *
 * Every test starts from a freshly loaded editor with empty localStorage, then calls the
 * editor's own functions inside the page, or drives it with real mouse input and key events.
 * `T` is a small helper object installed in the page (see installHelpers).
 */
import { describe, test, before, after, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { launch } from './browser.mjs';

const EDITOR = new URL('../../tools/pixel-editor.html', import.meta.url).href;
let page;
const pack5 = (r, g, b) => r | (g << 5) | (b << 10);

/* runs inside the page after every load */
function installHelpers() {
  window.T = {
    toasts: [],
    doc(w, h, mode = 'sprite') { newDoc(w, h, mode, false); undoStack.length = 0; redoStack.length = 0; analyze(); updateButtons(); },
    px(x, y) { return S.px[y * S.W + x]; },
    rect(x, y, w, h, c) { for (let j = y; j < y + h; j++) for (let i = x; i < x + w; i++) putPx(i, j, c); },
    count(c) { let n = 0; for (const v of S.px) if (v === c) n++; return n; },
    key(key, o = {}, target = document) {
      const e = new KeyboardEvent('keydown', { key, bubbles: true, cancelable: true, ...o });
      target.dispatchEvent(e); return e.defaultPrevented;
    },
    screen(x, y) { const r = view.getBoundingClientRect(); return [Math.round(r.left + S.offX + (x + 0.5) * S.zoom), Math.round(r.top + S.offY + (y + 0.5) * S.zoom)]; },
    last() { return this.toasts[this.toasts.length - 1] || ''; },
    stat(label) {
      for (const d of document.querySelectorAll('#stats .st')) {
        if (d.firstChild.textContent === label) { const b = d.querySelector('b'); return { val: b.textContent, cls: b.className }; }
      }
      return null;
    },
    png(w, h, fn) {             /* fn(x, y) → [r, g, b, a] */
      const c = document.createElement('canvas'); c.width = w; c.height = h;
      const x = c.getContext('2d'), id = x.createImageData(w, h);
      for (let y = 0; y < h; y++) for (let i = 0; i < w; i++) id.data.set(fn(i, y), (y * w + i) * 4);
      x.putImageData(id, 0, 0);
      return new Promise(r => c.toBlob(r, 'image/png'));
    },
    async file(name, w, h, fn) { return new File([await this.png(w, h, fn)], name, { type: 'image/png' }); },
    rgba(canvas, x, y) { return [...canvas.getContext('2d').getImageData(x, y, 1, 1).data]; },
    stubSave() {
      T.saved = null;
      window.showSaveFilePicker = async o => ({ name: o.suggestedName, createWritable: async () => ({ write: async b => { T.saved = { blob: b, opts: o }; }, close: async () => {} }) });
    },
    wait(ms) { return new Promise(r => setTimeout(r, ms)); },
  };
  const original = toast;
  toast = (m, ms) => { T.toasts.push(m); original(m, ms); };
}

async function load() {
  await page.goto(EDITOR);
  await page.waitFor('typeof A !== "undefined" && A !== null');
  await page.run(installHelpers);
}

before(async () => {
  page = await launch();
  await page.send('Page.addScriptToEvaluateOnNewDocument', {
    source: "if (!sessionStorage.getItem('keepStorage')) localStorage.clear(); sessionStorage.removeItem('keepStorage');",
  });
});
after(() => page && page.close());
beforeEach(load);

const run = (fn, ...a) => page.run(fn, ...a);
const at = (x, y) => run((x, y) => T.screen(x, y), x, y);
async function click(x, y, o = {}) {
  const [sx, sy] = await at(x, y);
  await page.mouse('mousePressed', sx, sy, o);
  await page.mouse('mouseReleased', sx, sy, o);
}
async function drag(points, o = {}) {
  const pts = [];
  for (const [x, y] of points) pts.push(await at(x, y));
  await page.mouse('mousePressed', ...pts[0], o);
  for (const p of pts.slice(1)) await page.mouse('mouseMoved', ...p, o);
  await page.mouse('mouseReleased', ...pts[pts.length - 1], o);
}
async function hover(x, y) { const [sx, sy] = await at(x, y); await page.mouse('mouseMoved', sx, sy, { button: 'none' }); }

/* ================================================================================================ */
describe('startup', () => {
  test('opens a blank 128×64 sprite sheet named "player" with the pencil', async () => {
    const s = await run(() => ({ W: S.W, H: S.H, mode: S.mode, name: $('#name').value, tool: S.tool, allT: S.px.every(v => v === -1), prim: S.prim, sec: S.sec }));
    assert.deepEqual(s, { W: 128, H: 64, mode: 'sprite', name: 'player', tool: 'pencil', allT: true, prim: { p: 0, i: 1 }, sec: { p: 0, i: 0 } });
  });
  test('shows the welcome toast and has no page errors', async () => {
    assert.match(await run(() => $('#toast').textContent), /Press \? for shortcuts/);
    assert.equal(page.errors.length, 0, JSON.stringify(page.errors));
  });
  test('builds the tool bar, palettes and stats', async () => {
    const s = await run(() => ({ tools: document.querySelectorAll('#tools button[data-tool]').length, rows: document.querySelectorAll('#pals .prow').length, stats: !!T.stat('Size') }));
    assert.deepEqual(s, { tools: 12, rows: 8, stats: true });
  });
  test('undo and redo start disabled', async () => {
    assert.deepEqual(await run(() => [$('#bUndo').disabled, $('#bRedo').disabled]), [true, true]);
  });
});

/* ================================================================================================ */
describe('colors (RGB555)', () => {
  test('c5to8 expands 5-bit channels like the GBC', async () => {
    assert.deepEqual(await run(() => [c5to8(0), c5to8(16), c5to8(31)]), [0, 132, 255]);
  });
  test('c8to5 rounds 8-bit channels and round-trips every 5-bit value', async () => {
    assert.deepEqual(await run(() => [c8to5(0), c8to5(128), c8to5(255)]), [0, 16, 31]);
    assert.equal(await run(() => [...Array(32).keys()].every(v => c8to5(c5to8(v)) === v)), true);
  });
  test('pack / R5 / G5 / B5 use the GBC bit layout r | g<<5 | b<<10', async () => {
    assert.deepEqual(await run(() => { const c = pack(1, 2, 3); return [c, R5(c), G5(c), B5(c)]; }), [3137, 1, 2, 3]);
  });
  test('p8 packs 8-bit RGB', async () => {
    assert.deepEqual(await run(() => [p8(255, 0, 0), p8(0, 255, 0), p8(0, 0, 255), p8(255, 255, 255)]), [31, 992, 31744, 32767]);
  });
  test('LUT holds opaque RGBA for every color', async () => {
    assert.deepEqual(await run(() => [LUT.length, LUT[pack(31, 0, 0)], LUT[0], LUT[32767]]), [32768, 0xFF0000FF, 0xFF000000, 0xFFFFFFFF]);
  });
  test('css and hex format a color', async () => {
    assert.deepEqual(await run(() => [css(pack(31, 0, 0)), hex(pack(31, 16, 0)), hex(0)]), ['rgb(255,0,0)', '#ff8400', '#000000']);
  });
  test('luma orders black < blue < red < green < white', async () => {
    assert.equal(await run(() => luma(0) < luma(pack(0, 0, 31)) && luma(pack(0, 0, 31)) < luma(pack(31, 0, 0)) && luma(pack(31, 0, 0)) < luma(pack(0, 31, 0)) && luma(pack(0, 31, 0)) < luma(32767)), true);
  });
  test('dist is symmetric, zero on equal colors, weighted 3/4/2', async () => {
    assert.deepEqual(await run(() => [dist(pack(1, 0, 0), 0), dist(pack(0, 1, 0), 0), dist(pack(0, 0, 1), 0), dist(5000, 5000), dist(123, 9999) === dist(9999, 123)]), [3, 4, 2, 0, true]);
  });
  test('parseHex accepts #rrggbb, rrggbb and #rgb, snaps to RGB555, rejects junk', async () => {
    assert.deepEqual(await run(() => [parseHex('#ff0000'), parseHex('FF0000'), parseHex(' #f00 '), parseHex('#010101'), parseHex('red'), parseHex('#12345')]), [31, 31, 31, 0, -1, -1]);
  });
});

/* ================================================================================================ */
describe('default palettes', () => {
  test('8 palettes of 4 valid RGB555 colors in both modes', async () => {
    assert.equal(await run(() => ['sprite', 'map'].every(m => { const p = defaultPals(m); return p.length === 8 && p.every(q => q.length === 4 && q.every(c => c >= 0 && c <= 32767)); })), true);
  });
  test('sprite palettes have color 0 = 0 and distinct visible colors', async () => {
    assert.equal(await run(() => defaultPals('sprite').every(p => p[0] === 0 && new Set(p.slice(1)).size === 3)), true);
  });
  test('map palettes go light → dark', async () => {
    assert.equal(await run(() => defaultPals('map').every(p => luma(p[0]) > luma(p[1]) && luma(p[1]) > luma(p[2]) && luma(p[2]) > luma(p[3]))), true);
  });
  test('each call returns fresh arrays', async () => {
    assert.equal(await run(() => { const a = defaultPals('map'); a[0][0] = 1; return defaultPals('map')[0][0] !== 1; }), true);
  });
  test('slotColor: color 0 is transparent (-1) only in sprite mode', async () => {
    assert.deepEqual(await run(() => { const a = slotColor({ p: 0, i: 0 }); T.doc(8, 8, 'map'); return [a, slotColor({ p: 0, i: 0 }) === S.pals[0][0]]; }), [-1, true]);
  });
});

/* ================================================================================================ */
describe('image buffer', () => {
  test('putPx writes the pixel and the canvas, returns false when unchanged', async () => {
    const r = await run(() => {
      T.doc(16, 16);
      const a = putPx(1, 1, pack(31, 0, 0)), b = putPx(1, 1, pack(31, 0, 0));
      putPx(2, 1, -1); flush();
      return [a, b, T.px(1, 1), T.rgba(imgCanvas, 1, 1), T.rgba(imgCanvas, 2, 1)[3]];
    });
    assert.deepEqual(r, [true, false, 31, [255, 0, 0, 255], 0]);
  });
  test('setPx ignores pixels outside the image', async () => {
    assert.equal(await run(() => { T.doc(8, 8); setPx(-1, 0, 5); setPx(8, 0, 5); setPx(0, 8, 5); return T.count(5); }), 0);
  });
  test('setPx is clipped to the selection', async () => {
    assert.deepEqual(await run(() => { T.doc(16, 16); S.sel = { x: 2, y: 2, w: 2, h: 2 }; setPx(0, 0, 5); setPx(3, 3, 5); return [T.px(0, 0), T.px(3, 3)]; }), [-1, 5]);
  });
  test('setPx is not clipped while pixels float', async () => {
    assert.equal(await run(() => { T.doc(16, 16); S.sel = { x: 2, y: 2, w: 2, h: 2 }; S.flt = { x: 0, y: 0, w: 1, h: 1, d: new Int32Array(1) }; setPx(0, 0, 5); return T.px(0, 0); }), 5);
  });
  test('rebuildImage redraws the canvas from S.px', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); S.px[9] = pack(0, 31, 0); rebuildImage(); return T.rgba(imgCanvas, 1, 1); }), [0, 255, 0, 255]);
  });
  test('markDirty grows one rectangle', async () => {
    assert.deepEqual(await run(() => { flush(); markDirty(5, 5); markDirty(2, 7); markDirty(9, 1); const d = { ...dirty }; flush(); return [d, dirty]; }), [{ x0: 2, y0: 1, x1: 9, y1: 7 }, null]);
  });
});

/* ================================================================================================ */
describe('undo / redo', () => {
  test('an edit with no change adds no undo step', async () => {
    assert.equal(await run(() => { T.doc(8, 8); beginEdit(); setPx(0, 0, -1); endEdit(); return undoStack.length; }), 0);
  });
  test('undo restores pixels, redo re-applies them', async () => {
    const r = await run(() => {
      T.doc(8, 8); beginEdit(); setPx(0, 0, 7); endEdit();
      undo(); const a = T.px(0, 0); redo(); return [a, T.px(0, 0), undoStack.length, redoStack.length];
    });
    assert.deepEqual(r, [-1, 7, 1, 0]);
  });
  test('a new edit clears redo', async () => {
    assert.equal(await run(() => { T.doc(8, 8); beginEdit(); setPx(0, 0, 7); endEdit(); undo(); beginEdit(); setPx(1, 0, 7); endEdit(); return redoStack.length; }), 0);
  });
  test('undo restores palettes and canvas size', async () => {
    const r = await run(() => {
      T.doc(16, 8); const p = S.pals[2][2];
      pushUndo(); S.pals[2][2] = 1; resizeCanvasRaw(32, 32, 0, 0);
      undo(); return [S.W, S.H, S.pals[2][2] === p, imgCanvas.width];
    });
    assert.deepEqual(r, [16, 8, true, 16]);
  });
  test('undo drops floating pixels onto the image first', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(0, 0, 7); S.sel = { x: 0, y: 0, w: 1, h: 1 }; lift(false); S.flt.x = 4;
      undo(); return [S.flt, T.px(0, 0), T.px(4, 0)];
    });
    assert.deepEqual(r, [null, 7, -1]);
  });
  test('history is capped at 200 steps', async () => {
    assert.equal(await run(() => { T.doc(8, 8); for (let i = 0; i < 250; i++) pushUndo(); return undoStack.length; }), 200);
  });
  test('history is capped at 40 million pixels', async () => {
    assert.equal(await run(() => { undoStack.length = 0; for (let i = 0; i < 10; i++) undoStack.push({ px: { length: 5e6 } }); trimUndo(); return undoStack.length; }), 8);
  });
  test('the Undo / Redo buttons follow the stacks and run undo / redo', async () => {
    const r = await run(() => {
      T.doc(8, 8); const a = [$('#bUndo').disabled, $('#bRedo').disabled];
      beginEdit(); setPx(0, 0, 7); endEdit(); const b = [$('#bUndo').disabled, $('#bRedo').disabled];
      $('#bUndo').click(); const c = [$('#bUndo').disabled, $('#bRedo').disabled, T.px(0, 0)];
      $('#bRedo').click(); return [a, b, c, T.px(0, 0)];
    });
    assert.deepEqual(r, [[true, true], [false, true], [true, false, -1], 7]);
  });
});

/* ================================================================================================ */
describe('analysis (GBC tile rules)', () => {
  test('tileKey matches a tile with its mirrored copy', async () => {
    assert.equal(await run(() => {
      const a = new Int32Array(64).fill(-1), b = new Int32Array(64).fill(-1); a[0] = 5; b[7] = 5;
      return tileKey(a, 1, 0) === tileKey(b, 0, 0) && tileKey(a, 0, 0) !== tileKey(b, 0, 0);
    }), true);
  });
  test('groupSets merges color sets into palettes of at most cap colors', async () => {
    assert.deepEqual(await run(() => groupSets([[1, 2], [2, 3], [4], []], 3).map(g => [...g].sort())), [[1, 2, 3], [4]]);
  });
  test('groupSets skips sets bigger than cap', async () => {
    assert.equal(await run(() => groupSets([[1, 2, 3, 4]], 3).length), 0);
  });
  test('tileColorSets lists each distinct set once, sorted, without transparency', async () => {
    const r = await run(() => { T.doc(24, 8); putPx(0, 0, 9); putPx(1, 0, 3); putPx(8, 0, 3); putPx(9, 0, 9); putPx(16, 0, 4); return tileColorSets(); });
    assert.deepEqual(r, [[3, 9], [4]]);
  });
  test('empty sprite sheet: nothing to check', async () => {
    assert.deepEqual(await run(() => { T.doc(16, 16); analyze(); return [A.nonEmpty, A.badN, A.colors, A.needed]; }), [0, 0, 0, 0]);
  });
  test('sprite tiles get the palette that holds their colors', async () => {
    const r = await run(() => { T.doc(16, 8); putPx(0, 0, S.pals[0][1]); putPx(1, 0, S.pals[0][3]); putPx(8, 0, S.pals[3][2]); analyze(); return [...A.tp]; });
    assert.deepEqual(r, [0, 3]);
  });
  test('sprite tile over 3 colors is flagged 1', async () => {
    const r = await run(() => { T.doc(8, 8); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(4, 0, S.pals[1][1]); analyze(); return [A.bad[0], A.over, A.badN]; });
    assert.deepEqual(r, [1, 1, 1]);
  });
  test('sprite tile whose colors are split between palettes is flagged 2', async () => {
    const r = await run(() => { T.doc(8, 8); putPx(0, 0, S.pals[0][1]); putPx(1, 0, S.pals[1][1]); analyze(); return [A.bad[0], A.nofit, A.tp[0]]; });
    assert.deepEqual(r, [2, 1, -1]);
  });
  test('counts colors, color sets and palettes needed', async () => {
    const r = await run(() => {
      T.doc(16, 8); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(8, 0, S.pals[1][1]);
      analyze(); return [A.colors, A.sets, A.needed, A.np, A.cap];
    });
    assert.deepEqual(r, [4, 2, 2, 8, 3]);
  });
  test('frame usage per row (sprite sheets)', async () => {
    const r = await run(() => { T.doc(64, 32); putPx(1, 1, 5); putPx(33, 1, 5); analyze(); return [A.rowFrames, A.framesUsed, A.fcols, A.frows]; });
    assert.deepEqual(r, [[3, 0], 2, 4, 2]);
  });
  test('map tiles: 4 colors from BG palettes 0–6', async () => {
    const r = await run(() => { T.doc(8, 8, 'map'); T.rect(0, 0, 8, 8, S.pals[2][0]); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[2][i])); analyze(); return [A.cap, A.np, A.badN, A.tp[0]]; });
    assert.deepEqual(r, [4, 7, 0, 2]);
  });
  test('map tile with 5 colors is flagged 1', async () => {
    assert.equal(await run(() => { T.doc(8, 8, 'map'); [0, 1, 2, 3].forEach(i => putPx(i, 0, S.pals[2][i])); putPx(5, 0, S.pals[4][1]); analyze(); return A.bad[0]; }), 1);
  });
  test('map tiles cannot use BG palette 7 (UI)', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8, 'map'); T.rect(0, 0, 8, 8, S.pals[7][0]); analyze(); return [A.bad[0], A.nofit]; }), [2, 1]);
  });
  test('transparent pixels in a map tile are flagged 3', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8, 'map'); putPx(0, 0, -1); analyze(); return [A.bad[0], A.transp]; }), [3, 1]);
  });
  test('unique map tiles merge X, Y and XY mirrored copies', async () => {
    const r = await run(() => {
      const u = (x, y) => { T.doc(16, 8, 'map'); putPx(0, 0, S.pals[0][3]); putPx(x, y, S.pals[0][3]); analyze(); return A.uniq; };
      return [u(8, 0), u(15, 0), u(8, 7), u(15, 7), u(9, 1)];
    });
    assert.deepEqual(r, [1, 1, 1, 1, 2]);
  });
  test('scheduleAnalyze re-runs the check after edits', async () => {
    await run(() => { T.doc(8, 8); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(4, 0, S.pals[1][1]); afterChange(); });
    await page.waitFor('A && A.over === 1');
  });
});

/* ================================================================================================ */
describe('Check panel', () => {
  test('says "Ready to export" with no problems and disables Next problem', async () => {
    const r = await run(() => { T.doc(16, 16); analyze(); renderStats(); return [$('#stats').textContent.includes('Ready to export'), $('#bNextProb').disabled, T.stat('Size').val]; });
    assert.deepEqual(r, [true, true, '16×16 px · 2×2 tiles']);
  });
  test('reports problem tiles in red', async () => {
    const r = await run(() => {
      T.doc(8, 8); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(4, 0, S.pals[1][1]); analyze(); renderStats();
      return [$('#stats').textContent.includes('1 problem tile(s)'), $('#bNextProb').disabled, T.stat('Tiles over 3 colors'), T.stat('Colors used').cls];
    });
    assert.deepEqual(r, [true, false, { val: '1', cls: 'bad' }, 'warn']);
  });
  test('warns when a sprite sheet uses more than 3 colors', async () => {
    const r = await run(() => { T.doc(16, 8); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(8, 0, S.pals[1][1]); analyze(); renderStats(); return $('#stats').textContent; });
    assert.match(r, /More than 3 colors in the sheet/);
  });
  test('flags more than 128 sprite tiles in used frames', async () => {
    const r = await run(() => { T.doc(256, 80); for (let i = 0; i < 33; i++) putPx((i % 16) * 16, ((i / 16) | 0) * 16, 5); analyze(); renderStats(); return T.stat('Sprite tiles (used frames)'); });
    assert.deepEqual(r, { val: '132 / 128', cls: 'bad' });
  });
  test('warns when the image is not a multiple of the frame size', async () => {
    assert.deepEqual(await run(() => { T.doc(64, 32); S.grid.fw = 24; analyze(); renderStats(); return T.stat('Image vs frame size'); }), { val: 'not a multiple', cls: 'warn' });
  });
  test('map sheets show unique tiles, transparency and the 1600 px limit', async () => {
    const r = await run(() => { T.doc(1608, 8, 'map'); putPx(0, 0, -1); analyze(); renderStats(); return [T.stat('Unique tiles (mirrors merged)').val, T.stat('Tiles with transparency'), T.stat('Map size')]; });
    assert.deepEqual(r, ['2 / 128', { val: '1', cls: 'bad' }, { val: 'max 1600×1600', cls: 'bad' }]);
  });
  test('Next problem cycles through bad tiles, explains them and centers the view', async () => {
    const r = await run(() => {
      T.doc(32, 16); S.pals[5][1] = S.pals[0][1];
      [1, 2, 3].forEach(i => putPx(8 + i, 0, S.pals[0][i])); putPx(12, 0, S.pals[1][1]);           /* tile (1,0): 4 colors */
      putPx(16, 8, S.pals[2][1]); putPx(17, 8, S.pals[3][1]);                                    /* tile (2,1): no palette */
      analyze(); renderStats(); S.zoom = 2;
      const out = [];
      for (let k = 0; k < 3; k++) { $('#bNextProb').click(); out.push(T.last()); }
      return [out, S.zoom >= 8, S.offX === Math.round(vw / 2 - (8 + 4) * S.zoom), flashTile && [flashTile.tx, flashTile.ty]];
    });
    assert.deepEqual(r, [['Tile (1, 0): more than 3 colors', 'Tile (2, 1): colors not all in one palette', 'Tile (1, 0): more than 3 colors'], true, true, [1, 0]]);
  });
  test('Next problem explains transparent map tiles', async () => {
    assert.equal(await run(() => { T.doc(8, 8, 'map'); putPx(0, 0, -1); analyze(); nextProblem(); return T.last(); }), 'Tile (0, 0): transparent pixels in a map tile');
  });
});

/* ================================================================================================ */
describe('palette panel', () => {
  test('sprite rows are OBJ0–7 with a transparent color 0', async () => {
    const r = await run(() => [[...document.querySelectorAll('#pals .pl')].map(e => e.textContent).join(' '), document.querySelectorAll('#pals .sw.chk').length, $('#palHint').textContent]);
    assert.equal(r[0], 'OBJ0 OBJ1 OBJ2 OBJ3 OBJ4 OBJ5 OBJ6 OBJ7');
    assert.equal(r[1], 8);
    assert.match(r[2], /Color 0 is transparent/);
  });
  test('map rows are BG0–7 and BG7 is marked as the UI palette', async () => {
    const r = await run(() => { T.doc(8, 8, 'map'); return [[...document.querySelectorAll('#pals .pl')].map(e => e.textContent).join(' '), document.querySelectorAll('#pals .prow.ui').length, document.querySelectorAll('#pals .sw.chk').length, $('#palHint').textContent]; });
    assert.deepEqual(r.slice(0, 3), ['BG0 BG1 BG2 BG3 BG4 BG5 BG6 BG7*', 1, 0]);
    assert.match(r[3], /BG7\* is reserved/);
  });
  test('left click picks the primary slot, right click the secondary', async () => {
    const r = await run(() => {
      const md = (p, i, button) => document.querySelector(`#pals .sw[data-p="${p}"][data-i="${i}"]`).dispatchEvent(new MouseEvent('mousedown', { button, bubbles: true }));
      md(3, 2, 0); md(5, 1, 2);
      return [S.prim, S.sec, !!document.querySelector('#pals .sw[data-p="3"][data-i="2"].prim'), !!document.querySelector('#pals .sw[data-p="5"][data-i="1"].sec'), document.querySelector('.prow.curp .pl').dataset.p];
    });
    assert.deepEqual(r, [{ p: 3, i: 2 }, { p: 5, i: 1 }, true, true, '3']);
  });
  test('clicking a palette label keeps the color index', async () => {
    assert.deepEqual(await run(() => { S.prim = { p: 0, i: 3 }; document.querySelector('#pals .pl[data-p="6"]').dispatchEvent(new MouseEvent('mousedown', { bubbles: true })); return S.prim; }), { p: 6, i: 3 });
  });
  test('the palette context menu is suppressed', async () => {
    assert.equal(await run(() => { const e = new MouseEvent('contextmenu', { bubbles: true, cancelable: true }); $('#pals').dispatchEvent(e); return e.defaultPrevented; }), true);
  });
  test('● marks palettes used by tiles', async () => {
    const r = await run(() => { T.doc(8, 8); putPx(0, 0, S.pals[2][1]); analyze(); renderPals(); return [...document.querySelectorAll('#pals .prow')].map(r => r.lastChild.textContent).join(''); });
    assert.equal(r, '●');
    assert.equal(await run(() => document.querySelectorAll('#pals .prow')[2].lastChild.textContent), '●');
  });
  test('the primary / secondary swatches show the current colors', async () => {
    const r = await run(() => { S.prim = { p: 1, i: 2 }; renderPals(); return [$('#curPrim').style.background.replace(/ /g, ''), css(S.pals[1][2]), $('#curSec').className]; });
    assert.equal(r[0], r[1]);
    assert.equal(r[2], 'chk');
  });
});

/* ================================================================================================ */
describe('color editor', () => {
  test('shows the selected slot as RGB555 channels, hex and RGB8', async () => {
    const r = await run(() => { S.prim = { p: 1, i: 2 }; renderPals(); const c = S.pals[1][2]; return [+$('#nR').value === R5(c), +$('#cG').value === G5(c), +$('#nB').value === B5(c), $('#cHex').value === hex(c), $('#cRgb8').textContent === `RGB8(${c5to8(R5(c))},${c5to8(G5(c))},${c5to8(B5(c))})`, $('#edSlot').textContent]; });
    assert.deepEqual(r, [true, true, true, true, true, 'palette 1 · color 2']);
  });
  test('is disabled for the transparent sprite slot', async () => {
    const r = await run(() => { S.prim = { p: 0, i: 0 }; renderPals(); return [$('#cR').disabled, $('#nG').disabled, $('#cHex').disabled, $('#cPick').disabled, $('#cRgb8').textContent, $('#edSlot').textContent]; });
    assert.deepEqual(r, [true, true, true, true, 'transparent', 'palette 0 · color 0 (transparent)']);
  });
  test('editColor ignores the transparent slot', async () => {
    assert.equal(await run(() => { S.prim = { p: 0, i: 0 }; editColor(pack(9, 9, 9)); return S.pals[0][0]; }), 0);
  });
  test('sliders edit the color; one drag is one undo step', async () => {
    const r = await run(() => {
      T.doc(8, 8); S.prim = { p: 2, i: 1 }; renderPals(); const g = G5(S.pals[2][1]), b = B5(S.pals[2][1]);
      for (const v of [10, 20, 31]) { $('#cR').value = v; $('#cR').dispatchEvent(new Event('input')); }
      $('#cR').dispatchEvent(new Event('change'));
      return [S.pals[2][1] === pack(31, g, b), undoStack.length, +$('#nR').value];
    });
    assert.deepEqual(r, [true, 1, 31]);
  });
  test('number inputs are clamped to 0–31', async () => {
    assert.equal(await run(() => { S.prim = { p: 2, i: 1 }; renderPals(); $('#nB').value = 99; $('#nB').dispatchEvent(new Event('input')); endColorEdit(); return B5(S.pals[2][1]); }), 31);
  });
  test('hex field sets the color (snapped) and reverts invalid text', async () => {
    const r = await run(() => {
      S.prim = { p: 2, i: 1 }; renderPals();
      $('#cHex').value = '#ff0000'; $('#cHex').dispatchEvent(new Event('change')); const a = S.pals[2][1];
      $('#cHex').value = 'nope'; $('#cHex').dispatchEvent(new Event('change'));
      return [a, S.pals[2][1], $('#cHex').value, undoStack.length];
    });
    assert.deepEqual(r, [31, 31, '#ff0000', 1]);
  });
  test('system color picker sets the color', async () => {
    assert.equal(await run(() => { S.prim = { p: 4, i: 3 }; renderPals(); $('#cPick').value = '#00ff00'; $('#cPick').dispatchEvent(new Event('input')); $('#cPick').dispatchEvent(new Event('change')); return S.pals[4][3]; }), 992);
  });
  test('"Recolor pixels" changes pixels of that color in tiles using the palette', async () => {
    const r = await run(() => {
      T.doc(16, 8); const c = S.pals[0][1]; S.pals[1][1] = c;
      putPx(0, 0, c); putPx(1, 0, S.pals[0][2]);        /* tile 0 → palette 0 */
      putPx(8, 0, c); putPx(9, 0, S.pals[1][2]);        /* tile 1 → palette 1 */
      S.prim = { p: 0, i: 1 }; editColor(pack(1, 2, 3)); endColorEdit();
      return [T.px(0, 0), T.px(8, 0) === c];
    });
    assert.deepEqual(r, [3137, true]);
  });
  test('recolor off leaves pixels alone', async () => {
    assert.equal(await run(() => { T.doc(8, 8); const c = S.pals[0][1]; putPx(0, 0, c); $('#recolor').checked = false; S.prim = { p: 0, i: 1 }; editColor(pack(1, 2, 3)); endColorEdit(); return T.px(0, 0) === c; }), true);
  });
  test('no recolor when the color appears twice in the palette', async () => {
    assert.equal(await run(() => { T.doc(8, 8); const c = S.pals[0][1]; S.pals[0][2] = c; putPx(0, 0, c); S.prim = { p: 0, i: 1 }; editColor(pack(1, 2, 3)); endColorEdit(); return T.px(0, 0) === c; }), true);
  });
  test('undo reverts both the palette and the recolored pixels', async () => {
    assert.equal(await run(() => { T.doc(8, 8); const c = S.pals[0][1]; putPx(0, 0, c); S.prim = { p: 0, i: 1 }; editColor(pack(1, 2, 3)); endColorEdit(); undo(); return T.px(0, 0) === c && S.pals[0][1] === c; }), true);
  });
});

/* ================================================================================================ */
describe('palette actions', () => {
  test('Extract (sprite): one palette per color group, darkest in slot 1', async () => {
    const r = await run(() => {
      T.doc(8, 8); const d = p8(10, 10, 10), m = p8(100, 100, 100), l = p8(240, 240, 240);
      putPx(0, 0, l); putPx(1, 0, d); putPx(2, 0, m);
      $('#bExtract').click(); return [S.pals[0].slice(1).join() === [d, m, l].join(), T.last(), undoStack.length];
    });
    assert.deepEqual(r, [true, 'Extracted 1 palette(s) from 1 tile color set(s).', 1]);
  });
  test('Extract (sprite) with 2 colors keeps slot 3', async () => {
    assert.equal(await run(() => { T.doc(8, 8); const keep = S.pals[0][3]; putPx(0, 0, p8(200, 0, 0)); putPx(1, 0, p8(0, 0, 40)); extractPalettes(); return S.pals[0][1] === p8(0, 0, 40) && S.pals[0][2] === p8(200, 0, 0) && S.pals[0][3] === keep; }), true);
  });
  test('Extract (map): lightest in slot 0', async () => {
    const r = await run(() => {
      T.doc(8, 8, 'map'); const cs = [p8(10, 10, 10), p8(80, 80, 80), p8(160, 160, 160), p8(250, 250, 250)];
      T.rect(0, 0, 4, 4, cs[2]); T.rect(4, 0, 4, 4, cs[0]); T.rect(0, 4, 4, 4, cs[3]); T.rect(4, 4, 4, 4, cs[1]);
      extractPalettes(); return S.pals[0].join() === [...cs].reverse().join();
    });
    assert.equal(r, true);
  });
  test('Extract reports too many palettes and tiles over the color limit', async () => {
    const r = await run(() => {
      T.doc(80, 8);
      for (let i = 1; i <= 9; i++) { putPx(i * 8 - 8, 0, pack(i, 0, 0)); putPx(i * 8 - 7, 0, pack(i, 10, 0)); putPx(i * 8 - 6, 0, pack(i, 20, 0)); }
      [0, 1, 2, 3].forEach(k => putPx(72 + k, 0, pack(0, 0, k + 1)));
      extractPalettes(); return [T.last(), S.pals[7][1]];
    });
    assert.match(r[0], /9 needed but only 8 fit/);
    assert.match(r[0], /1 tile\(s\) have more than 3 colors/);
  });
  test('Extract on an empty image changes nothing', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); extractPalettes(); return [T.last(), undoStack.length]; }), ['No colors to extract', 0]);
  });
  test('Fit to palettes snaps colors to the best palette', async () => {
    const r = await run(() => {
      T.doc(8, 8); const c = S.pals[0][2]; putPx(0, 0, pack(R5(c) + 1, G5(c), B5(c)));
      $('#bFitBest').click(); const a = [T.px(0, 0) === c, T.last()];
      fitToPalettes(false); return [...a, T.last(), undoStack.length];
    });
    assert.deepEqual(r, [true, 'Recolored with the nearest palette colors', 'Already fits', 1]);
  });
  test('Fit to current uses only the current palette', async () => {
    const r = await run(() => {
      T.doc(8, 8); const c = S.pals[0][2]; putPx(0, 0, c); S.prim = { p: 1, i: 1 };
      let best = 0, bd = 1e9; for (const q of S.pals[1].slice(1)) if (dist(c, q) < bd) { bd = dist(c, q); best = q; }
      $('#bFitCur').click(); return T.px(0, 0) === best;
    });
    assert.equal(r, true);
  });
  test('Fit only touches the selection', async () => {
    const r = await run(() => {
      T.doc(16, 8); const off = pack(R5(S.pals[0][2]) + 1, G5(S.pals[0][2]), B5(S.pals[0][2]));
      putPx(0, 0, off); putPx(8, 0, off); S.sel = { x: 0, y: 0, w: 8, h: 8 }; fitToPalettes(false);
      return [T.px(0, 0) === S.pals[0][2], T.px(8, 0) === off];
    });
    assert.deepEqual(r, [true, true]);
  });
  test('Fit fills transparent map pixels with the palette background', async () => {
    assert.equal(await run(() => { T.doc(8, 8, 'map'); putPx(0, 0, -1); fitToPalettes(false); return T.px(0, 0) === S.pals[0][0]; }), true);
  });
  test('Fit to current refuses BG palette 7', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8, 'map'); S.prim = { p: 7, i: 0 }; fitToPalettes(true); return [T.last(), undoStack.length]; }), ['BG palette 7 is the UI palette', 0]);
  });
  test('palettesAsC writes 8 lines of RGB8 values', async () => {
    const r = await run(() => {
      const t = palettesAsC(), c = S.pals[0][1];
      T.doc(8, 8, 'map'); const m = palettesAsC();
      return [t.split('\n').length, t.split('\n')[0].startsWith(`    /* OBJ 0 */ RGB8(0, 0, 0), RGB8(${c5to8(R5(c))}, ${c5to8(G5(c))}, ${c5to8(B5(c))}),`), m.split('\n')[7].startsWith('    /* BG 7 (UI) */ RGB8(')];
    });
    assert.deepEqual(r, [8, true, true]);
  });
  test('pastePalettes reads RGB8 and RGB from the current palette on', async () => {
    const r = await run(() => {
      S.prim = { p: 6, i: 1 };
      pastePalettes('RGB8(255,0,0) RGB(0,31,0), RGB( 0 , 0 , 99 ) RGB8(0,0,0) RGB(1,1,1) RGB(2,2,2) RGB(3,3,3) RGB(4,4,4) RGB(5,5,5)');
      return [S.pals[6], S.pals[7], T.last(), undoStack.length];
    });
    assert.deepEqual(r, [[31, 992, 31744, 0], [1057, 2114, 3171, 4228], 'Loaded 9 colors', 1]);
  });
  test('pastePalettes with no colors changes nothing', async () => {
    assert.deepEqual(await run(() => { pastePalettes('hello'); return [T.last(), undoStack.length]; }), ['No RGB8(...) or RGB(...) values found', 0]);
  });
  test('Copy as C and Paste C round-trip the palettes', async () => {
    const r = await run(async () => {
      let text = ''; navigator.clipboard.writeText = async t => { text = t; };
      const before = JSON.stringify(S.pals);
      $('#bPalCopy').click(); await T.wait(0);
      S.pals = defaultPals('map'); S.prim = { p: 0, i: 1 };
      $('#bPalPaste').click(); const open = $('#dPalPaste').open;
      $('#palPasteText').value = text; $('#palPasteOk').click();
      return [open, $('#dPalPaste').open, JSON.stringify(S.pals) === before, T.toasts.includes('Palettes copied as C')];
    });
    assert.deepEqual(r, [true, false, true, true]);
  });
  test('Copy as C falls back to a prompt when the clipboard fails', async () => {
    assert.equal(await run(async () => { let shown = ''; navigator.clipboard.writeText = async () => { throw new Error('denied'); }; window.prompt = (m, t) => { shown = t; }; $('#bPalCopy').click(); await T.wait(0); return shown === palettesAsC(); }), true);
  });
  test('Defaults restores the mode\'s palettes (undoable)', async () => {
    const r = await run(() => { S.pals[3][2] = 1; $('#bPalReset').click(); const a = S.pals[3][2] === defaultPals('sprite')[3][2]; undo(); return [a, S.pals[3][2]]; });
    assert.deepEqual(r, [true, 1]);
  });
});

/* ================================================================================================ */
describe('view', () => {
  test('setZoom keeps the point under the cursor', async () => {
    assert.equal(await run(() => { const ix = (300 - S.offX) / S.zoom; setZoom(16, 300, 200); return Math.abs((300 - S.offX) / S.zoom - ix) <= 1 / 16; }), true);
  });
  test('zoomStep walks the zoom list and clamps at both ends', async () => {
    const r = await run(() => {
      const z = (from, dir) => { S.zoom = from; zoomStep(dir); return S.zoom; };
      return [z(8, 1), z(8, -1), z(7, 1), z(7, -1), z(64, 1), z(1, -1)];
    });
    assert.deepEqual(r, [10, 6, 8, 6, 64, 1]);
  });
  test('fitView picks the biggest zoom that fits and centers the image', async () => {
    const r = await run(() => {
      T.doc(32, 16); S.zoom = 1; fitView();
      let z = 1; for (const l of ZL) if (32 * l <= vw - 40 && 16 * l <= vh - 40) z = l;
      return [S.zoom === z, S.offX === Math.round((vw - 32 * z) / 2), $('#zLabel').textContent === z + '×'];
    });
    assert.deepEqual(r, [true, true, true]);
  });
  test('zoom buttons', async () => {
    const r = await run(() => { S.zoom = 8; $('#zIn').click(); const a = S.zoom; $('#zOut').click(); $('#zOut').click(); const b = S.zoom; $('#zFit').click(); return [a, b, $('#zLabel').textContent === S.zoom + '×']; });
    assert.deepEqual(r, [10, 6, true]);
  });
  test('mouse wheel zooms at the cursor, Shift+wheel pans', async () => {
    await run(() => { T.doc(32, 32); setZoom(8); });
    const [x, y] = await at(4, 4);
    await page.mouse('mouseWheel', x, y, { deltaY: -100 });
    assert.equal(await run(() => S.zoom), 10);
    await page.mouse('mouseWheel', x, y, { deltaY: 100 });
    assert.equal(await run(() => S.zoom), 8);
    const ox = await run(() => S.offX);
    await page.mouse('mouseWheel', x, y, { deltaY: 100, shift: true });
    assert.deepEqual(await run(() => [S.zoom, S.offX !== 0]), [8, true]);
    assert.notEqual(await run(() => S.offX), ox);
  });
  test('render draws the image at the current zoom and offset', async () => {
    const r = await run(() => {
      T.doc(32, 32); putPx(3, 2, pack(31, 0, 0)); S.grid.problems = false; render();
      const sx = Math.floor(S.offX + 3.5 * S.zoom), sy = Math.floor(S.offY + 2.5 * S.zoom);
      const t = T.rgba(view, Math.floor(S.offX + 10.5 * S.zoom), Math.floor(S.offY + 10.5 * S.zoom));
      return [T.rgba(view, sx, sy), ['154,154,154', '200,200,200'].includes(t.slice(0, 3).join())];
    });
    assert.deepEqual(r, [[255, 0, 0, 255], true]);
  });
  test('render outlines problem tiles when "Show problems" is on', async () => {
    const r = await run(() => {
      T.doc(32, 32); T.rect(0, 0, 8, 8, pack(0, 0, 31)); [1, 2, 3].forEach(i => putPx(i, 0, S.pals[0][i])); putPx(4, 0, S.pals[1][1]); analyze();
      const sx = Math.floor(S.offX + 4.5 * S.zoom), sy = Math.floor(S.offY + 4.5 * S.zoom);
      $('#gProblems').checked = true; $('#gProblems').dispatchEvent(new Event('change')); render(); const on = T.rgba(view, sx, sy);
      $('#gProblems').checked = false; $('#gProblems').dispatchEvent(new Event('change')); render(); const off = T.rgba(view, sx, sy);
      return [on[0] > 40, off.join()];
    });
    assert.deepEqual(r, [true, '0,0,255,255']);
  });
  test('render labels tile palettes when "Tile palettes" is on', async () => {
    const r = await run(() => {
      T.doc(32, 32); T.rect(0, 0, 8, 8, S.pals[2][2]); analyze();
      const sx = S.offX + 2, sy = S.offY + 2;
      render(); const off = T.rgba(view, sx, sy).join();
      $('#gPals').checked = true; $('#gPals').dispatchEvent(new Event('change')); render();
      return [off === T.rgba(view, sx, sy).join(), S.grid.pals];
    });
    assert.deepEqual(r, [false, true]);
  });
  test('render draws floating pixels', async () => {
    assert.deepEqual(await run(() => { T.doc(32, 32); S.flt = { x: 5, y: 5, w: 1, h: 1, d: Int32Array.of(pack(0, 31, 0)) }; buildFlt(); render(); return T.rgba(view, Math.floor(S.offX + 5.5 * S.zoom), Math.floor(S.offY + 5.5 * S.zoom)); }), [0, 255, 0, 255]);
  });
  test('the status bar describes the pixel under the mouse', async () => {
    await run(() => { T.doc(32, 32); putPx(5, 2, S.pals[0][2]); analyze(); });
    await hover(3, 2);
    let s = await run(() => $('#status').textContent);
    for (const part of ['x 3, y 2', 'tile 0, 0', 'frame row 0 col 0 (#0)', 'transparent', '32×32']) assert.ok(s.includes(part), s);
    await hover(5, 2);
    s = await run(() => $('#status').textContent);
    assert.ok(s.includes(await run(() => hex(S.pals[0][2]))), s);
    assert.ok(s.includes('tile palette 0'), s);
    await run(() => { S.sel = { x: 1, y: 1, w: 4, h: 4 }; lift(false); });
    await hover(3, 2);
    assert.ok((await run(() => $('#status').textContent)).includes('selection 4×4 at 1, 1 (floating)'));
  });
});

/* ================================================================================================ */
describe('geometry', () => {
  test('linePts is a connected Bresenham line from start to end', async () => {
    const r = await run(() => {
      const ok = (x0, y0, x1, y1) => {
        const p = linePts(x0, y0, x1, y1);
        const conn = p.every((q, i) => !i || (Math.abs(q[0] - p[i - 1][0]) <= 1 && Math.abs(q[1] - p[i - 1][1]) <= 1));
        return conn && p[0].join() === `${x0},${y0}` && p[p.length - 1].join() === `${x1},${y1}` && p.length === Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0)) + 1;
      };
      return [ok(0, 0, 5, 2), ok(5, 2, 0, 0), ok(3, 9, 4, 0), ok(0, 0, -6, -6), linePts(2, 2, 2, 2)];
    });
    assert.deepEqual(r, [true, true, true, true, [[2, 2]]]);
  });
  test('rectPts outline and fill, any corner order', async () => {
    const r = await run(() => [rectPts(0, 0, 3, 2, false).length, rectPts(0, 0, 3, 2, true).length, rectPts(3, 2, 0, 0, false).map(String).sort().join() === rectPts(0, 0, 3, 2, false).map(String).sort().join(), rectPts(1, 1, 1, 1, false)]);
    assert.deepEqual(r, [10, 12, true, [[1, 1]]]);
  });
  test('ellipsePts is symmetric, touches its box, outline ⊂ fill', async () => {
    const r = await run(() => {
      const f = new Set(ellipsePts(2, 3, 9, 10, true).map(String)), o = ellipsePts(2, 3, 9, 10, false).map(String);
      const pts = [...f].map(s => s.split(',').map(Number));
      const sym = pts.every(([x, y]) => f.has(`${11 - x},${y}`) && f.has(`${x},${13 - y}`));
      const box = [Math.min(...pts.map(p => p[0])), Math.max(...pts.map(p => p[0])), Math.min(...pts.map(p => p[1])), Math.max(...pts.map(p => p[1]))];
      return [sym, box, o.every(s => f.has(s)), o.length < f.size, ellipsePts(4, 4, 4, 4, false), ellipsePts(0, 0, 4, 0, true).length];
    });
    assert.deepEqual(r, [true, [2, 9, 3, 10], true, true, [[4, 4]], 5]);
  });
  test('normRect orders the corners', async () => {
    assert.deepEqual(await run(() => normRect(5, 6, 2, 2)), { x: 2, y: 2, w: 4, h: 5 });
  });
  test('constrain: Shift snaps lines to 0/45/90° and shapes to squares', async () => {
    const r = await run(() => {
      const c = (tool, dx, dy) => { const d = { tool, x0: 10, y0: 10, x1: 10 + dx, y1: 10 + dy }; constrain(d, true); return [d.x1 - 10, d.y1 - 10]; };
      const n = { tool: 'rect', x0: 0, y0: 0, x1: 5, y1: 2 }; constrain(n, false);
      return [c('line', 10, 2), c('line', 2, 10), c('line', 5, -4), c('rect', 5, -2), c('ellipse', 0, 0), [n.x1, n.y1]];
    });
    assert.deepEqual(r, [[10, 0], [0, 10], [5, -5], [5, -5], [0, 0], [5, 2]]);
  });
});

/* ================================================================================================ */
describe('drawing operations', () => {
  test('mirrorX mirrors inside the frame, or the whole image with the frame grid off', async () => {
    assert.deepEqual(await run(() => { T.doc(64, 16); const a = [mirrorX(0), mirrorX(17)]; S.grid.frame = false; return [...a, mirrorX(0), mirrorX(17)]; }), [15, 30, 63, 46]);
  });
  test('plot paints a square brush centered on the pixel', async () => {
    const r = await run(() => {
      const n = b => { T.doc(16, 16); S.brush = b; plot(5, 5, 7); return T.count(7); };
      T.doc(16, 16); S.brush = 3; plot(5, 5, 7); const corners = [T.px(4, 4), T.px(6, 6), T.px(7, 7)];
      return [n(1), n(2), n(3), n(8), corners];
    });
    assert.deepEqual(r, [1, 4, 9, 64, [7, 7, -1]]);
  });
  test('plot with Mirror X also paints the mirrored pixel', async () => {
    assert.deepEqual(await run(() => { T.doc(32, 16); S.mirror = true; plot(2, 3, 7); return [T.px(2, 3), T.px(13, 3), T.count(7)]; }), [7, 7, 2]);
  });
  test('floodFill fills the connected area only', async () => {
    const r = await run(() => { T.doc(16, 16); T.rect(8, 0, 1, 16, 3); floodFill(0, 0, 7, false); return [T.count(7), T.px(12, 12)]; });
    assert.deepEqual(r, [128, -1]);
  });
  test('floodFill global fills every pixel of that color', async () => {
    assert.equal(await run(() => { T.doc(16, 16); T.rect(8, 0, 1, 16, 3); floodFill(0, 0, 7, true); return T.count(7); }), 240);
  });
  test('floodFill stays inside the selection and ignores clicks outside it', async () => {
    const r = await run(() => {
      T.doc(16, 16); S.sel = { x: 0, y: 0, w: 4, h: 4 }; floodFill(1, 1, 7, false); const a = T.count(7);
      floodFill(10, 10, 8, false); return [a, T.count(8)];
    });
    assert.deepEqual(r, [16, 0]);
  });
  test('floodFill with the same color or outside the image does nothing', async () => {
    assert.equal(await run(() => { T.doc(8, 8); changed = false; floodFill(0, 0, -1, false); floodFill(-1, 0, 5, false); floodFill(0, 8, 5, false); return changed; }), false);
  });
  test('pick selects the slot holding the color, preferring that button\'s current palette', async () => {
    const r = await run(() => {
      T.doc(8, 8); const c = S.pals[3][2]; S.pals[5][1] = c; putPx(0, 0, c);
      pick(0, 0, 0, false); const a = { ...S.prim };
      S.prim = { p: 5, i: 3 }; pick(0, 0, 0, false); const b = { ...S.prim };
      pick(0, 0, 2, false); return [a, b, S.sec];
    });
    assert.deepEqual(r, [{ p: 3, i: 2 }, { p: 5, i: 1 }, { p: 3, i: 2 }]);
  });
  test('pick on a transparent pixel: sprite → color 0, map → message', async () => {
    const r = await run(() => {
      T.doc(8, 8); S.prim = { p: 4, i: 2 }; pick(1, 1, 0, false); const a = { ...S.prim };
      T.doc(8, 8, 'map'); putPx(1, 1, -1); pick(1, 1, 0, false); return [a, T.last()];
    });
    assert.deepEqual(r, [{ p: 4, i: 0 }, 'Transparent pixel']);
  });
  test('pick of a color in no palette explains Shift+click', async () => {
    assert.match(await run(() => { T.doc(8, 8); putPx(0, 0, pack(1, 2, 3)); pick(0, 0, 0, false); return T.last(); }), /#081018 is not in any palette/);
  });
  test('pick with write puts the color into the selected slot (undoable)', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(0, 0, pack(1, 2, 3)); S.prim = { p: 1, i: 2 }; pick(0, 0, 0, true); const a = S.pals[1][2];
      S.prim = { p: 1, i: 0 }; pick(0, 0, 0, true); const b = T.last();
      pick(5, 5, 0, true); return [a, b, T.last(), undoStack.length];
    });
    assert.deepEqual(r, [3137, 'Select a visible slot (1–3) first', 'Transparent pixel', 1]);
  });
  test('pick reads floating pixels on top of the image', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); S.flt = { x: 0, y: 0, w: 1, h: 1, d: Int32Array.of(S.pals[6][3]) }; pick(0, 0, 0, false); return S.prim; }), { p: 6, i: 3 });
  });
  test('pick outside the image does nothing', async () => {
    assert.deepEqual(await run(() => { pick(-1, 0, 0, false); pick(0, 999, 0, false); return S.prim; }), { p: 0, i: 1 });
  });
});

/* ================================================================================================ */
describe('selection & clipboard', () => {
  test('lift cuts the selection into floating pixels', async () => {
    const r = await run(() => { T.doc(8, 8); putPx(1, 1, 7); S.sel = { x: 1, y: 1, w: 2, h: 2 }; lift(false); return [T.px(1, 1), S.flt.w, S.flt.h, [...S.flt.d], undoStack.length]; });
    assert.deepEqual(r, [-1, 2, 2, [7, -1, -1, -1], 1]);
  });
  test('lift(copy) leaves the image alone', async () => {
    assert.equal(await run(() => { T.doc(8, 8); putPx(1, 1, 7); S.sel = { x: 1, y: 1, w: 2, h: 2 }; lift(true); return T.px(1, 1); }), 7);
  });
  test('commitFloating drops opaque pixels, clips, and selects what landed', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(7, 7, 3);
      S.flt = { x: 6, y: 6, w: 3, h: 3, d: Int32Array.of(7, 7, 7, 7, -1, 7, 7, 7, 7) }; commitFloating();
      const a = [T.px(6, 6), T.px(7, 7), S.sel];
      S.flt = { x: -5, y: 0, w: 2, h: 2, d: Int32Array.of(1, 1, 1, 1) }; commitFloating();
      return [...a, S.sel, S.flt];
    });
    assert.deepEqual(r, [7, 3, { x: 6, y: 6, w: 2, h: 2 }, null, null]);
  });
  test('Select all / None buttons', async () => {
    const r = await run(() => { T.doc(16, 8); $('#bSelAll').click(); const a = S.sel; $('#bDesel').click(); return [a, S.sel]; });
    assert.deepEqual(r, [{ x: 0, y: 0, w: 16, h: 8 }, null]);
  });
  test('deselect drops floating pixels first', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); S.flt = { x: 2, y: 2, w: 1, h: 1, d: Int32Array.of(7) }; deselect(); return [S.flt, S.sel, T.px(2, 2)]; }), [null, null, 7]);
  });
  test('regionData reads the floating pixels, the selection or the whole image', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(1, 1, 7); const all = regionData();
      S.sel = { x: 1, y: 1, w: 2, h: 1 }; const sel = regionData();
      lift(false); S.flt.d[1] = 9; const flt = regionData();
      return [all.w * all.h, [...sel.d], [...flt.d]];
    });
    assert.deepEqual(r, [64, [7, -1], [7, 9]]);
  });
  test('Copy stores the selection in the internal clipboard', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); putPx(2, 2, 7); S.sel = { x: 2, y: 2, w: 4, h: 4 }; $('#bCopy').click(); return [S.clip.w, S.clip.h, S.clip.d[0], T.last(), T.px(2, 2)]; }), [4, 4, 7, 'Copied 4×4', 7]);
  });
  test('Delete clears the selection (undoable)', async () => {
    const r = await run(() => { T.doc(8, 8); T.rect(0, 0, 8, 8, 7); S.sel = { x: 0, y: 0, w: 2, h: 2 }; updateButtons(); $('#bDel').click(); const a = T.count(7); undo(); return [a, T.count(7)]; });
    assert.deepEqual(r, [60, 64]);
  });
  test('Delete discards floating pixels', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); S.flt = { x: 0, y: 0, w: 1, h: 1, d: Int32Array.of(7) }; deleteSel(); return [S.flt, T.px(0, 0)]; }), [null, -1]);
  });
  test('Delete / Cut with nothing selected say so', async () => {
    assert.deepEqual(await run(() => { deleteSel(); const a = T.last(); cutSel(); return [a, T.last(), S.clip]; }), ['Nothing selected', 'Nothing selected', null]);
  });
  test('Cut copies then clears', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); putPx(0, 0, 7); S.sel = { x: 0, y: 0, w: 1, h: 1 }; updateButtons(); $('#bCut').click(); return [S.clip.d[0], T.px(0, 0)]; }), [7, -1]);
  });
  test('pasteData floats the clip at the selection and switches to Move', async () => {
    const r = await run(() => { T.doc(16, 16); S.sel = { x: 5, y: 6, w: 2, h: 2 }; pasteData({ w: 2, h: 1, d: Int32Array.of(7, 8) }); return [S.flt.x, S.flt.y, [...S.flt.d], S.tool]; });
    assert.deepEqual(r, [5, 6, [7, 8], 'move']);
  });
  test('pasteData at an explicit point', async () => {
    assert.deepEqual(await run(() => { T.doc(16, 16); pasteData({ w: 1, h: 1, d: Int32Array.of(7) }, { x: 9, y: 3 }); return [S.flt.x, S.flt.y]; }), [9, 3]);
  });
  test('pasteData grows the canvas to fit a bigger clip', async () => {
    assert.deepEqual(await run(() => { T.doc(8, 8); pasteData({ w: 20, h: 3, d: new Int32Array(60) }); return [S.W, S.H, T.last()]; }), [24, 8, 'Canvas grew to 24×8 to fit the paste']);
  });
  test('Paste button uses an image from the system clipboard', async () => {
    const r = await run(async () => {
      T.doc(16, 16); const blob = await T.png(3, 2, () => [255, 0, 0, 255]);
      navigator.clipboard.read = async () => [{ types: ['image/png'], getType: async () => blob }];
      await pasteButton(); return [S.flt.w, S.flt.h, S.flt.d[0]];
    });
    assert.deepEqual(r, [3, 2, 31]);
  });
  test('Paste button falls back to the internal clipboard, or says there is nothing', async () => {
    const r = await run(async () => {
      navigator.clipboard.read = async () => { throw new Error('denied'); };
      await pasteButton(); const a = T.last();
      T.doc(16, 16); S.clip = { w: 1, h: 1, d: Int32Array.of(7) }; await pasteButton(); return [a, S.flt && S.flt.d[0]];
    });
    assert.deepEqual(r, ['Nothing to paste (Ctrl+V pastes images from other apps)', 7]);
  });
  test('nudge lifts the selection and moves it', async () => {
    const r = await run(() => { T.doc(16, 16); putPx(0, 0, 7); S.sel = { x: 0, y: 0, w: 2, h: 2 }; nudge(3, 1); const a = [S.flt.x, S.flt.y]; commitFloating(); return [a, T.px(3, 1), T.px(0, 0)]; });
    assert.deepEqual(r, [[3, 1], 7, -1]);
  });
  test('nudge with nothing selected does nothing', async () => {
    assert.equal(await run(() => { nudge(1, 0); return S.flt; }), null);
  });
  test('sameClip compares size and pixels', async () => {
    assert.deepEqual(await run(() => { const a = { w: 2, h: 1, d: Int32Array.of(1, 2) }; return [!!sameClip(a, { w: 2, h: 1, d: Int32Array.of(1, 2) }), !!sameClip(a, { w: 2, h: 1, d: Int32Array.of(1, 3) }), !!sameClip(a, { w: 1, h: 2, d: Int32Array.of(1, 2) }), !!sameClip(a, null)]; }), [true, false, false, false]);
  });
});

/* ================================================================================================ */
describe('transforms', () => {
  test('transformData flips and rotates', async () => {
    const r = await run(() => {
      const d = Int32Array.of(1, 2, 3, 4, 5, 6);   /* 3×2:  1 2 3 / 4 5 6 */
      return ['flipH', 'flipV', 'cw', 'ccw'].map(k => { const t = transformData(d, 3, 2, k); return [t.w, t.h, [...t.d]]; });
    });
    assert.deepEqual(r, [[3, 2, [3, 2, 1, 6, 5, 4]], [3, 2, [4, 5, 6, 1, 2, 3]], [2, 3, [4, 1, 5, 2, 6, 3]], [2, 3, [3, 6, 2, 5, 1, 4]]]);
  });
  test('transform buttons act on the whole image when nothing is selected', async () => {
    const r = await run(() => {
      T.doc(16, 8); putPx(0, 0, 7);
      $('#bFlipH').click(); const h = T.px(15, 0);
      $('#bFlipV').click(); const v = T.px(15, 7);
      $('#bRotCW').click(); const cw = [S.W, S.H, T.px(0, 15)];
      $('#bRotCCW').click(); return [h, v, cw, [S.W, S.H, T.px(15, 7)], undoStack.length];
    });
    assert.deepEqual(r, [7, 7, [8, 16, 7], [16, 8, 7], 4]);
  });
  test('rotating the whole image is undoable back to the old size', async () => {
    assert.deepEqual(await run(() => { T.doc(16, 8); applyTransform('cw'); undo(); return [S.W, S.H]; }), [16, 8]);
  });
  test('transforms on a selection float it and stay centered', async () => {
    const r = await run(() => {
      T.doc(16, 16); putPx(4, 4, 7); S.sel = { x: 4, y: 4, w: 4, h: 2 };
      applyTransform('flipH'); const a = [S.flt.x, S.flt.d[3]];
      applyTransform('cw'); return [a, S.flt.x, S.flt.y, S.flt.w, S.flt.h];
    });
    assert.deepEqual(r, [[4, 7], 5, 3, 2, 4]);
  });
  test('Shift (wrap) moves pixels around the image', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(7, 0, 7); putPx(0, 0, 5);
      document.querySelector('[data-shift="1,0"]').click(); const a = [T.px(0, 0), T.px(1, 0)];
      document.querySelector('[data-shift="0,-1"]').click(); return [a, T.px(0, 7), T.px(1, 7), undoStack.length];
    });
    assert.deepEqual(r, [[7, 5], 7, 5, 2]);
  });
  test('Shift (wrap) by 8 px, inside the selection only', async () => {
    const r = await run(() => {
      T.doc(16, 8); putPx(0, 0, 7); putPx(12, 0, 5); S.sel = { x: 0, y: 0, w: 8, h: 8 };
      $('#shiftAmt').value = '8'; document.querySelector('[data-shift="0,1"]').click();
      shiftWrap(3, 0); return [T.px(3, 0), T.px(12, 0)];
    });
    assert.deepEqual(r, [7, 5]);
  });
});

/* ================================================================================================ */
describe('canvas size', () => {
  test('resizeCanvasRaw anchors top-left, center and bottom-right', async () => {
    const r = await run(() => {
      const t = (ax, ay) => { T.doc(16, 16); putPx(0, 0, 7); resizeCanvasRaw(32, 32, ax, ay); return S.px.indexOf(7); };
      return [t(0, 0), t(0.5, 0.5), t(1, 1)];
    });
    assert.deepEqual(r, [0, 8 * 32 + 8, 16 * 32 + 16]);
  });
  test('shrinking crops; new map pixels use the background color; selection clears', async () => {
    const r = await run(() => {
      T.doc(16, 16); putPx(10, 0, 7); S.sel = { x: 0, y: 0, w: 2, h: 2 }; resizeCanvasRaw(8, 8, 0, 0); const a = [T.count(7), S.sel, S.W];
      T.doc(8, 8, 'map'); resizeCanvasRaw(16, 8, 0, 0); return [...a, T.px(15, 0) === S.pals[0][0]];
    });
    assert.deepEqual(r, [0, null, 8, true]);
  });
  test('Resize dialog: size rounded to 8, 9 anchors, undoable', async () => {
    const r = await run(() => {
      T.doc(32, 32); putPx(0, 0, 7);
      $('#bResize').click(); const open = $('#dResize').open, pre = [$('#rW').value, $('#rH').value];
      const radios = document.querySelectorAll('input[name=anc]');
      $('#rW').value = 45; $('#rH').value = 40;
      document.querySelector('input[name=anc][value="1,1"]').checked = true; $('#rOk').click();
      const a = [S.W, S.H, S.px.indexOf(7) === 8 * S.W + 16, $('#dResize').open];
      undo(); return [open, pre, radios.length, a, [S.W, S.H]];
    });
    assert.deepEqual(r, [true, ['32', '32'], 9, [48, 40, true, false], [32, 32]]);
  });
  test('Resize dialog rejects sizes over 1600', async () => {
    assert.deepEqual(await run(() => { $('#bResize').click(); $('#rW').value = 2000; $('#rOk').click(); return [T.last(), $('#dResize').open, S.W]; }), ['Width and height: 8 to 1600, multiples of 8', true, 128]);
  });
  test('Crop to selection rounds out to the 8×8 grid', async () => {
    const r = await run(() => { T.doc(32, 32); putPx(8, 0, 7); S.sel = { x: 10, y: 2, w: 4, h: 4 }; updateButtons(); $('#bCrop').click(); return [S.W, S.H, T.px(0, 0), S.sel]; });
    assert.deepEqual(r, [8, 8, 7, null]);
  });
  test('Crop drops floating pixels first and is undoable', async () => {
    const r = await run(() => { T.doc(32, 32); putPx(0, 0, 7); S.sel = { x: 0, y: 0, w: 1, h: 1 }; lift(false); S.flt.x = 9; cropToSel(); const a = [S.W, S.H, T.px(1, 0)]; undo(); return [a, S.W]; });
    assert.deepEqual(r, [[8, 8, 7], 32]);
  });
  test('Crop with no selection says so', async () => {
    assert.equal(await run(() => { cropToSel(); return T.last(); }), 'Select an area first');
  });
});

/* ================================================================================================ */
describe('mouse drawing', () => {
  beforeEach(() => run(() => T.doc(32, 32)));

  test('pencil drag draws a continuous line as one undo step', async () => {
    await drag([[1, 1], [6, 1]]);
    const r = await run(() => [[1, 2, 3, 4, 5, 6].every(x => T.px(x, 1) === S.pals[0][1]), T.count(S.pals[0][1]), undoStack.length]);
    assert.deepEqual(r, [true, 6, 1]);
  });
  test('right button draws the secondary color', async () => {
    await run(() => { S.sec = { p: 2, i: 3 }; });
    await click(4, 4, { button: 'right' });
    assert.equal(await run(() => T.px(4, 4) === S.pals[2][3]), true);
  });
  test('eraser makes pixels transparent', async () => {
    await run(() => { T.rect(0, 0, 32, 32, 7); setTool('eraser'); });
    await click(4, 4);
    assert.deepEqual(await run(() => [T.px(4, 4), T.count(7)]), [-1, 1023]);
  });
  test('Shift+click draws a straight line from the last point', async () => {
    await click(1, 1);
    await click(9, 1, { shift: true });
    assert.equal(await run(() => T.count(S.pals[0][1])), 9);
  });
  test('brush size and Mirror X apply to mouse strokes', async () => {
    await run(() => { $('#brush').value = '3'; $('#brush').dispatchEvent(new Event('change')); $('#mirror').checked = true; $('#mirror').dispatchEvent(new Event('change')); });
    await click(5, 5);
    assert.deepEqual(await run(() => [S.brush, S.mirror, T.count(S.pals[0][1]), T.px(10, 5) === S.pals[0][1]]), [3, true, 18, true]);
  });
  test('drawing is clipped to the selection', async () => {
    await run(() => { S.sel = { x: 0, y: 0, w: 4, h: 4 }; });
    await drag([[2, 2], [8, 2]]);
    assert.equal(await run(() => T.count(S.pals[0][1])), 2);
  });
  test('line tool previews during the drag and draws on release', async () => {
    await run(() => setTool('line'));
    const pts = [await at(0, 0), await at(5, 5)];
    await page.mouse('mousePressed', ...pts[0]);
    await page.mouse('mouseMoved', ...pts[1]);
    assert.deepEqual(await run(() => [T.count(S.pals[0][1]), drag.type]), [0, 'shape']);
    await page.mouse('mouseReleased', ...pts[1]);
    assert.equal(await run(() => [0, 1, 2, 3, 4, 5].every(i => T.px(i, i) === S.pals[0][1]) && T.count(S.pals[0][1]) === 6), true);
  });
  for (const [tool, n] of [['rect', 16], ['rectf', 25], ['ellipse', null], ['ellipsef', null]]) {
    test(`${tool} tool draws its shape`, async () => {
      await run(t => setTool(t), tool);
      await drag([[2, 2], [6, 6]]);
      const r = await run(t => [T.count(S.pals[0][1]), ellipsePts(2, 2, 6, 6, t === 'ellipsef').length, rectPts(2, 2, 6, 6, t === 'rectf').length], tool);
      assert.equal(r[0], n !== null ? n : r[1]);
      if (n !== null) assert.equal(r[0], r[2]);
    });
  }
  test('Shift+drag draws a square', async () => {
    await run(() => setTool('rectf'));
    await drag([[2, 2], [6, 4]], { shift: true });
    assert.equal(await run(() => T.count(S.pals[0][1])), 25);
  });
  test('fill tool fills the area; Shift or "Global fill" fills every match', async () => {
    await run(() => { T.rect(8, 0, 1, 32, 3); setTool('fill'); });
    await click(0, 0);
    assert.equal(await run(() => T.count(S.pals[0][1])), 256);
    await run(() => { T.doc(32, 32); T.rect(8, 0, 1, 32, 3); });
    await click(0, 0, { shift: true });
    assert.equal(await run(() => T.count(S.pals[0][1])), 1024 - 32);
    await run(() => { T.doc(32, 32); T.rect(8, 0, 1, 32, 3); $('#fillGlobal').checked = true; $('#fillGlobal').dispatchEvent(new Event('change')); });
    await click(0, 0);
    assert.equal(await run(() => T.count(S.pals[0][1])), 1024 - 32);
  });
  test('picker tool: click picks primary, right click secondary, drag keeps picking', async () => {
    await run(() => { putPx(1, 1, S.pals[2][3]); putPx(5, 1, S.pals[4][2]); setTool('picker'); });
    await click(1, 1);
    assert.deepEqual(await run(() => S.prim), { p: 2, i: 3 });
    await click(5, 1, { button: 'right' });
    assert.deepEqual(await run(() => S.sec), { p: 4, i: 2 });
    await drag([[5, 1], [3, 1], [1, 1]]);
    assert.deepEqual(await run(() => S.prim), { p: 2, i: 3 });
  });
  test('Shift+click with the picker writes the color into the slot', async () => {
    await run(() => { putPx(1, 1, pack(3, 4, 5)); S.prim = { p: 1, i: 2 }; setTool('picker'); });
    await click(1, 1, { shift: true });
    assert.equal(await run(() => S.pals[1][2]), pack5(3, 4, 5));
  });
  test('Alt+click picks with any tool without drawing', async () => {
    await run(() => putPx(1, 1, S.pals[3][1]));
    await click(4, 4, { alt: true });
    await click(1, 1, { alt: true });
    assert.deepEqual(await run(() => [S.prim, T.px(4, 4)]), [{ p: 3, i: 1 }, -1]);
  });
  test('select tool: drag makes a selection, a click clears it', async () => {
    await run(() => setTool('select'));
    await drag([[5, 6], [2, 2]]);
    assert.deepEqual(await run(() => S.sel), { x: 2, y: 2, w: 4, h: 5 });
    await click(20, 20);
    assert.equal(await run(() => S.sel), null);
  });
  test('dragging inside the selection moves it; Ctrl+drag moves a copy', async () => {
    await run(() => { putPx(2, 2, 7); S.sel = { x: 2, y: 2, w: 2, h: 2 }; setTool('select'); });
    await drag([[2, 2], [5, 2]]);
    assert.deepEqual(await run(() => { T.key('Enter'); return [T.px(2, 2), T.px(5, 2), S.sel]; }), [-1, 7, { x: 5, y: 2, w: 2, h: 2 }]);
    await drag([[5, 2], [5, 6]], { ctrl: true });
    assert.deepEqual(await run(() => { commitFloating(); return [T.px(5, 2), T.px(5, 6)]; }), [7, 7]);
  });
  test('a floating selection can be dragged again before dropping it', async () => {
    await run(() => { putPx(2, 2, 7); S.sel = { x: 2, y: 2, w: 1, h: 1 }; lift(false); setTool('move'); });
    await drag([[2, 2], [4, 2]]);
    await drag([[4, 2], [4, 5]]);
    assert.deepEqual(await run(() => [S.flt.x, S.flt.y]), [4, 5]);
  });
  test('move tool with no selection moves the whole image', async () => {
    await run(() => { putPx(0, 0, 7); setTool('move'); });
    await drag([[10, 10], [13, 12]]);
    assert.deepEqual(await run(() => { commitFloating(); return [T.px(0, 0), T.px(3, 2)]; }), [-1, 7]);
  });
  test('drawing with another tool drops floating pixels first', async () => {
    await run(() => { S.flt = { x: 9, y: 9, w: 1, h: 1, d: Int32Array.of(7) }; buildFlt(); S.tool = 'pencil'; });
    await click(1, 1);
    assert.deepEqual(await run(() => [S.flt, T.px(9, 9)]), [null, 7]);
  });
  test('hand tool, middle button and Space+drag pan the view without drawing', async () => {
    for (const how of ['hand', 'middle', 'space']) {
      await run(h => { T.doc(32, 32); if (h === 'hand') setTool('hand'); if (h === 'space') T.key(' '); }, how);
      const before = await run(() => [S.offX, S.offY, S.zoom]);
      await drag([[2, 2], [6, 3]], how === 'middle' ? { button: 'middle' } : {});
      const after = await run(() => [S.offX, S.offY, T.count(S.pals[0][1])]);
      assert.deepEqual(after, [before[0] + 4 * before[2], before[1] + before[2], 0], how);
    }
  });
  test('the canvas context menu is suppressed', async () => {
    assert.equal(await run(() => { const e = new MouseEvent('contextmenu', { bubbles: true, cancelable: true }); view.dispatchEvent(e); return e.defaultPrevented; }), true);
  });
});

/* ================================================================================================ */
describe('keyboard shortcuts', () => {
  test('tool keys B E L R F O P G I M V H', async () => {
    const r = await run(() => [...'belrfopgimvh'].map(k => { T.key(k); return S.tool; }));
    assert.deepEqual(r, ['pencil', 'eraser', 'line', 'rect', 'rectf', 'ellipse', 'ellipsef', 'fill', 'picker', 'select', 'move', 'hand']);
  });
  test('Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z and Cmd+Z', async () => {
    const r = await run(() => {
      T.doc(8, 8); beginEdit(); setPx(0, 0, 7); endEdit();
      T.key('z', { ctrlKey: true }); const a = T.px(0, 0);
      T.key('y', { ctrlKey: true }); const b = T.px(0, 0);
      T.key('z', { metaKey: true }); T.key('Z', { ctrlKey: true, shiftKey: true }); return [a, b, T.px(0, 0)];
    });
    assert.deepEqual(r, [-1, 7, 7]);
  });
  test('Ctrl+A, Ctrl+D, Ctrl+C, Ctrl+X', async () => {
    const r = await run(() => {
      T.doc(8, 8); putPx(0, 0, 7);
      const p = T.key('a', { ctrlKey: true }); const a = S.sel;
      T.key('c', { ctrlKey: true }); const c = S.clip && S.clip.w;
      T.key('d', { ctrlKey: true }); const d = S.sel;
      S.sel = { x: 0, y: 0, w: 1, h: 1 }; T.key('x', { ctrlKey: true });
      return [p, a, c, d, T.px(0, 0), S.clip.w];
    });
    assert.deepEqual(r, [true, { x: 0, y: 0, w: 8, h: 8 }, 8, null, -1, 1]);
  });
  test('Ctrl+S exports, Ctrl+O opens the file chooser', async () => {
    const r = await run(() => { let s = 0, o = 0; exportPng = () => { s++; }; $('#fileIn').click = () => { o++; }; T.key('s', { ctrlKey: true }); T.key('o', { ctrlKey: true }); return [s, o]; });
    assert.deepEqual(r, [1, 1]);
  });
  test('Shift+H / V / R / Q flip and rotate', async () => {
    const r = await run(() => {
      T.doc(16, 8); putPx(0, 0, 7);
      T.key('H', { shiftKey: true }); const h = T.px(15, 0);
      T.key('V', { shiftKey: true }); const v = T.px(15, 7);
      T.key('R', { shiftKey: true }); const rr = [S.W, S.H];
      T.key('Q', { shiftKey: true }); return [h, v, rr, [S.W, S.H]];
    });
    assert.deepEqual(r, [7, 7, [8, 16], [16, 8]]);
  });
  test('arrows nudge 1 px, Shift+arrows 8 px', async () => {
    const r = await run(() => {
      T.doc(32, 32); S.sel = { x: 8, y: 8, w: 2, h: 2 };
      T.key('ArrowRight'); T.key('ArrowDown'); const a = [S.flt.x, S.flt.y];
      T.key('ArrowLeft', { shiftKey: true }); T.key('ArrowUp', { shiftKey: true }); return [a, [S.flt.x, S.flt.y]];
    });
    assert.deepEqual(r, [[9, 9], [1, 1]]);
  });
  test('Delete / Backspace, Enter, Escape', async () => {
    const r = await run(() => {
      T.doc(8, 8); T.rect(0, 0, 8, 8, 7);
      S.sel = { x: 0, y: 0, w: 1, h: 1 }; T.key('Delete'); S.sel = { x: 1, y: 0, w: 1, h: 1 }; T.key('Backspace'); const del = T.count(7);
      S.sel = { x: 4, y: 4, w: 1, h: 1 }; nudge(1, 0); T.key('Enter'); const ent = [S.flt, T.px(4, 4), !!S.sel];
      T.key('Escape'); return [del, ent, S.sel];
    });
    assert.deepEqual(r, [62, [null, -1, true], null]);
  });
  test('X swaps colors, 1–4 pick a color index', async () => {
    const r = await run(() => { S.prim = { p: 2, i: 1 }; S.sec = { p: 3, i: 3 }; T.key('x'); const a = [S.prim, S.sec]; T.key('4'); const b = S.prim.i; T.key('1'); return [a, b, S.prim]; });
    assert.deepEqual(r, [[{ p: 3, i: 3 }, { p: 2, i: 1 }], 3, { p: 3, i: 0 }]);
  });
  test('[ and ] change the brush size within 1–8', async () => {
    const r = await run(() => { const o = []; T.key('['); o.push(S.brush); for (let i = 0; i < 3; i++) T.key(']'); o.push(S.brush, +$('#brush').value); for (let i = 0; i < 9; i++) T.key(']'); o.push(S.brush); return o; });
    assert.deepEqual(r, [1, 4, 4, 8]);
  });
  test('+ = - _ 0 zoom', async () => {
    const r = await run(() => { S.zoom = 8; T.key('+'); const a = S.zoom; T.key('='); const b = S.zoom; T.key('-'); T.key('_'); const c = S.zoom; T.key('0'); fitView(); const d = S.zoom; S.zoom = 1; T.key('0'); return [a, b, c, S.zoom === d]; });
    assert.deepEqual(r, [10, 12, 8, true]);
  });
  test('T toggles the 8×8 grid and its checkbox', async () => {
    assert.deepEqual(await run(() => { T.key('t'); const a = [S.grid.tile, $('#gTile').checked]; T.key('T'); return [a, S.grid.tile]; }), [[false, false], true]);
  });
  test('Space holds the hand; releasing it or leaving the window ends it', async () => {
    const r = await run(() => {
      T.key(' '); const a = [spaceDown, view.classList.contains('pan')];
      document.dispatchEvent(new KeyboardEvent('keyup', { key: ' ' })); const b = spaceDown;
      T.key(' '); window.dispatchEvent(new Event('blur')); return [a, b, spaceDown, view.classList.contains('pan')];
    });
    assert.deepEqual(r, [[true, true], false, false, false]);
  });
  test('Alt+key and Shift+tool key do not change the tool', async () => {
    assert.equal(await run(() => { T.key('e', { altKey: true }); T.key('E', { shiftKey: true }); return S.tool; }), 'pencil');
  });
  test('keys typed in inputs are ignored; Escape leaves the input', async () => {
    const r = await run(() => { const n = $('#name'); n.focus(); T.key('e', {}, n); const a = S.tool; T.key('Escape', {}, n); return [a, document.activeElement === n]; });
    assert.deepEqual(r, ['pencil', false]);
  });
  test('keys are ignored while a dialog is open', async () => {
    assert.equal(await run(() => { $('#help').showModal(); T.key('e'); return S.tool; }), 'pencil');
  });
});

/* ================================================================================================ */
describe('tool bar and options', () => {
  test('clicking a tool selects it and highlights only that button', async () => {
    const r = await run(() => { document.querySelector('#tools button[data-tool="ellipse"]').click(); return [S.tool, [...document.querySelectorAll('#tools button.on')].map(b => b.dataset.tool)]; });
    assert.deepEqual(r, ['ellipse', ['ellipse']]);
  });
  test('every tool button has an icon and a title', async () => {
    assert.equal(await run(() => [...document.querySelectorAll('#tools button[data-tool]')].every(b => b.title && b.querySelector('svg').innerHTML.length > 10)), true);
  });
  test('clicking the color swatches swaps primary and secondary', async () => {
    assert.deepEqual(await run(() => { S.sec = { p: 4, i: 2 }; document.querySelector('.cur').click(); return [S.prim, S.sec]; }), [{ p: 4, i: 2 }, { p: 0, i: 1 }]);
  });
  test('hand tool shows the grab cursor', async () => {
    assert.deepEqual(await run(() => { setTool('hand'); const a = view.classList.contains('pan'); setTool('pencil'); return [a, view.classList.contains('pan')]; }), [true, false]);
  });
  test('changing tool drops floating pixels, except select / move / hand', async () => {
    const r = await run(() => {
      const f = () => { S.flt = { x: 0, y: 0, w: 1, h: 1, d: Int32Array.of(7) }; };
      const keep = ['select', 'move', 'hand'].map(t => { f(); setTool(t); return !!S.flt; });
      f(); setTool('line', true); const kept = !!S.flt;
      setTool('pencil'); return [keep, kept, S.flt];
    });
    assert.deepEqual(r, [[true, true, true], true, null]);
  });
  test('selection buttons are enabled only with a selection', async () => {
    const r = await run(() => {
      const st = () => ['#bDel', '#bCut', '#bCrop', '#bDesel'].map(id => $(id).disabled);
      const a = st(); selectAll(); const b = st(); return [a, b];
    });
    assert.deepEqual(r, [[true, true, true, true], [false, false, false, false]]);
  });
  test('Undo is enabled while pixels float', async () => {
    assert.equal(await run(() => { S.flt = { x: 0, y: 0, w: 1, h: 1, d: Int32Array.of(7) }; updateButtons(); return $('#bUndo').disabled; }), false);
  });
  test('grid checkboxes update the view settings', async () => {
    const r = await run(() => ['gPixel', 'gTile', 'gProblems', 'gPals', 'gFrame'].map(id => { const e = $('#' + id); e.checked = !e.checked; e.dispatchEvent(new Event('change')); return e.checked; }).join() === [S.grid.pixel, S.grid.tile, S.grid.problems, S.grid.pals, S.grid.frame].join());
    assert.equal(r, true);
  });
  test('frame size inputs round to multiples of 8 (min 8)', async () => {
    const r = await run(() => {
      const set = (w, h) => { $('#fw').value = w; $('#fh').value = h; $('#fw').dispatchEvent(new Event('change')); return [S.grid.fw, S.grid.fh, +$('#fw').value]; };
      return [set(20, 32), set(3, ''), set(64, 13)];
    });
    assert.deepEqual(r, [[24, 32, 24], [8, 8, 8], [64, 16, 64]]);
  });
  test('mode switch changes the rules and hides the preview in map mode', async () => {
    const r = await run(() => { $('#mode').value = 'map'; $('#mode').dispatchEvent(new Event('change')); const a = [S.mode, $('#pvPanel').style.display, document.querySelector('#pals .pl').textContent]; $('#mode').value = 'sprite'; $('#mode').dispatchEvent(new Event('change')); return [a, $('#pvPanel').style.display]; });
    assert.deepEqual(r, [['map', 'none', 'BG0'], '']);
  });
  test('name field is sanitized', async () => {
    const r = await run(() => { const n = $('#name'); n.value = 'My Sprite!'; n.dispatchEvent(new Event('change')); const a = [S.name, n.value]; n.value = '!!!'; n.dispatchEvent(new Event('change')); return [a, S.name]; });
    assert.deepEqual(r, [['my_sprite', 'my_sprite'], 'untitled']);
  });
  test('Help opens and closes', async () => {
    assert.deepEqual(await run(() => { $('#bHelp').click(); const a = $('#help').open; $('#help .actions button').click(); return [a, $('#help').open]; }), [true, false]);
  });
  test('toast shows a message and hides it later', async () => {
    const r = await run(async () => { toast('hello', 30); const a = [$('#toast').textContent, $('#toast').classList.contains('show')]; await T.wait(120); return [a, $('#toast').classList.contains('show')]; });
    assert.deepEqual(r, [['hello', true], false]);
  });
});

/* ================================================================================================ */
describe('new image', () => {
  test('newDoc makes a transparent sprite sheet or a filled map sheet', async () => {
    const r = await run(() => {
      newDoc(24, 16, 'sprite', false); const a = [S.W, S.H, T.count(-1), S.prim];
      newDoc(16, 16, 'map', false); return [a, [S.mode, T.count(S.pals[0][0]), S.prim, S.sec]];
    });
    assert.deepEqual(r, [[24, 16, 384, { p: 0, i: 1 }], ['map', 256, { p: 0, i: 3 }, { p: 0, i: 0 }]]);
  });
  test('newDoc can keep the palettes and is undoable', async () => {
    const r = await run(() => { S.pals[2][2] = 77; newDoc(16, 16, 'sprite', true); const a = S.pals[2][2]; newDoc(16, 16, 'sprite', false); const b = S.pals[2][2]; undo(); undo(); return [a, b === 77, S.W, S.H]; });
    assert.deepEqual(r, [77, false, 128, 64]);
  });
  test('New dialog: presets, rounding, mode and validation', async () => {
    const r = await run(() => {
      $('#bNew').click(); const pre = [$('#dNew').open, $('#nW').value, $('#nH').value, $('#nMode').value];
      document.querySelector('[data-size="160,144"]').click(); const preset = [$('#nW').value, $('#nH').value];
      $('#nW').value = 1700; $('#nOk').click(); const bad = [T.last(), $('#dNew').open];
      $('#nW').value = 30; $('#nMode').value = 'map'; $('#nOk').click();
      return [pre, preset, bad, [S.W, S.H, S.mode, $('#dNew').open, $('#mode').value]];
    });
    assert.deepEqual(r, [[true, '128', '64', 'sprite'], ['160', '144'], ['Width and height: 8 to 1600, multiples of 8', true], [32, 144, 'map', false, 'map']]);
  });
});

/* ================================================================================================ */
describe('import', () => {
  test('decodeBlob: alpha < 128 and magenta are transparent, colors snap to RGB555', async () => {
    const r = await run(async () => {
      const px = [[255, 0, 0, 255], [0, 255, 0, 0], [255, 0, 255, 255], [0, 0, 255, 100], [130, 130, 130, 255]];
      const img = await decodeBlob(await T.png(5, 1, x => px[x]));
      return [img.w, img.h, [...img.d], p8(130, 130, 130)];
    });
    assert.deepEqual(r.slice(0, 3), [5, 1, [31, -1, -1, -1, r[3]]]);
  });
  test('detectScale finds the pixel-art scale, keeping at least 8 px', async () => {
    const r = await run(() => {
      const img = (w, h, f) => { const d = new Int32Array(w * h); for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) d[y * w + x] = f(x, y); return { w, h, d }; };
      return [detectScale(img(16, 16, (x, y) => (x >> 1) + (y >> 1) * 8)), detectScale(img(24, 24, (x, y) => ((x / 3) | 0) * 7 + ((y / 3) | 0))),
        detectScale(img(32, 32, () => 5)), detectScale(img(8, 8, () => 5)), detectScale(img(16, 16, (x, y) => x * 16 + y))];
    });
    assert.deepEqual(r, [2, 3, 4, 1, 1]);
  });
  test('downscale follows the Import scale option', async () => {
    const r = await run(() => {
      const d = new Int32Array(256); for (let i = 0; i < 256; i++) d[i] = i; const img = { w: 16, h: 16, d };
      const o = v => { $('#impScale').value = v; const r = downscale(img); return [r.s, r.img.w, r.img.d[1]]; };
      return [o('auto'), o('1'), o('2')];
    });
    assert.deepEqual(r, [[1, 16, 1], [1, 16, 1], [2, 8, 2]]);
  });
  test('opening a PNG with transparency makes a padded sprite sheet with extracted palettes', async () => {
    const r = await run(async () => {
      await openFile(await T.file('My Hero.png', 10, 5, x => x < 2 ? [0, 0, 0, 0] : [200, 40, 40, 255]));
      return [S.W, S.H, S.mode, S.name, $('#name').value, S.pals[0][1] === p8(200, 40, 40), T.px(12, 0), T.px(0, 6), S.prim, T.last()];
    });
    assert.deepEqual(r.slice(0, 9), [16, 8, 'sprite', 'my_hero', 'my_hero', true, -1, -1, { p: 0, i: 1 }]);
    assert.match(r[9], /sprite sheet \(has transparency\), padded to 16×8\. Palettes were extracted/);
  });
  test('opening an opaque PNG makes a map sheet; 2× art is scaled down', async () => {
    const r = await run(async () => {
      $('#impScale').value = 'auto';
      await openFile(await T.file('level.png', 32, 32, (x, y) => ((x >> 1) + (y >> 1)) & 1 ? [0, 0, 0, 255] : [255, 255, 255, 255]));
      return [S.W, S.H, S.mode, S.pals[0][0], S.pals[0][1], T.last()];
    });
    assert.deepEqual(r.slice(0, 5), [16, 16, 'map', 32767, 0]);
    assert.match(r[5], /map sheet \(no transparency\), scaled down 2×/);
  });
  test('opening is undoable', async () => {
    assert.deepEqual(await run(async () => { await openFile(await T.file('a.png', 8, 8, () => [1, 2, 3, 255])); undo(); return [S.W, S.H]; }), [128, 64]);
  });
  test('non-images and broken images are rejected', async () => {
    const r = await run(async () => {
      await openFile(new File(['x'], 'a.txt', { type: 'text/plain' })); const a = T.last();
      await openFile(new File(['garbage'], 'b.png', { type: 'image/png' })); return [a, T.last(), S.W];
    });
    assert.deepEqual(r, ['Not an image file', 'Could not read that image', 128]);
  });
  test('Open… button and the file input', async () => {
    await run(async () => {
      let clicked = false; $('#fileIn').click = () => { clicked = true; }; $('#bOpen').click(); if (!clicked) throw new Error('file chooser not opened');
      const dt = new DataTransfer(); dt.items.add(await T.file('x.png', 24, 8, () => [9, 9, 9, 0]));
      $('#fileIn').files = dt.files; $('#fileIn').dispatchEvent(new Event('change'));
    });
    await page.waitFor('S.W === 24');
    assert.equal(await run(() => $('#fileIn').value), '');
  });
  test('dropping a PNG on the canvas opens it', async () => {
    const r = await run(async () => {
      const dt = new DataTransfer(); dt.items.add(await T.file('drop.png', 40, 8, () => [9, 9, 9, 0]));
      wrap.dispatchEvent(new DragEvent('dragover', { bubbles: true, cancelable: true, dataTransfer: dt })); const over = wrap.classList.contains('drop');
      wrap.dispatchEvent(new DragEvent('dragleave', { bubbles: true })); const left = wrap.classList.contains('drop');
      wrap.dispatchEvent(new DragEvent('drop', { bubbles: true, cancelable: true, dataTransfer: dt }));
      return [over, left];
    });
    assert.deepEqual(r, [true, false]);
    await page.waitFor('S.W === 40 && S.name === "drop"');
  });
  test('Ctrl+V pastes an image from another app as floating pixels', async () => {
    await run(async () => {
      T.doc(32, 32); const dt = new DataTransfer(); dt.items.add(await T.file('p.png', 16, 16, (x, y) => ((x >> 1) + (y >> 1)) & 1 ? [255, 0, 0, 255] : [0, 0, 255, 255]));
      document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: dt, bubbles: true, cancelable: true }));
    });
    await page.waitFor('S.flt !== null');
    const r = await run(() => [S.flt.w, S.flt.h, S.tool, T.last()]);
    assert.deepEqual(r.slice(0, 3), [8, 8, 'move']);
    assert.match(r[3], /Pasted 8×8 \(scaled down 2×\)/);
  });
  test('pasting our own copied image keeps it at 1× (no rescale)', async () => {
    await run(async () => {
      T.doc(32, 32); for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) putPx(x, y, ((x >> 1) + (y >> 1)) & 1 ? 31 : 31744);
      S.sel = { x: 0, y: 0, w: 16, h: 16 }; S.clip = regionData(); S.sel = null; T.toasts.length = 0;
      const dt = new DataTransfer(); dt.items.add(await T.file('p.png', 16, 16, (x, y) => ((x >> 1) + (y >> 1)) & 1 ? [255, 0, 0, 255] : [0, 0, 255, 255]));
      document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: dt, bubbles: true, cancelable: true }));
    });
    await page.waitFor('S.flt !== null');
    assert.deepEqual(await run(() => [S.flt.w, T.toasts.some(t => t.startsWith('Pasted'))]), [16, false]);
  });
  test('Ctrl+V with no image uses the internal clipboard or says there is none', async () => {
    const r = await run(() => {
      const paste = () => document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: new DataTransfer(), bubbles: true, cancelable: true }));
      paste(); const a = T.last(); S.clip = { w: 1, h: 1, d: Int32Array.of(7) }; paste(); return [a, S.flt && S.flt.d[0]];
    });
    assert.deepEqual(r, ['The clipboard has no image', 7]);
  });
  test('paste is ignored in text fields and while a dialog is open', async () => {
    const r = await run(() => {
      S.clip = { w: 1, h: 1, d: Int32Array.of(7) };
      $('#name').dispatchEvent(new ClipboardEvent('paste', { clipboardData: new DataTransfer(), bubbles: true, cancelable: true })); const a = S.flt;
      $('#help').showModal(); document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: new DataTransfer(), bubbles: true, cancelable: true })); return [a, S.flt];
    });
    assert.deepEqual(r, [null, null]);
  });
});

/* ================================================================================================ */
describe('export', () => {
  test('sanitize makes safe lowercase file names', async () => {
    assert.deepEqual(await run(() => [sanitize('My Hero!!'), sanitize('__a__b__'), sanitize('lvl-2.final'), sanitize('***')]), ['my_hero', 'a__b', 'lvl_2_final', '']);
  });
  test('dataToCanvas scales with nearest neighbour and keeps transparency', async () => {
    const r = await run(() => { const c = dataToCanvas({ w: 2, h: 1, d: Int32Array.of(31, -1) }, 2); return [c.width, c.height, T.rgba(c, 1, 1), T.rgba(c, 2, 0)[3]]; });
    assert.deepEqual(r, [4, 2, [255, 0, 0, 255], 0]);
  });
  test('Export PNG saves <name>.png with the exact pixels', async () => {
    const r = await run(async () => {
      T.doc(16, 8); putPx(3, 2, S.pals[0][2]); T.stubSave(); $('#name').value = 'Hero Sprite';
      $('#bExport').click(); await T.wait(300);
      const img = await decodeBlob(T.saved.blob);
      return [T.saved.opts.suggestedName, T.saved.opts.id, img.w, img.h, img.d.every((v, i) => v === S.px[i]), $('#name').value, T.last()];
    });
    assert.deepEqual(r, ['hero_sprite.png', 'gbc-spritesheets', 16, 8, true, 'hero_sprite', 'Saved hero_sprite.png']);
  });
  test('Export scale multiplies the size; map sheets use their own folder id', async () => {
    const r = await run(async () => { T.doc(16, 8, 'map'); T.stubSave(); $('#expScale').value = '3'; await exportPng(); const img = await decodeBlob(T.saved.blob); return [img.w, img.h, T.saved.opts.id]; });
    assert.deepEqual(r, [48, 24, 'gbc-mapsheets']);
  });
  test('Export asks before saving an image with problems', async () => {
    const r = await run(async () => {
      T.doc(8, 8); putPx(0, 0, S.pals[0][1]); putPx(1, 0, S.pals[1][1]); T.stubSave();
      let asked = ''; window.confirm = m => { asked = m; return false; };
      await exportPng(); const a = [asked, T.saved];
      window.confirm = () => true; await exportPng(); return [a, !!T.saved];
    });
    assert.deepEqual(r, [['1 tile(s) break the GBC palette rules (outlined in red). Export anyway?', null], true]);
  });
  test('Export drops floating pixels first', async () => {
    assert.deepEqual(await run(async () => { T.doc(8, 8); T.stubSave(); S.flt = { x: 1, y: 1, w: 1, h: 1, d: Int32Array.of(S.pals[0][1]) }; await exportPng(); return [S.flt, (await decodeBlob(T.saved.blob)).d[9] === S.pals[0][1]]; }), [null, true]);
  });
  test('without a save picker it downloads the file', async () => {
    const r = await run(async () => {
      window.showSaveFilePicker = undefined; const got = [];
      HTMLAnchorElement.prototype.click = function () { got.push(this.download); };
      await exportPng(); return [got, T.last()];
    });
    assert.deepEqual(r, [['player.png'], 'Downloaded player.png — move it into source_art/spritesheets/']);
  });
  test('cancelling the save picker does not download', async () => {
    const r = await run(async () => {
      window.showSaveFilePicker = async () => { throw new DOMException('cancel', 'AbortError'); }; const got = [];
      HTMLAnchorElement.prototype.click = function () { got.push(this.download); };
      await exportPng(); return got.length;
    });
    assert.equal(r, 0);
  });
  test('notes template for a sprite sheet lists frames, scale, palette and animations', async () => {
    const r = await run(() => {
      T.doc(64, 32); $('#name').value = 'hero'; const empty = notesTemplate();
      putPx(0, 0, S.pals[3][1]); putPx(16, 0, S.pals[3][1]); putPx(16, 16, S.pals[3][1]); analyze();
      return [empty, notesTemplate()];
    });
    assert.equal(r[0], 'frame: 16x16\nscale: 1\npal_slot: 0\nanimations (row: name, frames, frames per step, loop or once):\n  row 0: idle, 1\nnotes: ');
    assert.equal(r[1], 'frame: 16x16\nscale: 1\npal_slot: 3\nanimations (row: name, frames, frames per step, loop or once):\n  row 0: anim0, 2, 8, loop\n  row 1: anim1, 2, 8, loop\nnotes: ');
  });
  test('notes template for map sheets (160×144 is a title screen)', async () => {
    const r = await run(() => { T.doc(320, 144, 'map'); $('#name').value = 'Forest'; const a = notesTemplate(); T.doc(160, 144, 'map'); return [a, notesTemplate().split('\n')[0]]; });
    assert.equal(r[0], 'type: map               (map | tiles | title)\ntileset: forest\nscale: 1\nsolid: \noneway: \nhazard: \nover: \nspawn: player at tile (x, y)\ntrigger: ');
    assert.equal(r[1], 'type: title               (map | tiles | title)');
  });
  test('Notes dialog: regenerate and save a CRLF .txt', async () => {
    const r = await run(async () => {
      T.stubSave(); $('#name').value = 'hero'; $('#bNotes').click();
      const a = [$('#dNotes').open, $('#notesName').textContent, $('#notesText').value === notesTemplate()];
      $('#notesText').value = 'x'; $('#notesRegen').click(); const b = $('#notesText').value === notesTemplate();
      $('#notesText').value = 'a\nb'; $('#notesSave').click(); await T.wait(100);
      return [a, b, T.saved.opts.suggestedName, await T.saved.blob.text(), T.saved.blob.type];
    });
    assert.deepEqual(r, [[true, 'hero', true], true, 'hero.txt', 'a\r\nb\r\n', 'text/plain']);
  });
});

/* ================================================================================================ */
describe('autosave', () => {
  test('saveLocal stores name, mode, palettes, grid and a PNG', async () => {
    const r = await run(() => { S.name = 'abc'; saveLocal(); const st = JSON.parse(localStorage.getItem('gbc-pixel-editor')); return [st.v, st.name, st.mode, st.pals.length, st.grid.fw, st.png.startsWith('data:image/png')]; });
    assert.deepEqual(r, [1, 'abc', 'sprite', 8, 16, true]);
  });
  test('loadLocal restores the document', async () => {
    const r = await run(async () => {
      T.doc(16, 8, 'map'); putPx(1, 1, pack(3, 4, 5)); S.pals[2][1] = 99; S.name = 'lvl'; S.grid.fw = 32; saveLocal();
      T.doc(8, 8); S.name = 'x'; S.grid.fw = 8;
      const ok = await loadLocal(); return [ok, S.W, S.H, S.mode, T.px(1, 1), S.pals[2][1], S.name, S.grid.fw, S.prim];
    });
    assert.deepEqual(r, [true, 16, 8, 'map', 5251, 99, 'lvl', 32, { p: 0, i: 3 }]);
  });
  test('edits are autosaved and restored after a reload', async () => {
    await run(() => { T.doc(16, 16); beginEdit(); setPx(2, 3, 31); endEdit(); });
    await page.waitFor('localStorage.getItem("gbc-pixel-editor") !== null', 3000);
    await run(() => sessionStorage.setItem('keepStorage', '1'));
    await load();
    assert.deepEqual(await run(() => [S.W, S.H, T.px(2, 3)]), [16, 16, 31]);
  });
  test('bad saved data is ignored and palettes are clamped', async () => {
    const r = await run(async () => {
      localStorage.setItem('gbc-pixel-editor', '{bad'); const a = await loadLocal();
      localStorage.setItem('gbc-pixel-editor', '{"v":1}'); const b = await loadLocal();
      saveLocal(); const st = JSON.parse(localStorage.getItem('gbc-pixel-editor'));
      st.pals[0][1] = 99999; st.pals[0][2] = -5; localStorage.setItem('gbc-pixel-editor', JSON.stringify(st));
      await loadLocal(); return [a, b, S.pals[0][1], S.pals[0][2]];
    });
    assert.deepEqual(r, [false, false, 32767, 0]);
  });
});

/* ================================================================================================ */
describe('animation preview', () => {
  const setup = () => run(() => {
    T.doc(32, 32);
    T.rect(0, 0, 8, 16, 31);            /* row 0 frame 0: red on the left half */
    T.rect(16, 0, 16, 16, 31744);       /* row 0 frame 1: blue */
    T.rect(0, 16, 16, 16, 992);         /* row 1 frame 0: green */
    analyze(); $('#pvPlay').checked = false; pv.frame = 0; pv.tick = 0;
  });
  const sample = () => run(async () => { await T.wait(60); const c = $('#pvCanvas'); return [T.rgba(c, 32, 64).join(), T.rgba(c, 96, 64).join()]; });

  test('draws the current frame scaled up', async () => {
    await setup();
    assert.deepEqual(await sample(), ['255,0,0,255', '0,0,0,0']);
    await run(() => { pv.frame = 1; });
    assert.deepEqual(await sample(), ['0,0,255,255', '0,0,255,255']);
  });
  test('Face left mirrors the frame', async () => {
    await setup();
    await run(() => { $('#pvFlip').checked = true; });
    assert.deepEqual(await sample(), ['0,0,0,0', '255,0,0,255']);
  });
  test('Row chooses the animation row', async () => {
    await setup();
    await run(() => { $('#pvRow').value = 1; });
    assert.deepEqual(await sample(), ['0,255,0,255', '0,255,0,255']);
  });
  test('Play steps through the used frames at Speed', async () => {
    await setup();
    const seen = await run(async () => {
      $('#pvSpeed').value = 1; $('#pvPlay').checked = true; const s = new Set();
      for (let i = 0; i < 20; i++) { await T.wait(20); s.add(pv.frame); }
      return [...s].sort();
    });
    assert.deepEqual(seen, [0, 1]);
  });
  test('Frames limits the loop', async () => {
    await setup();
    const seen = await run(async () => {
      $('#pvSpeed').value = 1; $('#pvFrames').value = 1; $('#pvPlay').checked = true; const s = new Set();
      for (let i = 0; i < 10; i++) { await T.wait(20); s.add(pv.frame); }
      return [...s];
    });
    assert.deepEqual(seen, [0]);
  });
});

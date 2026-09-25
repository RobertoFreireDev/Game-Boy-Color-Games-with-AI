# PNG helper script

Read when: you are about to measure a PNG (section 14.1). Copy the script to the scratchpad/temp folder, never into the project.

### 14.5 Helper script `png2gb.js` (copy to a temp folder, never commit)

```js
// png2gb.js <file.png> [colors|grid|tiles] [--scale N] [--rect x,y,w,h]
const fs = require('fs'), zlib = require('zlib');
const a = process.argv.slice(2), file = a[0], mode = a[1] && !a[1].startsWith('--') ? a[1] : 'colors';
const opt = k => { const i = a.indexOf(k); return i < 0 ? null : a[i + 1]; };
const scale = +(opt('--scale') || 1);

// ---- decode (non-interlaced PNG, any color type, bit depth 1-8) ----
const b = fs.readFileSync(file);
let p = 8, W, H, depth, ctype, plte = [], trns = null, idat = [];
while (p < b.length) {
  const len = b.readUInt32BE(p), type = b.toString('ascii', p + 4, p + 8), d = b.subarray(p + 8, p + 8 + len);
  if (type === 'IHDR') { W = d.readUInt32BE(0); H = d.readUInt32BE(4); depth = d[8]; ctype = d[9];
    if (d[12]) throw 'interlaced PNG: re-save without interlacing'; if (depth > 8) throw '16-bit PNG: save as 8-bit'; }
  if (type === 'PLTE') for (let i = 0; i < len; i += 3) plte.push([d[i], d[i + 1], d[i + 2], 255]);
  if (type === 'tRNS') trns = d;
  if (type === 'IDAT') idat.push(d);
  p += 12 + len;
}
if (trns && ctype === 3) for (let i = 0; i < trns.length; i++) plte[i][3] = trns[i];
const ch = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[ctype], bpp = Math.max(1, (ch * depth) >> 3), stride = (W * ch * depth + 7) >> 3;
const raw = zlib.inflateSync(Buffer.concat(idat)), img = Buffer.alloc(stride * H);
for (let y = 0; y < H; y++) {
  const f = raw[y * (stride + 1)], src = raw.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1));
  for (let x = 0; x < stride; x++) {
    const L = x >= bpp ? img[y * stride + x - bpp] : 0, U = y ? img[(y - 1) * stride + x] : 0,
          UL = y && x >= bpp ? img[(y - 1) * stride + x - bpp] : 0;
    let v = src[x];
    if (f === 1) v += L; else if (f === 2) v += U; else if (f === 3) v += (L + U) >> 1;
    else if (f === 4) { const q = L + U - UL, pa = Math.abs(q - L), pb = Math.abs(q - U), pc = Math.abs(q - UL);
      v += pa <= pb && pa <= pc ? L : pb <= pc ? U : UL; }
    img[y * stride + x] = v & 255;
  }
}
const sample = (x, y, c) => { const bit = (x * ch + c) * depth, byte = img[y * stride + (bit >> 3)];
  return depth === 8 ? byte : (byte >> (8 - depth - (bit & 7))) & ((1 << depth) - 1); };
const up = v => depth === 8 ? v : Math.round(v * 255 / ((1 << depth) - 1));
function rgba(x, y) {
  if (ctype === 3) return plte[sample(x, y, 0)];
  if (ctype === 0 || ctype === 4) { const g = up(sample(x, y, 0)); return [g, g, g, ctype === 4 ? sample(x, y, 1) : 255]; }
  return [sample(x, y, 0), sample(x, y, 1), sample(x, y, 2), ctype === 6 ? sample(x, y, 3) : 255];
}

// ---- pixels as color keys ('-' = transparent: alpha < 128 or magenta FF00FF) ----
const hex = v => v.toString(16).padStart(2, '0').toUpperCase();
const r = (opt('--rect') || `0,0,${W / scale | 0},${H / scale | 0}`).split(',').map(Number);
const w = r[2], h = r[3], px = [];
for (let y = 0; y < h; y++) { px.push([]); for (let x = 0; x < w; x++) {
  const c = rgba((r[0] + x) * scale, (r[1] + y) * scale);
  px[y].push(c[3] < 128 || (c[0] === 255 && c[1] === 0 && c[2] === 255) ? '-' : hex(c[0]) + hex(c[1]) + hex(c[2]));
} }
const count = {}; px.flat().forEach(k => count[k] = (count[k] || 0) + 1);
const keys = Object.keys(count).sort((p, q) => count[q] - count[p]);
const SYM = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
const sym = {}; let n = 0; keys.forEach(k => sym[k] = k === '-' ? '.' : SYM[n++] || '?');
const rgb8 = k => k === '-' ? 'transparent' : `RGB8(${parseInt(k.slice(0, 2), 16)},${parseInt(k.slice(2, 4), 16)},${parseInt(k.slice(4), 16)})`;
console.log(`${file}: ${W}x${H} px, scale ${scale} -> ${w}x${h} (${w / 8}x${h / 8} tiles), ${keys.length} colors`);
keys.forEach(k => console.log(`  ${sym[k]}  #${k === '-' ? '------' : k}  ${rgb8(k)}  x${count[k]}`));

if (mode === 'grid') px.forEach(row => console.log(row.map(k => sym[k]).join('')));

if (mode === 'tiles') {   // unique 8x8 tiles (flips count as the same tile) + tile map
  const tiles = [], ids = {}, map = [], sets = {};
  for (let ty = 0; ty < h >> 3; ty++) { map.push([]); for (let tx = 0; tx < w >> 3; tx++) {
    const t = []; for (let y = 0; y < 8; y++) t.push(px[ty * 8 + y].slice(tx * 8, tx * 8 + 8).map(k => sym[k]).join(''));
    const fx = t.map(s => [...s].reverse().join('')), fy = [...t].reverse(), fxy = [...fx].reverse();
    let id = ids[t.join('/')], f = '';
    if (id === undefined && ids[fx.join('/')] !== undefined) { id = ids[fx.join('/')]; f = 'X'; }
    if (id === undefined && ids[fy.join('/')] !== undefined) { id = ids[fy.join('/')]; f = 'Y'; }
    if (id === undefined && ids[fxy.join('/')] !== undefined) { id = ids[fxy.join('/')]; f = 'XY'; }
    if (id === undefined) { id = tiles.length; ids[t.join('/')] = id; tiles.push(t); }
    map[ty].push(id + f);
  } }
  console.log(`\n${tiles.length} unique tiles (limit 128 per scene)`);
  tiles.forEach((t, i) => { const cs = [...new Set(t.join(''))].sort().join(''); (sets[cs] = sets[cs] || []).push(i);
    console.log(`tile ${i}  colors [${cs}]${cs.replace('.', '').length > 4 ? '  !! more than 4 colors' : ''}`); t.forEach(s => console.log('  ' + s)); });
  console.log(`\n${Object.keys(sets).length} distinct color sets (merge them into <= 7 BG palettes of 4 colors):`);
  Object.keys(sets).forEach(s => console.log(`  [${s}] tiles ${sets[s].join(',')}`));
  console.log('\ntile map (id + X/Y flip):'); map.forEach(row => console.log(row.map(v => String(v).padStart(4)).join('')));
}
```

/*
 * Minimal headless Game Boy Color for the engine unit tests (no npm packages).
 * Runs a GBDK ROM: SM83 CPU, MBC5, CGB VRAM/WRAM banks, CGB palettes, OAM DMA,
 * general-purpose HDMA, LY/STAT timing with VBlank/LCD interrupts, DIV/TIMA,
 * double speed (KEY1 + STOP) and the serial port.
 * It draws no pixels and plays no sound: the tests only look at memory.
 *
 * Test hooks:
 *  - Every byte sent on the serial port (SB, then SC = 0x81) goes to onSerial.
 *  - Sound registers FF10-FF3F read back exactly what was written (real hardware
 *    masks some bits), so tests can check what the audio driver wrote.
 *  - Joypad: no button is ever pressed (tests replace joypad() instead).
 */

const FZ = 0x80, FN = 0x40, FH = 0x20, FC = 0x10;

export class GBC {
  constructor(rom) {
    this.rom = rom;
    this.vram = new Uint8Array(0x4000);
    this.wram = new Uint8Array(0x8000);
    this.sram = new Uint8Array(0x20000);
    this.oam = new Uint8Array(0xA0);
    this.hram = new Uint8Array(0x80);
    this.io = new Uint8Array(0x80);
    this.bgPal = new Uint8Array(64);
    this.objPal = new Uint8Array(64);
    this.vbk = 0; this.svbk = 1;
    this.romBank = 1; this.ramBank = 0; this.ramOn = false;
    this.ie = 0;
    this.ime = false; this.eiPending = false; this.halted = false; this.stopped = false;
    this.double = false;
    this.ly = 0; this.dot = 0; this.dotAcc = 0;
    this.div = 0; this.timaAcc = 0;
    this.cycles = 0;              /* total CPU cycles */
    this.frames = 0;              /* VBlanks since power on */
    this.onSerial = null;
    /* CGB state after the boot ROM */
    this.a = 0x11; this.f = 0x80; this.b = 0x00; this.c = 0x00;
    this.d = 0xFF; this.e = 0x56; this.h = 0x00; this.l = 0x0D;
    this.sp = 0xFFFE; this.pc = 0x0100;
    this.io[0x40] = 0x91; this.io[0x41] = 0x81; this.io[0x47] = 0xFC;
  }

  /* ---------------- memory ---------------- */

  read(a) {
    if (a < 0x4000) return this.rom[a];
    if (a < 0x8000) return this.rom[(this.romBank * 0x4000 + a - 0x4000) % this.rom.length];
    if (a < 0xA000) return this.vram[this.vbk * 0x2000 + a - 0x8000];
    if (a < 0xC000) return this.ramOn ? this.sram[this.ramBank * 0x2000 + a - 0xA000] : 0xFF;
    if (a < 0xD000) return this.wram[a - 0xC000];
    if (a < 0xE000) return this.wram[this.svbk * 0x1000 + a - 0xD000];
    if (a < 0xFE00) return this.read(a - 0x2000);
    if (a < 0xFEA0) return this.oam[a - 0xFE00];
    if (a < 0xFF00) return 0xFF;
    if (a < 0xFF80) return this.readIO(a & 0x7F);
    if (a < 0xFFFF) return this.hram[a - 0xFF80];
    return this.ie;
  }

  write(a, v) {
    v &= 0xFF;
    if (a < 0x2000) { this.ramOn = (v & 0x0F) === 0x0A; return; }
    if (a < 0x3000) { this.romBank = (this.romBank & 0x100) | v; return; }
    if (a < 0x4000) { this.romBank = (this.romBank & 0xFF) | ((v & 1) << 8); return; }
    if (a < 0x6000) { this.ramBank = v & 0x0F; return; }
    if (a < 0x8000) return;
    if (a < 0xA000) { this.vram[this.vbk * 0x2000 + a - 0x8000] = v; return; }
    if (a < 0xC000) { if (this.ramOn) this.sram[this.ramBank * 0x2000 + a - 0xA000] = v; return; }
    if (a < 0xD000) { this.wram[a - 0xC000] = v; return; }
    if (a < 0xE000) { this.wram[this.svbk * 0x1000 + a - 0xD000] = v; return; }
    if (a < 0xFE00) { this.write(a - 0x2000, v); return; }
    if (a < 0xFEA0) { this.oam[a - 0xFE00] = v; return; }
    if (a < 0xFF00) return;
    if (a < 0xFF80) { this.writeIO(a & 0x7F, v); return; }
    if (a < 0xFFFF) { this.hram[a - 0xFF80] = v; return; }
    this.ie = v;
  }

  mode() {
    if (!(this.io[0x40] & 0x80)) return 0;
    if (this.ly >= 144) return 1;
    if (this.dot < 80) return 2;
    if (this.dot < 252) return 3;
    return 0;
  }

  readIO(r) {
    switch (r) {
      case 0x00: return 0xC0 | (this.io[0] & 0x30) | 0x0F;
      case 0x04: return (this.div >> 8) & 0xFF;
      case 0x0F: return 0xE0 | this.io[0x0F];
      case 0x41: return 0x80 | (this.io[0x41] & 0x78) | (this.ly === this.io[0x45] ? 4 : 0) | this.mode();
      case 0x44: return this.ly;
      case 0x4D: return (this.double ? 0x80 : 0) | 0x7E | (this.io[0x4D] & 1);
      case 0x4F: return 0xFE | this.vbk;
      case 0x69: return this.bgPal[this.io[0x68] & 0x3F];
      case 0x6B: return this.objPal[this.io[0x6A] & 0x3F];
      case 0x70: return 0xF8 | this.svbk;
      default: return this.io[r];
    }
  }

  writeIO(r, v) {
    switch (r) {
      case 0x02:
        this.io[2] = v;
        if (v & 0x80) {
          if (this.onSerial) this.onSerial(this.io[1]);
          this.io[2] = v & 0x7F;
          this.io[0x0F] |= 0x08;
        }
        return;
      case 0x04: this.div = 0; return;
      case 0x0F: this.io[0x0F] = v & 0x1F; return;
      case 0x40:
        if (!(v & 0x80)) { this.ly = 0; this.dot = 0; }
        this.io[0x40] = v; return;
      case 0x41: this.io[0x41] = v & 0x78; return;
      case 0x44: return;
      case 0x46: {
        const src = v << 8;
        for (let i = 0; i < 0xA0; i++) this.oam[i] = this.read(src + i);
        this.io[0x46] = v; return;
      }
      case 0x4D: this.io[0x4D] = v & 1; return;
      case 0x4F: this.vbk = v & 1; return;
      case 0x55: {
        const src = ((this.io[0x51] << 8) | this.io[0x52]) & 0xFFF0;
        const dst = 0x8000 | (((this.io[0x53] << 8) | this.io[0x54]) & 0x1FF0);
        const len = ((v & 0x7F) + 1) * 16;
        for (let i = 0; i < len; i++) this.write(((dst + i) & 0x1FFF) | 0x8000, this.read((src + i) & 0xFFFF));
        this.io[0x55] = 0xFF; return;
      }
      case 0x69: this.palWrite(this.bgPal, 0x68, v); return;
      case 0x6B: this.palWrite(this.objPal, 0x6A, v); return;
      case 0x70: this.svbk = (v & 7) || 1; return;
      default: this.io[r] = v;
    }
  }

  palWrite(pal, spec, v) {
    const s = this.io[spec];
    pal[s & 0x3F] = v;
    if (s & 0x80) this.io[spec] = 0x80 | ((s + 1) & 0x3F);
  }

  /* ---------------- timing ---------------- */

  tick(cyc) {
    this.cycles += cyc;
    this.div = (this.div + cyc) & 0xFFFF;
    const tac = this.io[0x07];
    if (tac & 4) {
      const period = [1024, 16, 64, 256][tac & 3];
      this.timaAcc += cyc;
      while (this.timaAcc >= period) {
        this.timaAcc -= period;
        if (++this.io[0x05] > 0xFF || this.io[0x05] === 0) { this.io[0x05] = this.io[0x06]; this.io[0x0F] |= 0x04; }
      }
    }
    if (!(this.io[0x40] & 0x80)) return;
    this.dotAcc += this.double ? cyc : cyc * 2;
    this.dot += this.dotAcc >> 1;
    this.dotAcc &= 1;
    while (this.dot >= 456) {
      this.dot -= 456;
      if (++this.ly > 153) this.ly = 0;
      if (this.ly === 144) {
        this.frames++;
        this.io[0x0F] |= 0x01;
        if (this.io[0x41] & 0x10) this.io[0x0F] |= 0x02;
      }
      if (this.ly === this.io[0x45] && (this.io[0x41] & 0x40)) this.io[0x0F] |= 0x02;
    }
  }

  /* ---------------- CPU helpers ---------------- */

  fetch() { const v = this.read(this.pc); this.pc = (this.pc + 1) & 0xFFFF; return v; }
  fetch16() { const lo = this.fetch(); return lo | (this.fetch() << 8); }
  push(v) { this.sp = (this.sp - 1) & 0xFFFF; this.write(this.sp, v >> 8); this.sp = (this.sp - 1) & 0xFFFF; this.write(this.sp, v & 0xFF); }
  pop() { const lo = this.read(this.sp); this.sp = (this.sp + 1) & 0xFFFF; const hi = this.read(this.sp); this.sp = (this.sp + 1) & 0xFFFF; return lo | (hi << 8); }

  get hl() { return (this.h << 8) | this.l; }
  set hl(v) { this.h = (v >> 8) & 0xFF; this.l = v & 0xFF; }

  getR(i) {
    switch (i) {
      case 0: return this.b; case 1: return this.c; case 2: return this.d; case 3: return this.e;
      case 4: return this.h; case 5: return this.l; case 6: return this.read(this.hl); default: return this.a;
    }
  }
  setR(i, v) {
    switch (i) {
      case 0: this.b = v; break; case 1: this.c = v; break; case 2: this.d = v; break; case 3: this.e = v; break;
      case 4: this.h = v; break; case 5: this.l = v; break; case 6: this.write(this.hl, v); break; default: this.a = v;
    }
  }
  getRR(i) {   /* BC DE HL SP */
    switch (i) {
      case 0: return (this.b << 8) | this.c; case 1: return (this.d << 8) | this.e;
      case 2: return this.hl; default: return this.sp;
    }
  }
  setRR(i, v) {
    v &= 0xFFFF;
    switch (i) {
      case 0: this.b = v >> 8; this.c = v & 0xFF; break; case 1: this.d = v >> 8; this.e = v & 0xFF; break;
      case 2: this.hl = v; break; default: this.sp = v;
    }
  }
  cond(i) {
    switch (i) {
      case 0: return !(this.f & FZ); case 1: return !!(this.f & FZ);
      case 2: return !(this.f & FC); default: return !!(this.f & FC);
    }
  }

  alu(op, v) {
    const a = this.a;
    let r;
    switch (op) {
      case 0: r = a + v; this.f = ((r & 0xFF) ? 0 : FZ) | (((a & 0xF) + (v & 0xF)) > 0xF ? FH : 0) | (r > 0xFF ? FC : 0); this.a = r & 0xFF; break;
      case 1: { const cy = (this.f & FC) ? 1 : 0; r = a + v + cy; this.f = ((r & 0xFF) ? 0 : FZ) | (((a & 0xF) + (v & 0xF) + cy) > 0xF ? FH : 0) | (r > 0xFF ? FC : 0); this.a = r & 0xFF; break; }
      case 2: r = a - v; this.f = FN | ((r & 0xFF) ? 0 : FZ) | ((a & 0xF) < (v & 0xF) ? FH : 0) | (r < 0 ? FC : 0); this.a = r & 0xFF; break;
      case 3: { const cy = (this.f & FC) ? 1 : 0; r = a - v - cy; this.f = FN | ((r & 0xFF) ? 0 : FZ) | ((a & 0xF) - (v & 0xF) - cy < 0 ? FH : 0) | (r < 0 ? FC : 0); this.a = r & 0xFF; break; }
      case 4: this.a = a & v; this.f = (this.a ? 0 : FZ) | FH; break;
      case 5: this.a = a ^ v; this.f = this.a ? 0 : FZ; break;
      case 6: this.a = a | v; this.f = this.a ? 0 : FZ; break;
      default: r = a - v; this.f = FN | ((r & 0xFF) ? 0 : FZ) | ((a & 0xF) < (v & 0xF) ? FH : 0) | (r < 0 ? FC : 0); break;
    }
  }

  inc8(v) { const r = (v + 1) & 0xFF; this.f = (this.f & FC) | (r ? 0 : FZ) | ((v & 0xF) === 0xF ? FH : 0); return r; }
  dec8(v) { const r = (v - 1) & 0xFF; this.f = (this.f & FC) | FN | (r ? 0 : FZ) | ((v & 0xF) === 0 ? FH : 0); return r; }

  addSP(e) {   /* SP + signed e8, flags from the low byte */
    const sp = this.sp, s = e & 0x80 ? e - 256 : e;
    this.f = (((sp & 0xF) + (e & 0xF)) > 0xF ? FH : 0) | (((sp & 0xFF) + e) > 0xFF ? FC : 0);
    return (sp + s) & 0xFFFF;
  }

  cb() {
    const op = this.fetch(), r = op & 7, n = (op >> 3) & 7;
    let v = this.getR(r);
    const x = op >> 6;
    if (x === 1) {   /* BIT */
      this.f = (this.f & FC) | FH | ((v & (1 << n)) ? 0 : FZ);
      return r === 6 ? 12 : 8;
    }
    if (x === 2) { this.setR(r, v & ~(1 << n)); return r === 6 ? 16 : 8; }
    if (x === 3) { this.setR(r, v | (1 << n)); return r === 6 ? 16 : 8; }
    let c;
    switch (n) {
      case 0: c = v >> 7; v = ((v << 1) | c) & 0xFF; break;                          /* RLC */
      case 1: c = v & 1; v = (v >> 1) | (c << 7); break;                             /* RRC */
      case 2: c = v >> 7; v = ((v << 1) | ((this.f & FC) ? 1 : 0)) & 0xFF; break;    /* RL */
      case 3: c = v & 1; v = (v >> 1) | ((this.f & FC) ? 0x80 : 0); break;           /* RR */
      case 4: c = v >> 7; v = (v << 1) & 0xFF; break;                                /* SLA */
      case 5: c = v & 1; v = (v >> 1) | (v & 0x80); break;                           /* SRA */
      case 6: c = 0; v = ((v << 4) | (v >> 4)) & 0xFF; break;                        /* SWAP */
      default: c = v & 1; v = v >> 1; break;                                         /* SRL */
    }
    this.f = (v ? 0 : FZ) | (c ? FC : 0);
    this.setR(r, v);
    return r === 6 ? 16 : 8;
  }

  /* ---------------- CPU ---------------- */

  step() {
    const pending = this.ie & this.io[0x0F] & 0x1F;
    if (this.halted) {
      if (!pending) { this.tick(4); return; }
      this.halted = false;
    }
    if (this.ime && pending) {
      const bit = 31 - Math.clz32(pending & -pending);
      this.io[0x0F] &= ~(1 << bit);
      this.ime = false;
      this.push(this.pc);
      this.pc = 0x40 + bit * 8;
      this.tick(20);
      return;
    }
    const enableAfter = this.eiPending;
    this.eiPending = false;
    this.tick(this.exec(this.fetch()));
    if (enableAfter) this.ime = true;
  }

  exec(op) {
    if (op >= 0x40 && op < 0x80) {
      if (op === 0x76) {                                   /* HALT */
        if (this.ime || !(this.ie & this.io[0x0F] & 0x1F)) this.halted = true;
        return 4;
      }
      const s = op & 7, d = (op >> 3) & 7;
      this.setR(d, this.getR(s));
      return (s === 6 || d === 6) ? 8 : 4;
    }
    if (op >= 0x80 && op < 0xC0) {
      const s = op & 7;
      this.alu((op >> 3) & 7, this.getR(s));
      return s === 6 ? 8 : 4;
    }
    if (op < 0x40) {
      const r = (op >> 3) & 7, rr = (op >> 4) & 3;
      switch (op & 0x0F) {
        case 0x01: this.setRR(rr, this.fetch16()); return 12;
        case 0x03: this.setRR(rr, this.getRR(rr) + 1); return 8;
        case 0x09: {
          const hl = this.hl, v = this.getRR(rr), s = hl + v;
          this.f = (this.f & FZ) | (((hl & 0xFFF) + (v & 0xFFF)) > 0xFFF ? FH : 0) | (s > 0xFFFF ? FC : 0);
          this.hl = s & 0xFFFF; return 8;
        }
        case 0x0B: this.setRR(rr, this.getRR(rr) - 1); return 8;
      }
      switch (op & 0x07) {
        case 0x04: this.setR(r, this.inc8(this.getR(r))); return r === 6 ? 12 : 4;
        case 0x05: this.setR(r, this.dec8(this.getR(r))); return r === 6 ? 12 : 4;
        case 0x06: this.setR(r, this.fetch()); return r === 6 ? 12 : 8;
      }
      switch (op) {
        case 0x00: return 4;
        case 0x02: this.write(this.getRR(0), this.a); return 8;
        case 0x12: this.write(this.getRR(1), this.a); return 8;
        case 0x22: { const hl = this.hl; this.write(hl, this.a); this.hl = (hl + 1) & 0xFFFF; return 8; }
        case 0x32: { const hl = this.hl; this.write(hl, this.a); this.hl = (hl - 1) & 0xFFFF; return 8; }
        case 0x0A: this.a = this.read(this.getRR(0)); return 8;
        case 0x1A: this.a = this.read(this.getRR(1)); return 8;
        case 0x2A: { const hl = this.hl; this.a = this.read(hl); this.hl = (hl + 1) & 0xFFFF; return 8; }
        case 0x3A: { const hl = this.hl; this.a = this.read(hl); this.hl = (hl - 1) & 0xFFFF; return 8; }
        case 0x07: { const c = this.a >> 7; this.a = ((this.a << 1) | c) & 0xFF; this.f = c ? FC : 0; return 4; }
        case 0x0F: { const c = this.a & 1; this.a = (this.a >> 1) | (c << 7); this.f = c ? FC : 0; return 4; }
        case 0x17: { const c = this.a >> 7; this.a = ((this.a << 1) | ((this.f & FC) ? 1 : 0)) & 0xFF; this.f = c ? FC : 0; return 4; }
        case 0x1F: { const c = this.a & 1; this.a = (this.a >> 1) | ((this.f & FC) ? 0x80 : 0); this.f = c ? FC : 0; return 4; }
        case 0x08: { const a = this.fetch16(); this.write(a, this.sp & 0xFF); this.write((a + 1) & 0xFFFF, this.sp >> 8); return 20; }
        case 0x10:                                             /* STOP: CGB speed switch */
          this.fetch();
          if (this.io[0x4D] & 1) { this.double = !this.double; this.io[0x4D] = 0; }
          return 4;
        case 0x18: { const e = this.fetch(); this.pc = (this.pc + (e & 0x80 ? e - 256 : e)) & 0xFFFF; return 12; }
        case 0x20: case 0x28: case 0x30: case 0x38: {
          const e = this.fetch();
          if (this.cond((op >> 3) & 3)) { this.pc = (this.pc + (e & 0x80 ? e - 256 : e)) & 0xFFFF; return 12; }
          return 8;
        }
        case 0x27: {                                           /* DAA */
          let a = this.a, c = this.f & FC;
          if (!(this.f & FN)) {
            if (c || a > 0x99) { a += 0x60; c = FC; }
            if ((this.f & FH) || (a & 0x0F) > 9) a += 0x06;
          } else {
            if (c) a -= 0x60;
            if (this.f & FH) a -= 0x06;
          }
          a &= 0xFF;
          this.f = (this.f & FN) | (a ? 0 : FZ) | c;
          this.a = a; return 4;
        }
        case 0x2F: this.a ^= 0xFF; this.f |= FN | FH; return 4;
        case 0x37: this.f = (this.f & FZ) | FC; return 4;
        case 0x3F: this.f = (this.f & FZ) | ((this.f & FC) ? 0 : FC); return 4;
      }
    }
    /* 0xC0 - 0xFF */
    if ((op & 0xE7) === 0xC0) { if (this.cond((op >> 3) & 3)) { this.pc = this.pop(); return 20; } return 8; }
    if ((op & 0xE7) === 0xC2) { const a = this.fetch16(); if (this.cond((op >> 3) & 3)) { this.pc = a; return 16; } return 12; }
    if ((op & 0xE7) === 0xC4) { const a = this.fetch16(); if (this.cond((op >> 3) & 3)) { this.push(this.pc); this.pc = a; return 24; } return 12; }
    if ((op & 0xC7) === 0xC6) { this.alu((op >> 3) & 7, this.fetch()); return 8; }
    if ((op & 0xC7) === 0xC7) { this.push(this.pc); this.pc = op & 0x38; return 16; }
    if ((op & 0xCF) === 0xC1) {
      const v = this.pop(), q = (op >> 4) & 3;
      if (q === 3) { this.a = v >> 8; this.f = v & 0xF0; } else this.setRR(q, v);
      return 12;
    }
    if ((op & 0xCF) === 0xC5) {
      const q = (op >> 4) & 3;
      this.push(q === 3 ? (this.a << 8) | this.f : this.getRR(q));
      return 16;
    }
    switch (op) {
      case 0xC3: this.pc = this.fetch16(); return 16;
      case 0xC9: this.pc = this.pop(); return 16;
      case 0xD9: this.pc = this.pop(); this.ime = true; return 16;
      case 0xCB: return this.cb();
      case 0xCD: { const a = this.fetch16(); this.push(this.pc); this.pc = a; return 24; }
      case 0xE0: this.write(0xFF00 | this.fetch(), this.a); return 12;
      case 0xF0: this.a = this.read(0xFF00 | this.fetch()); return 12;
      case 0xE2: this.write(0xFF00 | this.c, this.a); return 8;
      case 0xF2: this.a = this.read(0xFF00 | this.c); return 8;
      case 0xEA: this.write(this.fetch16(), this.a); return 16;
      case 0xFA: this.a = this.read(this.fetch16()); return 16;
      case 0xE8: this.sp = this.addSP(this.fetch()); return 16;
      case 0xF8: this.hl = this.addSP(this.fetch()); return 12;
      case 0xE9: this.pc = this.hl; return 4;
      case 0xF9: this.sp = this.hl; return 8;
      case 0xF3: this.ime = false; this.eiPending = false; return 4;
      case 0xFB: this.eiPending = true; return 4;
    }
    throw new Error(`illegal opcode 0x${op.toString(16)} at 0x${((this.pc - 1) & 0xFFFF).toString(16)}`);
  }

  /* Runs until stop() returns true or maxCycles CPU cycles pass. Returns true if stopped by stop(). */
  run(stop, maxCycles) {
    const end = this.cycles + maxCycles;
    while (this.cycles < end) {
      for (let i = 0; i < 1000; i++) this.step();
      if (stop()) return true;
    }
    return false;
  }
}

/*
 * gcr62.h - DOS 3.3 GCR 6-and-2 sector codec, converts between raw 256-byte
 * sectors (.dsk, 143360 bytes) and nibblelized tracks (.nib, 6656 bytes/track)
 *
 * The encoder is a faithful port of the reference dsk2nib.c by Stephen A.
 * Edwards (sedwards@cs.columbia.edu), adapted from the PalmApple "dsk2pdb"
 * program, so that .dsk files loaded here produce the exact same nibble image
 * that converter emits.
 *
 * The decoder is its strict inverse: it scans tracks for address/data fields,
 * verifies every checksum, and only succeeds when all 16 sectors of a track
 * are present exactly once. It is used to convert back to .dsk on save, and
 * refuses (returns false) on anything non-standard so that a damaged or
 * non-DOS 3.3 track is never silently written back to a .dsk file.
 *
 * Copyright (c) 2026 - MIT license, same as the rest of reinette
 */

#ifndef __GCR62_H__
#define __GCR62_H__

#include <cstdint>
#include <cstring>

namespace gcr62 {

  constexpr int TRACKS         = 35;
  constexpr int SECTORS        = 16;
  constexpr int SECTOR_BYTES   = 256;
  constexpr int DSK_TRACK_BYTES = SECTORS * SECTOR_BYTES;                       // 4096
  constexpr int NIB_TRACK_BYTES = 0x1A00;                                       // 6656
  constexpr int DSK_FILE_BYTES  = TRACKS * DSK_TRACK_BYTES;                     // 143360
  constexpr int NIB_FILE_BYTES  = TRACKS * NIB_TRACK_BYTES;                     // 232960
  constexpr int VOLUME_NUMBER   = 254;

  // physical sector slot (address field value) -> DOS 3.3 logical sector in the .dsk file
  constexpr uint8_t DOS_SECTOR[SECTORS] = {
    0x0, 0x7, 0xE, 0x6, 0xD, 0x5, 0xC, 0x4,
    0xB, 0x3, 0xA, 0x2, 0x9, 0x1, 0x8, 0xF };

  constexpr uint8_t GCR_ENCODE[64] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF };

  // inverse table, -1 for bytes that are not valid GCR values
  struct GcrTables {
    int8_t decode[256];
    GcrTables() {
      memset(decode, -1, sizeof(decode));
      for (int i = 0; i < 64; i++)
        decode[GCR_ENCODE[i]] = i;
    }
  };

  inline const GcrTables& tables() {
    static GcrTables t;
    return t;
  }

  constexpr uint8_t SWAP_BIT[4] = { 0, 2, 1, 3 };                               // swap lower 2 bits, self-inverse


  // ---------------------------------------------------------------------------
  // encode one .dsk track (4096 bytes, DOS 3.3 logical sector order)
  // into one nibblelized track (6656 bytes)
  // ---------------------------------------------------------------------------
  inline void encodeTrack(const uint8_t* dsk, uint8_t* nib, int track,
                          uint8_t volume = VOLUME_NUMBER) {
    uint8_t* target = nib;

    auto writeByte = [&](uint8_t b) { *target++ = b; };
    auto writeSync = [&](int n)    { while (n--) writeByte(0xFF); };

    auto fmEncode = [&](uint8_t v) {                                              // 4-and-4 encoding for address fields
      writeByte((v >> 1) | 0xAA);
      writeByte(v | 0xAA);
    };

    uint8_t two[86], six[256];

    for (int slot = 0; slot < SECTORS; slot++) {

      // ---- address field ----
      writeSync(38);                                                              // inter-sector gap
      writeByte(0xD5); writeByte(0xAA); writeByte(0x96);
      fmEncode(volume);
      fmEncode((uint8_t)track);
      fmEncode((uint8_t)slot);
      fmEncode(volume ^ (uint8_t)track ^ (uint8_t)slot);
      writeByte(0xDE); writeByte(0xAA); writeByte(0xEB);
      writeSync(8);

      // ---- 6-and-2 encode the 256-byte page ----
      const uint8_t* page = dsk + DOS_SECTOR[slot] * SECTOR_BYTES;

      two[0] = SWAP_BIT[page[1] & 0x03];                                          // 86 * 3 = 258, so the first two
      two[1] = SWAP_BIT[page[0] & 0x03];                                          // bytes are encoded twice
      for (int i = 255, j = 2; i >= 0; i--, j = (j == 85 ? 0 : j + 1)) {
        two[j] = (two[j] << 2) | SWAP_BIT[page[i] & 0x03];
        six[i] = page[i] >> 2;
      }
      for (int i = 0; i < 86; i++)
        two[i] &= 0x3F;

      // ---- data field ----
      writeByte(0xD5); writeByte(0xAA); writeByte(0xAD);
      uint8_t last = 0;
      for (int i = 0x55; i >= 0; --i) {                                           // the 86 two-bit groups, in reverse
        writeByte(GCR_ENCODE[last ^ two[i]]);
        last = two[i];
      }
      for (int i = 0; i < 256; ++i) {
        writeByte(GCR_ENCODE[last ^ six[i]]);
        last = six[i];
      }
      writeByte(GCR_ENCODE[last]);                                                // checksum
      writeByte(0xDE); writeByte(0xAA); writeByte(0xEB);
    }

    while (target != nib + NIB_TRACK_BYTES)                                       // pad the rest with sync bytes
      writeByte(0xFF);
  }


  // ---------------------------------------------------------------------------
  // decode one nibblelized track (6656 bytes) back into a .dsk track (4096 bytes,
  // DOS 3.3 logical sector order). The scan is circular to handle tracks written
  // by the emulator that wrap around the end of the buffer.
  // Returns true only if all 16 sectors (0-15) are found exactly once, with the
  // announced track number, and every address and data checksum verifies.
  // ---------------------------------------------------------------------------
  inline bool decodeTrack(const uint8_t* nib, uint8_t* dsk, int track) {
    const GcrTables& T = tables();
    const int N = NIB_TRACK_BYTES;
    uint8_t have[SECTORS] = { 0 };
    int found = 0;

    auto at = [&](int i) -> uint8_t { return nib[i % N]; };                       // i may exceed N : tracks are
                                                                                    // circular and data fields wrap
    int cursor = 0;                                                               // scan position (unwrapped)
    int budget = N;                                                               // never scan more than one lap

    while (budget > 0) {
      int start = -1;                                                             // next address field prologue
      for (int k = 0; k < budget; k++)
        if (at(cursor + k) == 0xD5 && at(cursor + k + 1) == 0xAA && at(cursor + k + 2) == 0x96) {
          start = cursor + k;
          break;
        }
      if (start < 0) break;                                                       // no more address fields

      budget -= (start + 3) - cursor;                                             // consume up to the end of the
      cursor = start + 3;                                                         // prologue, whatever happens next

      int p = cursor;

      auto fmDecode = [&]() -> int {                                              // 4-and-4 decoding
        uint8_t hi = at(p++);
        uint8_t lo = at(p++);
        return ((hi & 0x55) << 1) | (lo & 0x55);
      };

      int vol  = fmDecode();
      int trk  = fmDecode();
      int sec  = fmDecode();
      int chk  = fmDecode();

      if ((vol ^ trk ^ sec) != chk) continue;                                     // spurious or damaged header :
      if (trk != track || sec < 0 || sec >= SECTORS) continue;                    // skip it, as RWTS itself would

      if (at(p) != 0xDE || at(p + 1) != 0xAA) return false;                       // address epilogue (RWTS itself
      p += 2;                                                                     // only requires DE AA : the next
                                                                                    // byte is not always EB on real disks)

      int dstart = -1;                                                            // locate the data field prologue
      for (int k = 0; k < 16; k++)                                                // (within the short sync gap)
        if (at(p + k) == 0xD5 && at(p + k + 1) == 0xAA && at(p + k + 2) == 0xAD) {
          dstart = p + k;
          break;
        }
      if (dstart < 0) return false;

      int q = dstart + 3;
      uint8_t raw[343];
      for (int k = 0; k < 343; k++) {                                             // read and un-GCR the stream
        int v = T.decode[at(q++)];
        if (v < 0) return false;                                                  // not a valid GCR byte
        raw[k] = (uint8_t)v;
      }

      for (int k = 1; k < 342; k++)                                               // undo the write-time XOR chain
        raw[k] ^= raw[k - 1];
      if (raw[342] != raw[341]) return false;                                     // data checksum

      uint8_t two[86], six[256];
      for (int j = 0; j < 86; j++)  two[j] = raw[85 - j];                         // groups were written in reverse
      memcpy(six, raw + 86, 256);

      uint8_t page[256];                                                          // rebuild the 256-byte sector
      for (int i = 0; i < 256; i++) page[i] = six[i] << 2;
      auto addLow2 = [&](int idx, int bits) { page[idx] |= SWAP_BIT[bits & 3]; };

      addLow2(171, two[0] >> 2);                                                  // the two 2-visit groups
      addLow2(85,  two[0]);
      addLow2(170, two[1] >> 2);
      addLow2(84,  two[1]);
      for (int j = 2; j < 86; j++) {                                              // the 84 3-visit groups
        addLow2(257 - j, two[j] >> 4);
        addLow2(171 - j, two[j] >> 2);
        addLow2(85  - j, two[j]);
      }

      int dest = DOS_SECTOR[sec];
      if (have[dest]++) return false;                                            // duplicate sector
      memcpy(dsk + dest * SECTOR_BYTES, page, SECTOR_BYTES);
      if (++found == SECTORS) break;                                              // complete

      budget -= q - cursor;                                                       // resume the scan just past
      cursor = q;                                                                 // this data field
    }

    for (int i = 0; i < SECTORS; i++)
      if (!have[i]) return false;                                                 // incomplete track
    return true;
  }

} // namespace gcr62

#endif

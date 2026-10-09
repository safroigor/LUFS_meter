# Measurement

## Standard

Integrated Loudness is measured according to **ITU-R BS.1770** and the
methodology referenced by **EBU R 128**.

- ITU-R BS.1770-5 (11/2023) is the current in-force revision.
- EBU R 128 v5.0 (11/2023) — programme loudness normalisation, target −23 LUFS.
- EBU Tech 3341 — "EBU Mode" metering and minimum-requirement tolerances.

## Algorithm (as implemented by the chosen library)

The measurement is delegated to **libebur128 v1.2.6** (MIT).

1. **K-weighting**, two cascaded IIR stages whose coefficients are recomputed for
   the actual sample rate:
   - high-shelf: `f0 = 1681.974450955533 Hz`, `G = +3.999843853973347 dB`,
     `Q = 0.7071752369554196`;
   - high-pass: `f0 = 38.13547087602444 Hz`, `Q = 0.5003270373238773`.
2. **Mean square** of the K-weighted samples over each gating block.
3. **Channel weighting**: surround channels (M±060, M±090, M±110) ×1.41;
   `DUAL_MONO` ×2.0; LFE (`UNUSED`) excluded.
4. **Gating**: 400 ms blocks with 75% overlap (100 ms step). Absolute gate
   −70 LUFS; relative gate −10 LU. The loudness offset is −0.691.
5. **Integrated value** = −0.691 + 10·log10(mean energy of the blocks above both
   gates).

If no block passes the absolute gate, libebur128 returns `-HUGE_VAL`; the plugin
shows an explicit "no valid value" state, never a fake `0 LUFS`.

## Supported configurations

- Channel counts: mono and stereo are the primary targets. libebur128 supports
  up to 64 channels with an ITU channel map; additional layouts are only
  reported as supported once tested.
- Sample rates: any rate between 16 Hz and 2 822 400 Hz; coefficients are
  recalculated per rate. Verified rates are listed in `docs/testing.md`.

## Real-time considerations

From libebur128 v1.2.6 source:

- In the default mode, `ebur128_add_frames_*` calls `malloc` for every 100 ms
  gating block (`ebur128_calc_gating_block`). This must not run on the audio
  thread.
- In `EBUR128_MODE_HISTOGRAM` mode no per-block allocation happens, but block
  energies are quantised to 0.1 LU bins.

The chosen design runs the whole library on a dedicated measurement thread in the
**exact (non-histogram)** mode, which keeps the audio callback free of
allocation and preserves full precision. See `docs/architecture.md` and the
architecture ADR.

`ebur128_state` must be used by a single thread only (not thread-safe).

## Limitations

- No True Peak, Momentary or Short-term LUFS in v1.
- Loudness Range (LRA) is not measured.
- The measurement is only accurate to the extent verified by the tests in
  `docs/testing.md`; no claim of full standard conformance is made before those
  tests pass.

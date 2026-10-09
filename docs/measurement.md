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

## Momentary and Short-Term loudness

Momentary (M) and Short-Term (S) are computed from the same `ebur128_state` and
the same K-weighting pass as the Integrated value; there is no second filter.
The library mode `EBUR128_MODE_I | EBUR128_MODE_S | EBUR128_MODE_TRUE_PEAK`
enables all measurements at once.

- **Momentary** is the mean square of the last **400 ms**, via
  `ebur128_loudness_momentary()`, converted with the same `-0.691` offset.
- **Short-Term** is the mean square of the last **3 s**, via
  `ebur128_loudness_shortterm()`.
- **Neither is gated.** Unlike Integrated, M and S reflect whatever is in their
  window; a signal below the `-70 LUFS` absolute gate still produces an
  (ungated) M/S reading. This is the intended distinction: Integrated applies the
  absolute and relative gates, M/S do not.
- **Warm-up.** libebur128 zero-fills the not-yet-populated part of its rolling
  buffer. The plugin therefore keeps M unavailable until a full 400 ms has been
  processed since the last prepare/reset, and S unavailable until a full 3 s
  has been processed. Until then the UI shows the em-dash placeholder (`—`).
- **Silence floor.** Values at or below `-120 LUFS` are treated as "no meaningful
  measurement" and shown as `—`. This is a display-validity floor, not integrated
  gating: it exists because the K-weighting filter's decaying tail can leave an
  extremely small but finite energy in an otherwise silent window (yielding a
  nonsense value like `-800 LUFS`) before it finally underflows to `-HUGE_VAL`.
  The floor is far below real material and below the `-70 LUFS` absolute gate.
- A silent passage fills the rolling window like any other audio and never
  resets it: M/S roll the silence through and show `—` once the window is
  (substantially) silent, while Integrated and True Peak keep their state.

## True Peak

True Peak is measured by libebur128 in `EBUR128_MODE_TRUE_PEAK`, using its
interpolator (`ebur128_true_peak()`): a 49-tap, Hanning-windowed polyphase FIR
that oversamples the signal to detect inter-sample peaks.

- Oversampling factor: **4× for sample rates below 96 kHz, 2× for 96–192 kHz,
  and none at or above 192 kHz** (libebur128's own policy). At `>= 192 kHz` the
  reported true peak therefore equals the sample peak.
- The plugin reports the **maximum true peak across all channels** observed since
  the last reset, as `20 * log10(peak)` dBTP (a linear amplitude of 1.0 is
  `0 dBTP`).
- Digital silence (`peak == 0`) is reported as unavailable (`—`), never as
  negative infinity.
- libebur128's own documentation describes the algorithm as "implementation
  defined" and warns against comparing results across library versions. The
  dependency is pinned (ADR 0004), so results are stable for a given build, but
  small differences against other true-peak meters are expected and acceptable.

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

Enabling Short-Term grows the library's internal rolling buffer from 400 ms to
3 s of K-weighted samples (e.g. ~2.3 MB at 48 kHz stereo, ~9.2 MB at 192 kHz);
True Peak adds small oversampling buffers. All of this is allocated in
`prepare()` on the message thread, never in the audio callback. True Peak adds
roughly a 49-tap/oversampled convolution per sample; Short-Term re-sums its 3 s
window on each publication. Both run on the measurement thread only.

`ebur128_state` must be used by a single thread only (not thread-safe).

## Silence and measurement continuity

Silent audio is fed to libebur128 like any other audio. Blocks whose mean square
is below the `-70 LUFS` absolute gate simply do not contribute to the integrated
value; they never clear the accumulated state. As a result:

- a silent passage during continuous playback does not reset the measurement;
- spoken-word takes separated by silent gaps read as one integrated value for
  the whole playback pass.

The other measurements behave consistently: True Peak keeps its running maximum,
and M/S roll the silence through their windows (becoming unavailable once a
window is essentially silent) without resetting anything.

Clearing the measurement is a **transport** concern, not a signal-amplitude
concern: see `docs/decisions/0007-transport-lifecycle.md` and
`docs/architecture.md`.

## Limitations

- Loudness Range (LRA) is not measured.
- True Peak at `>= 192 kHz` uses no oversampling and therefore equals the sample
  peak (libebur128 policy). The true-peak algorithm is implementation-defined and
  is not claimed to match other meters bit-for-bit.
- The M/S silence floor (`-120 LUFS`) is a display-validity choice, not part of
  the standards.
- The measurement is only accurate to the extent verified by the tests in
  `docs/testing.md`; no claim of full standard conformance is made before those
  tests pass.

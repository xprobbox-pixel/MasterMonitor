# Measurement design and limits

This development build deliberately displays no standards compliance badge. Deterministic checks and a limited independent reference comparison do not establish full conformance.

## Position and programme lifecycle

Input → 20 ms linear gain ramp for Bus Trim → analysis queue → 20 ms Mono blend and Dim gain ramps → output. Fully engaged Dim is 0.1 linear (−20 dB). Mono M=0.5(L+R), duplicated to both outputs. Mono layouts remain mono. No limiter, saturation, loudness compensation, auto normalisation, or audio oversampling. Reported latency is zero. Nonfinite inputs become zero, including in dry bypass. Finite ordinary input is otherwise unaltered in unity dry mode.

Active off and host bypass pause measurement and pass dry audio at unity. Re-entry starts a new programme at the first queued frame with the new epoch. Existing readings remain visible during a pause with a paused label. Host transport is deliberately not consulted: analysis measures all submitted samples, including stopped-transport tails and host-submitted silence; looping accumulates into the same programme. No submitted samples means elapsed time and measurements remain frozen. Reset Measurements starts again; sample-rate/layout re-preparation also resets. No host offline-render heuristics: Mono and Dim ALWAYS affect output and can affect exported audio.

The measurement worker consumes a fixed-capacity 262144-frame SPSC ring. This is analysis buffering, not audio latency. A full ring drops analysis frames, increments a visible counter, and tags subsequent frames with a new epoch. Measurements reset at the discontinuity rather than silently joining discontiguous audio. Audio processing never waits. Analysis errors are visible. The queue is ~5.46 seconds at 48 kHz, ~1.37 seconds at 192 kHz; ordinary worker lag should be much lower. UI uses a mutex only with the worker, never with the audio callback. Analysis continues when the editor is closed.

## Loudness

Pinned libebur128 1.2.6, commit in `cmake/Identity.cmake`, implements the BS.1770-4-era K-weighted EBU R128 algorithm, not RMS plus an offset. Stereo left/right each have weight 1. Mono uses one weighted channel; a duplicate right channel is explicitly marked unused for loudness. `EBUR128_MODE_S | I | TRUE_PEAK | HISTOGRAM` provides fixed-memory integrated gating: 400 ms blocks, 100 ms steps, −70 LUFS absolute gate and −10 LU relative gate. The library histogram uses 0.1 LU bins. Short-term uses the last 3 seconds without gating. Integrated is withheld until 400 ms; short-term until 3 seconds. Valid silence is −∞; startup/error/unavailable is —. Target difference is short-term measurement minus target, displayed only for finite loudness. No invented silence difference. Elapsed time counts measured frames.

## Peaks and true peak

Sample peak readings use per-batch maxima with 24 dB/second display decay. L/R holds accumulate maximum original sample magnitudes. Sample clipping latches for magnitude ≥1.0. These holds are explicitly separate from current readings.

True peak uses libebur128's reconstruction path: a 49-tap Hann-windowed sinc polyphase FIR, 4× below 96 kHz, 2× below 192 kHz, original-rate at ≥192 kHz. It never touches audible output. The maximum of both channels is retained; comparing each batch reconstruction maximum with the current limit latches exceedance. Reset clears the maximum and latch without resetting loudness. The measured maximum, limit and latch are labelled. “Within TP Limit” is an indication about this measured programme, not a limiter or streaming certification. “No Signal” uses decaying energy; a previous latch remains separately labelled during silence. FIR startup/end transients and finite filter accuracy limit the estimate. No claim of full inter-sample conformance is made. See VALIDATION.md and raw reference results for tested accuracy.

## VU

Independent mean-square envelopes use a 65 ms one-pole time constant; displayed sine-peak-equivalent amplitude is sqrt(2 × mean-square). A steady −18 dBFS peak sine reads 0 VU with default calibration. The envelope reaches within approximately 0.05 dB of the steady sine amplitude around 300 ms. This is a useful digital diagnostic convention, not a certified IEC/ANSI VU meter. Needle display spans −20 to +3 VU; numeric values can exceed that scale.

## Spectrum

4096-point periodic Hann FFTs, non-overlapping, on each channel independently. Sum both channel spectral powers and divide by N² × 0.375: this gives average stereo sine-peak-equivalent band power. L/R never cancel, including inverted material. Bands integrate bins between geometric means of adjacent labelled centres. Edges are 20 Hz and 22.05 kHz, clipped to Nyquist. Centres: 31, 63, 125, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 8000, 10000, 12000, 16000, 20000 Hz. A centre at or above Nyquist is unavailable. At low frequencies FFT bin resolution limits precision. Band envelope falls at 20 dB/s and rises immediately; holds persist until reset. Display scale −60 to 0 dB, underlying values extend lower. This irregular arrangement is NOT a third-octave analyser.

## Scope and history

Scope rising L zero-crossing trigger searches within a preceding half timebase. Without a crossing, it free-runs on the most recent timebase. Both channels share the trigger. 512 min/max pairs preserve peaks in each display column. The ring holds 16384 samples; timebases are 5–100 ms, bounded to 16000 frames at high rates. Display gain and zoom multiply graphics only. Freeze holds scope and goniometer, not other analysis or audio. History preserves extrema of both channels in 500 sequential 20 ms bins (10 seconds) and continues during scope freeze. Timebase is not synchronised to musical tempo.

## Stereo diagnostics

Goniometer: horizontal S=(L−R)/2, vertical M=(L+R)/2, drawn upwards. Decimated points show orientation and phase; the separate scope/history preserve peaks. Correlation = smoothed E(LR)/sqrt(E(L²)E(R²)) with 300 ms energy smoothing. If either channel energy is below 1e−12 it is unavailable; silence is not valid zero correlation.

Width = 200 × E(S²)/(E(M²)+E(S²)), 0–200%. Mono=0%, independent equal-power channels≈100%, inverted identical=200%. It is a diagnostic convention, not an industry-standard width unit. Silence/insufficient energy is —. Anti-phase remains identifiable through negative correlation and the width scale.

## State, safety and thread model

Only trim, Dim, Mono and Active are host automatable. Targets, VU calibration and display controls are non-automatable preferences saved in DAW state and local settings. Schema 1 state ignores unknown IDs, rejects unknown schemas, clamps parameter ranges and rejects nonfinite values. UI bounds are constrained. User preferences are stored using JUCE PropertiesFile under Application Support (macOS) or the equivalent per-user Windows app data directory. New instances default audio controls safely; only DAW/standalone state restores monitoring choices. `MM_NO_USER_SETTINGS=1` isolates automated tests from user preferences.

JUCE's standalone wrapper provides audio-device settings and its feedback-protection input mute. Input is muted on first launch; the user must explicitly disable mute to monitor. Device changes and user mute choices are persisted by the wrapper. Review routing and keep speakers low before enabling a live input. No audio/telemetry is uploaded or written by the analyser; test fixtures are synthetic and temporary.

# Work Items

This is the current backlog of useful future work.

## High Priority

- [x] Move LVGL/UI updates out of the 100 ms timer ISR.
- [x] Add division-by-zero guards for `Qs`, `P2`, and timer/progress calculations.
- [x] Improve timer precision by avoiding `(int)time_sec * 10` truncation.
- [x] Add VERSION auto-bump, firmware archives, and rollback scripts.
- [x] Speed up build/upload with project-local incremental cache + parallel jobs.
- [x] Harden build/upload scripts: CleanSketch vs Clean, upload retries, fingerprint skip, doctor, port detect fix.
- [ ] Confirm and document exact hardware model and final pinout.
- [ ] Verify N.O/N.C behavior on the real machine with a safe test plan.

## Medium Priority

- [ ] Make the Test tab a real, safe diagnostic page or clearly disable unsupported output toggles.
- [ ] Implement or remove the incomplete Touch enable/disable behavior.
- [ ] Decide how process parameters should persist and wire Save accordingly.
- [ ] Clean multilingual text encoding and font behavior in `actions.c`.
- [x] Add include guards where missing, especially `variables.h`.

## Agent Tooling

- [x] Add serial `PING`, `STAT`, navigation, touch injection, and `SAFE`.
- [x] Add real LCD `SHOT` capture through PSRAM.
- [x] Add `tools/hmi_agent.py`.
- [x] Add safe `smoke` workflow.
- [ ] Add richer menu traversal tests after UI behavior is hardened.
- [ ] Add optional screenshot comparison thresholds for visual regression detection.

## Documentation

- [x] Move canonical project knowledge into English docs.
- [x] Document LCD driver and color fixes.
- [x] Document LVGL image format fix.
- [x] Document real-device feedback workflow.
- [ ] Update docs after every meaningful hardware or scenario discovery.

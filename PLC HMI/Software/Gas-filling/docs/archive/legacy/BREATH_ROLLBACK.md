# General Settings — Breath (rollback note)

**Pre-Breath baseline archive:** `artifacts/builds/v1.2.61`

**Breath build archive:** `artifacts/builds/v1.2.63` (compiled OK; flash wrote — if agent PING fails, unplug/replug USB then retry)

Rollback firmware only:

```powershell
.\scripts\rollback.ps1 -Version 1.2.61 -Port COM3
```

New source files (delete these + revert `screens.c` / `actions.c` hooks to undo Breath in tree):

- `GasFilling_HMI/ui_breath.c`
- `GasFilling_HMI/ui_breath.h`

Behavior on General tab:

- Soft blue pulse (“Breath”) while Setting is open
- **Tap** = buzzer preview
- **Hold** = Focus (hides Language/Sound/Touch card)
- **Double-tap** = day / night page theme
- Card rows: Language · Sound · Touch

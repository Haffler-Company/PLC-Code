#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Copy live PLC/HMI variables into legacy web-app globals before HAND_SHAKE. */
void webapp_sync_vars_from_plc(void);

/** Apply legacy web-app globals (after mobile edit) back into PLC storage + flow vars. */
void webapp_apply_vars_to_plc(void);

/** Broadcast fresh state to connected web clients (no-op if web-app is off). */
void webapp_push_state_to_clients(void);

#ifdef __cplusplus
}
#endif

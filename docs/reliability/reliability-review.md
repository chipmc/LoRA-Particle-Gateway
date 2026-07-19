# MAFC Gateway Reliability Review

## Document purpose and scope

This living engineering notebook records comparative reliability findings between the **MAFC Gateway** and the **Reference Gateway**. It separates observed facts from hypotheses and future investigation, and provides traceability from evidence through decision and validation.

This notebook does not authorize implementation. The process and approval gates in [`../AI_WORKFLOW.md`](../AI_WORKFLOW.md) govern all reliability changes. The Reference Gateway is read-only and is used to identify potentially relevant behaviors, not as a code donor or universally superior implementation.

## Baselines

| Role | Repository | Local path | Branch/tag | Commit | Mutability |
|---|---|---|---|---|---|
| Implementation target | `LoRA-Particle-Gateway` | `/Users/chipmc/Documents/Maker/Particle/Projects/LoRA-Particle-Gateway` | `master` | `87e72a0` | Writable after approval |
| Reference | `LoRA-Particle-Gateway-Legacy` | `/Users/chipmc/Documents/Maker/Particle/Projects/LoRA-Particle-Gateway-Legacy` | `v27.0.0-production` | `2b90ac8` | Read-only |

The **MAFC Gateway** is the implementation target. The **Reference Gateway** is the read-only comparison source. Both repositories have diverged substantially and do not share a practical backport path. Comparisons must therefore be semantic and MAFC-specific rather than patch- or history-based.

## Known context

### Established facts and constraints

- The MAFC Gateway operates indoors, is wall-powered, and does not sleep.
- It supports up to 50 nearby, relatively dense nodes.
- Nodes report hourly and on every occupancy change; occupancy is the number of people in a room.
- The gateway owns extensive per-node configuration, including space number, scanning pattern, and in/out orientation.
- The system resets during closing hours and during a midday cleaning period.
- Existing Node firmware is frozen for this effort and must remain unchanged.
- The existing gateway-to-node protocol must remain compatible.
- MAFC-specific application behavior must be preserved.
- A hard fault in the MAFC Gateway FRAM erase path was identified and corrected before this formal review.

### Investigation leads, not proven issues

The following are suspected areas to investigate. Their presence here does not establish a defect, causal relationship, or need for change:

- NodeDB performance.
- Memory fragmentation.
- Scaling behavior at up to 50 nodes.
- Allocation behavior in packet and configuration paths.
- Persistence behavior.
- Event bursts and publish queuing.
- Cleaning-period and closing-hours reset paths.

No reliability conclusion should be drawn from a repository difference alone.

## Investigation domains

Use this checklist to track coverage. Checking an item means it was investigated and documented; it does not mean a defect was found.

- [ ] Application lifecycle and main loop.
- [ ] LoRa receive and decode path.
- [ ] Node lookup, insert, update, expiry, and deletion.
- [ ] NodeDB capacity and scaling.
- [ ] Per-node configuration ownership.
- [ ] Dynamic allocation and `String` use.
- [ ] JSON or serialization allocations.
- [ ] FRAM or other persistence lifecycle.
- [ ] Publish queues and event bursts.
- [ ] Cloud reconnect and recovery.
- [ ] Timers and callbacks.
- [ ] Cleaning-period and closing-hours resets.
- [ ] Heap and largest-free-block observability.
- [ ] Reset-reason observability.
- [ ] Differences caused intentionally by MAFC requirements.

## Reliability item template

Copy this section for each evidence-backed item. Assign the next unused sequential identifier. Do not create an item solely because the repositories differ.

```markdown
## RR-xxx — Title

- **Status:** Open
- **Date opened:** YYYY-MM-DD
- **Owner:** Name or role

### Observed symptom or question

### MAFC evidence

### Relevant MAFC files and functions

### Reference Gateway behavior

### Relevant Reference files and functions

### Reliability hypothesis

### Applicability analysis

### Minimal proposed change

### Alternatives considered

### Risks

### Protocol/Node compatibility impact

### Instrumentation required

### Bench validation

### Soak validation

### Rollback

### Decision

### Implementation commit

### Results
```

## Reliability items

## RR-001 — FRAM erase hard fault

- **Status:** Implemented (before this formal review; historical validation and closure records remain to be documented)
- **Date opened:** To be documented from repository history
- **Owner:** To be documented from repository history

### Observed symptom or question

A hard fault in the MAFC Gateway FRAM erase path was identified.

### MAFC evidence

The issue and its correction predate this formal review. Exact evidence is to be documented from repository history.

### Relevant MAFC files and functions

To be documented from repository history.

### Reference Gateway behavior

A corresponding issue had previously been addressed in the Reference Gateway. Exact implementation details are to be documented from repository history.

### Relevant Reference files and functions

To be documented from repository history.

### Reliability hypothesis

Historical: correcting the faulty FRAM erase behavior prevents the identified hard fault. The original hypothesis wording and supporting evidence are to be documented from repository history.

### Applicability analysis

The MAFC Gateway issue has already been fixed. This item demonstrates that a proven reliability behavior may transfer between sibling repositories when the MAFC failure and applicability are established. It does not establish that other Reference Gateway differences should be adopted.

### Minimal proposed change

Already implemented before this formal review. Exact implementation details are to be documented from repository history. No additional change is proposed by this item.

### Alternatives considered

To be documented from repository history.

### Risks

Historical risks and their disposition are to be documented from repository history.

### Protocol/Node compatibility impact

To be documented from repository history. No Node firmware change is authorized or proposed.

### Instrumentation required

None proposed by this historical entry. Any instrumentation used during the original diagnosis is to be documented from repository history.

### Bench validation

To be documented from repository history.

### Soak validation

To be documented from repository history.

### Rollback

To be documented from repository history.

### Decision

The MAFC Gateway issue was corrected before this formal review. Do not revisit or alter the fix unless documentation evidence shows that an accuracy correction is required and a separately approved scope authorizes it.

### Implementation commit

To be documented from repository history. No commit hash is asserted here.

### Results

The FRAM erase hard fault is recorded as already fixed. Exact test results and closure evidence are to be documented from repository history.

## RR-002 — Mode-1 cloud timeout must not lead to sleep

- **Status:** Bench Validated (candidate 24.5 passed 2026-07-19)
- **Date opened:** 2026-07-18
- **Owner:** Codex (focused investigation requested by Chip)

### Observed symptom or question

The MAFC Gateway is wall-powered and is required to remain awake so it can serve nearby nodes continuously. Static control-flow analysis shows that a Particle Cloud connection attempt that does not succeed within 10 minutes changes connectivity mode from 1 to 0, enters `DISCONNECTING_STATE`, and then enters `SLEEPING_STATE` when the publish queue permits. During sleep the gateway cannot receive or acknowledge LoRa reports or occupancy-change events.

This is a credible failure mode established by reachable source code. Whether it has occurred on the bench or deployed gateway has not yet been established from runtime logs.

### MAFC evidence

In `src/LoRA_Particle_Gateway.cpp`:

1. `setup()` unconditionally sets connectivity mode 1 and enters `CONNECTING_STATE`.
2. Normal mode-1 operation returns from a successful cloud connection to `LoRA_STATE` and stays awake.
3. After 600,000 ms without a cloud connection, `CONNECTING_STATE` sets connectivity mode 0 and enters `DISCONNECTING_STATE`.
4. After at least 90 seconds, when `PublishQueuePosix::getCanSleep()` is true, `DISCONNECTING_STATE` enters `SLEEPING_STATE`.
5. `SLEEPING_STATE` calls `System.sleep()` in `ULTRA_LOW_POWER` mode.
6. After waking, mode 0 limits LoRa listening to `DEFAULT_LORA_WINDOW` before another sleep/reconnect cycle.

The mode transition is therefore sufficient to reduce LoRa availability during a WiFi or Particle Cloud outage. No speculative memory or NodeDB claim is needed for this item.

### Relevant MAFC files and functions

- `src/LoRA_Particle_Gateway.cpp`
  - `setup()`
  - `loop()` cases `LoRA_STATE`, `CONNECTING_STATE`, `DISCONNECTING_STATE`, and `SLEEPING_STATE`
- `src/config.h`
  - `DEFAULT_LORA_WINDOW`
  - `STAY_CONNECTED`
- `src/MyPersistentData.h` and `src/MyPersistentData.cpp`
  - `sysStatusData::get_connectivityMode()`
  - `sysStatusData::set_connectivityMode()`
- `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp`
  - `PublishQueuePosix::getCanSleep()` state dependencies

### Reference Gateway behavior

At Reference commit `2b90ac8`, the corresponding state machine also changes to mode 0 and proceeds toward sleep after a 10-minute connection failure. It adds stronger connection diagnostics, a long connection guard, queue-drain observability, and a disconnect hard timeout, but its normal architecture intentionally includes bounded LoRa windows and sleep based on operating hours and power state.

The Reference behavior is relevant as applicability evidence, not as code to transplant: its failure-to-sleep behavior is consistent with its own power-managed architecture and is not suitable for the wall-powered, continuously listening MAFC operating model.

### Relevant Reference files and functions

- `src/LoRA_Particle_Gateway.cpp`
  - `loop()` cases `CONNECTING_STATE`, `DISCONNECTING_STATE`, `SLEEPING_STATE`, and `LoRA_STATE`
  - Particle connection guard and queue-drain logging
- `src/config.h`
  - `PARTICLE_CONNECT_GUARD_TIMEOUT_MS`
  - `DISCONNECTING_HARD_TIMEOUT_MS`
- `src/GatewayPlatform.h`
  - `MIN_CLOUD_COMMAND_WINDOW_MS`

### Reliability hypothesis

If a failed cloud connection attempt leaves MAFC connectivity mode at 1, returns control to `LoRA_STATE`, and suppresses immediate reconnection attempts for a bounded retry interval, then the gateway will continue receiving and acknowledging LoRa traffic throughout a cloud outage while still retrying cloud recovery periodically.

### Applicability analysis

This behavior applies specifically to the MAFC Gateway because it is wall-powered, does not intentionally sleep, and must accept hourly and occupancy-change reports from up to 50 nearby nodes. Preserving LoRa reception during a cloud outage does not require a Node firmware or protocol change.

The Reference Gateway's intentional sleep and battery-management behavior must remain separate. This proposal does not generalize the MAFC always-awake policy back into the Reference Gateway.

### Minimal proposed change

Limit the implementation to `src/LoRA_Particle_Gateway.cpp`:

1. Add RAM-only state that records a failed cloud attempt and its retry start time.
2. On the existing 10-minute connection timeout, keep connectivity mode 1, record the failure/cooldown, and return to `LoRA_STATE` instead of entering `DISCONNECTING_STATE`.
3. While the retry cooldown is active, do not let the existing hourly `lastConnection` check immediately send the gateway back to `CONNECTING_STATE` on every loop.
4. Retry after the existing one-hour operational cadence and clear the failure state after a successful connection.

The retry marker does not need persistence. A reboot should make a fresh cloud attempt, as it does now.

### Files expected to change

- `src/LoRA_Particle_Gateway.cpp`
- `docs/reliability/reliability-review.md` for implementation and validation results

No library, build, persistent-data layout, Node firmware, protocol, or Reference Gateway file is expected to change.

### Explicit non-goals

- Do not remove the general sleep state or mode-0 behavior; those paths may still support other historical configurations.
- Do not refactor the full state machine or connectivity-mode model.
- Do not copy the Reference Gateway connection architecture.
- Do not change the 10-minute initial connection-attempt timeout without separate evidence.
- Do not change Particle event names or downstream integrations.
- Do not add offline replay for node events that current firmware declines to enqueue while Particle is disconnected; investigate that separately if required.
- Do not change Node retry, reporting, acknowledgement, or packet behavior.

### Alternatives considered

- **Remain indefinitely in `CONNECTING_STATE`:** rejected because LoRa receive handling does not run there.
- **Return directly to LoRa with no retry cooldown:** rejected because the stale `lastConnection` check would immediately re-enter `CONNECTING_STATE`, leaving only a brief receive poll between repeated 10-minute attempts.
- **Write the attempt time into `lastConnection`:** rejected because that field represents a successful connection and is published as an operational metric.
- **Remove all sleep code or connectivity modes:** rejected as unnecessary scope expansion.
- **Copy Reference Gateway behavior:** rejected because the Reference Gateway intentionally sleeps and has different power and operating constraints.

### Risks

- Cloud recovery can be delayed by up to the selected retry interval after service becomes available, unless Particle's background connection attempt succeeds sooner.
- The RAM-only cooldown resets on reboot, causing an immediate connection attempt; this is intentional but must be covered in bench testing.
- Particle may reconnect in the system thread while the application is in the cooldown. The implementation must clear or reconcile cooldown state when a connection is observed so metrics and time sync remain coherent.
- Continued LoRa reception during a cloud outage can update node state while current firmware does not enqueue all disconnected node publishes. This proposal preserves radio service but does not solve offline event replay.
- Incorrect cooldown arithmetic could cause reconnect thrashing or suppress retries; use rollover-safe elapsed-time comparison.

### Protocol/Node compatibility impact

None expected. LoRa packet layouts, tokens, alert codes, acknowledgements, timing fields sent to nodes, Node firmware, and per-node configuration remain unchanged. The intended effect is to keep the existing LoRa receive/acknowledgement path available during cloud failure.

### Instrumentation required

Use concise transition logs for:

- cloud connection timeout and start of retry cooldown;
- return to continuous LoRa service;
- retry eligibility after cooldown;
- background cloud reconnection observed during cooldown;
- cooldown cleared after successful cloud handling.

No new persistent diagnostic field is required for the first bench test. Existing state-transition, LoRa receipt, connection, and free-memory logs should be retained.

### Bench validation

1. Record the baseline boot and healthy mode-1 sequence with WiFi and Particle Cloud available.
2. Remove WAN/Particle Cloud reachability while leaving the gateway powered. Where practical, test both loss of WiFi association and loss of upstream cloud reachability.
3. Leave the outage in place beyond the existing 10-minute connection timeout.
4. Confirm the candidate logs the timeout, returns to `LoRA_STATE`, and never enters `DISCONNECTING_STATE` or `SLEEPING_STATE` because of that timeout.
5. During the cooldown, send representative hourly and occupancy-change packets from existing unmodified nodes. Confirm each is received and acknowledged and that node database/occupancy state updates remain correct.
6. Restore network/cloud access. Confirm background recovery or the bounded retry succeeds, normal cloud handling resumes, and the cooldown clears.
7. Repeat with a power cycle during cooldown; confirm boot performs a fresh connection attempt and LoRa service remains recoverable.
8. Confirm scheduled closing-hours and cleaning-period resets retain existing behavior.

Pass criteria:

- No sleep or LoRa radio shutdown caused by cloud connection failure.
- Existing nodes continue to receive protocol-compatible acknowledgements during the outage.
- Cloud connection is retried within the documented bound and recovers without manual reset after service restoration.
- No unexpected reset, watchdog event, persistent-data reinitialization, or occupancy corruption.

### Soak validation

Run at least 48 hours on the bench with the expected MAFC node population or a representative traffic generator:

- include at least one cloud outage longer than 10 minutes and one lasting beyond a full retry interval;
- include occupancy-change bursts and hourly reports during the outage;
- restore connectivity and verify recovery;
- monitor state transitions, acknowledgements, reset count/reason if available, free heap, largest free block if instrumented separately, publish-queue depth, and node/space state.

Pass criteria are continuous LoRa acknowledgement coverage, no failure-induced sleep, bounded cloud retry/recovery, no escalating memory loss, and preserved scheduled reset behavior.

### Rollback

Revert the focused implementation commit and flash the last bench-approved MAFC firmware. The proposed RAM-only state requires no FRAM migration or cleanup. Keep the RR documentation and record the rollback reason and observed results.

### Decision

Chip explicitly approved a targeted first-step fix on 2026-07-18. Implementation is limited to the documented mode-1 cloud-failure path. Bench and soak approval remain pending.

### Implementation commit

`a0bf75d` (`fix: keep LoRa active after cloud timeout`) on branch `maintenance/rr-002-cloud-failure-lora`.

### Architecture review

Architecture review completed on 2026-07-19 with no blocking findings. The implementation conforms to the approved RR-002 scope:

- the mode-1 timeout no longer changes connectivity mode or enters the disconnect/sleep path;
- LoRa packet reporting retains priority over cloud retry handling;
- the one-hour application retry cooldown uses rollover-safe elapsed-time arithmetic;
- a background Particle Cloud reconnection is detected and reconciled;
- non-mode-1 timeout behavior, persistent layouts, Node firmware, and the LoRa protocol are unchanged.

The Particle system thread can continue attempting cloud recovery during the application cooldown. This is intentional: the cooldown prevents the application state machine from spending repeated 10-minute periods in `CONNECTING_STATE`, while allowing earlier background recovery.

This review initially cleared the candidate for bench flashing. That clearance was superseded by the failed bench test on 2026-07-19. The implementation does not provide continuous LoRa service while the application remains in `CONNECTING_STATE`, and the tested device subsequently entered the disconnect/sleep path.

### Results

Static evidence and Reference applicability comparison completed on 2026-07-18.

Implementation completed on 2026-07-18:

- added a RAM-only, rollover-safe one-hour retry cooldown;
- preserved mode 1 after its 10-minute cloud connection timeout;
- returned to `LoRA_STATE` instead of `DISCONNECTING_STATE` on that mode-1 timeout;
- retained legacy timeout behavior for non-mode-1 operation;
- handled background cloud recovery during cooldown;
- preserved received-packet reporting priority before cloud retry handling.

Local P2 user-module compilation succeeded with Particle Device OS 6.4.1 and the installed ARM toolchain. The binary was generated under `/tmp/rr002-local-build`, outside the repository. Particle binary inspection passed and reported a P2 application module depending on system module 6401. The reviewed candidate SHA-256 is `5667c59462e9e4157a09e5289a45ce514ee2b9090c6ec9d410c9568982f25837`. `git diff --check` passed. Bench evidence has not yet been collected.

On 2026-07-19, Chip authorized flashing the reviewed candidate. Codex executed the Particle Cloud flash to `Morrisville-MAFC` (`e00fce68772083a2d0b7df4e`, Product 20952), and the device subsequently reported online. Having an AI execute the flash did not preserve the intended human-approver boundary. The governing workflow now makes Chip the exclusive operator for all firmware flashes as well as Git commits and pushes.

Bench validation failed on 2026-07-19:

- Internet access was removed at approximately 16:55 SGT.
- A node report at 16:57 was received and acknowledged while the application remained in `LoRA_STATE`.
- At approximately 17:01 the application logged `From LoRA to Connecting`.
- Node reports at approximately 17:02 and 17:06 failed because the gateway was not listening while in `CONNECTING_STATE`.
- After ten minutes the gateway logged `Failed to connect in 10 minutes - giving up` and `From Connecting to Disconnecting`.
- After the existing 90-second disconnect window it logged `From Disconnecting to Sleeping`, powered down Wi-Fi, and scheduled sleep until 22:00. A subsequent node report also failed.
- The test therefore failed both continuous LoRa acknowledgement coverage and the requirement that cloud failure not lead to sleep.

The reviewed binary contains the new mode-1 timeout message, but firmware point release `24.4` was unchanged and no startup candidate identifier was logged. Subsequent read-only device inventory resolved the ambiguity: the serial log identifies the tested P2 as `0a10aced202194944a04084c` (`LoRA-Gateway-Dev-2`, Product 38211), while the candidate had been sent to `e00fce68772083a2d0b7df4e` (`Morrisville-MAFC`, Product 20952), a different hardware platform. The tested P2 therefore remained on its pre-existing version-24/point-release-24.4 firmware and executed the historical `giving up` path. Candidate `a0bf75d` was not exercised by this bench run.

RR-002 is returned to **Investigating**. Candidate `a0bf75d` is rejected for further deployment. The revised proposal must keep LoRa processing active during cloud connection work, prevent mode-1 execution from entering disconnect or sleep as a defensive invariant, log runtime connectivity mode at relevant transitions, and identify the next candidate as point release `24.5` in an explicit startup log. Revised scope and validation require Chip's approval before implementation.

### Revised architecture proposal for investigation

The next design must treat Particle Cloud connectivity as background activity while MAFC operates in mode 1. `SYSTEM_THREAD(ENABLED)` and `SYSTEM_MODE(SEMI_AUTOMATIC)` already allow the system thread to attempt cloud connectivity without requiring the application to remain in `CONNECTING_STATE`. The observed 20-minute outage demonstrated that LoRa receive and acknowledgement processing can continue while the system thread repeatedly attempts cloud recovery.

Proposed mode-1 invariants:

1. After initialization, mode 1 enters and remains in `LoRA_STATE` for normal application service, including while Particle Cloud is unavailable.
2. Mode 1 must not enter `CONNECTING_STATE` merely for startup, an hourly connection check, or background cloud recovery.
3. Mode 1 must not enter `DISCONNECTING_STATE` or execute `System.sleep()` because of cloud connection state or timeout.
4. `Particle.connect()` may initiate or preserve the system-thread connection request, but application code must continue calling the LoRa receive path on every normal loop.
5. Cloud-success work currently owned by `CONNECTING_STATE`—successful-connection timestamp and duration, time synchronization, gateway webhook/reporting, and retry-state reconciliation—must be preserved without introducing a new blocking LoRa outage.
6. Legacy non-mode-1 behavior remains unchanged unless investigation proves that preserving it prevents a safe mode-1 implementation.
7. Defensive checks at the disconnect and sleep boundaries must return mode 1 to `LoRA_STATE` before radio shutdown or `System.sleep()`, even if an unexpected upstream transition occurs.
8. Candidate identity and relevant runtime mode must be visible in logs. The next candidate uses point release `24.5`, logs `Firmware point release: 24.5` at every startup, and logs connectivity mode when cloud maintenance begins, succeeds, or is deferred.

This is a proposal for investigation, not approved implementation scope.

### Investigator validation brief

Before implementation, the Investigator must provide source-backed answers to all of the following:

1. Enumerate every writer and reader of `connectivityMode`, including initialization, Particle commands, persistence, and timeout behavior. Explain a credible path to the observed `giving up` branch or state that the running-image ambiguity prevents a conclusion.
2. Enumerate every transition into `CONNECTING_STATE`, `DISCONNECTING_STATE`, and `SLEEPING_STATE`, and prove whether the proposed mode-1 invariants close every cloud-failure path to LoRa unavailability.
3. Confirm from Particle Device OS behavior or authoritative documentation that `Particle.connect()` is nonblocking with `SYSTEM_THREAD(ENABLED)` and that the connection request remains active while application code executes in `LoRA_STATE`.
4. Trace all work currently performed on successful connection. Identify which operations are blocking, especially `waitUntil(Particle.syncTimeDone)`, and propose how to preserve their semantics without suspending LoRa reception.
5. Trace the hourly gateway webhook, `lastConnection`, connection-duration, daily counter reset, publish-queue, and time-sync behaviors. Identify any behavior that would be lost or repeated by removing mode-1 transitions into `CONNECTING_STATE`.
6. Confirm that LoRa receive/acknowledgement processing is safe while the Particle system thread is attempting Wi-Fi and cloud recovery, using both source structure and the successful outage evidence.
7. Check the exact candidate-identification mechanism, including `currentPointRelease` storage size and startup-log placement. Confirm the actual P2 bench device, Product, development-device state, and compatibility with the candidate build before handoff.
8. Produce a proposed minimal file set, risks, rollback, and updated bench cases. The bench cases must include boot without internet, loss before an hour boundary, reports throughout the former ten-minute connection window, recovery, an unexpected mode value, and proof that neither disconnect nor sleep is reachable in mode 1.

The Investigator must not edit firmware. Any uncertainty or expanded file requirement returns to the Architect and Chip before implementation.

### Architect disposition of Investigator report

The Investigator report received on 2026-07-19 is accepted with one material identity amendment. Its source and Particle threading conclusions support the revised architecture: with `SYSTEM_THREAD(ENABLED)` and `SYSTEM_MODE(SEMI_AUTOMATIC)`, mode-1 application code can remain in `LoRA_STATE` while Device OS handles connection and automatic reconnection. The blocking `waitUntil(Particle.syncTimeDone)` and the cloud-success housekeeping currently concentrated in `CONNECTING_STATE` must be separated from LoRa availability.

The Architect's read-only device query resolved the report's remaining Product uncertainty. The P2 that generated the bench serial log is `0a10aced202194944a04084c`, named `LoRA-Gateway-Dev-2`, in Product 38211. It reports platform P2, Device OS 6.4.1, firmware version 24, and development-device status enabled. It is not the Product 20952 device previously targeted. Particle documentation states that development devices are protected from automatic Product firmware updates, so Product auto-replacement is not a blocker for the correct P2 target. Startup point-release evidence remains mandatory.

The revised implementation may proceed only after Chip approves this bounded scope:

1. Candidate point release is `24.5`; log `Firmware point release: 24.5` during every startup before cloud connection work.
2. In mode 1, initialize Particle cloud connection asynchronously and enter `LoRA_STATE` rather than parking the application in `CONNECTING_STATE`.
3. Remove mode-1 hourly and recovery transitions from `LoRA_STATE` into `CONNECTING_STATE`. Device OS retains responsibility for background retry after `Particle.connect()`.
4. Preserve hourly gateway reporting, daily counter reset, `lastConnection`, connection-duration, time-sync initiation/completion, signal logging, publish-queue service, and cloud-recovery observability through bounded maintenance executed while LoRa listening remains active.
5. Replace the blocking time-sync wait with initiation plus later polling of `Particle.syncTimeDone()` from normal loop processing.
6. Add defensive mode-1 guards at `CONNECTING_STATE`, `DISCONNECTING_STATE`, and `SLEEPING_STATE` so an unexpected transition returns to `LoRA_STATE` before LoRa radio shutdown, Particle disconnect, or `System.sleep()`.
7. Log connectivity mode and point release at mode-1 cloud-maintenance, recovery, and defensive-guard events.
8. Remove the rejected cooldown state only where it is made obsolete by this design. Do not change non-mode-1 behavior, persistence layout, libraries, Node firmware, LoRa protocol, downstream event names, or integrations.

Expected implementation file set remains `src/LoRA_Particle_Gateway.cpp` plus this RR record for implementer results. If implementation requires another firmware or library file, returns a blocking call to the LoRa path, or changes existing protocol/persistence behavior, stop and return for approval.

### Scope-separation decision

On 2026-07-19, Chip directed that the two observed defects be fixed and validated one at a time. This decision supersedes the combined eight-point implementation scope above; that combined proposal remains as investigation history only.

RR-002 is now limited to the first defect: after ten minutes of unsuccessful cloud connection in mode 1, the gateway must return to `LoRA_STATE` and must not transition through `DISCONNECTING_STATE` into `SLEEPING_STATE`. The known loss of LoRa reception during the preceding connection attempt is explicitly excluded from RR-002 and recorded separately as RR-003.

Approved candidate-24.5 scope:

1. Retain or reapply the focused mode-1 timeout behavior that preserves mode 1, starts the bounded retry cooldown, and returns to `LoRA_STATE` instead of `DISCONNECTING_STATE`.
2. Preserve the existing non-mode-1 timeout path without modification.
3. Set `currentPointRelease` to `24.5` and log `Firmware point release: 24.5` during every startup before cloud connection work.
4. At entry to `CONNECTING_STATE` and at its ten-minute timeout, log the point release and current connectivity mode so the selected branch is unambiguous.
5. Do not redesign startup cloud handling, hourly transitions, time synchronization, or connection-success housekeeping in this item.
6. Do not add the proposed mode-1 state-boundary guards in this item; those could mask evidence needed to validate the focused timeout branch and belong with the broader RR-003 state redesign if approved later.
7. Do not change libraries, persistence layout, Node firmware, LoRa protocol, event names, integrations, or Reference Gateway files.

Expected implementation files are `src/LoRA_Particle_Gateway.cpp` and this RR record. Candidate preparation must stop if another firmware or library file is required.

RR-002 candidate-24.5 bench pass criteria:

- Startup identifies point release `24.5` and connectivity mode 1.
- During the first ten minutes in `CONNECTING_STATE`, failed node reports are expected and are recorded as RR-003 evidence, not an RR-002 failure.
- At the ten-minute timeout, the log identifies point release `24.5`, mode 1, and the return-to-LoRa timeout branch.
- The next application transition is `From Connecting to LoRA`; no `From Connecting to Disconnecting`, `From Disconnecting to Sleeping`, radio shutdown, or `System.sleep()` occurs.
- A node report immediately after the timeout is received and acknowledged.
- Restoring internet produces automatic cloud recovery without manual reset.

### Candidate 24.5 implementation results

Candidate 24.5 was prepared on 2026-07-19 within the approved scope:

- retained the existing focused mode-1 timeout behavior, bounded one-hour retry cooldown, and received-packet reporting priority;
- preserved connectivity mode 1 and the direct `CONNECTING_STATE` to `LoRA_STATE` transition on the ten-minute mode-1 timeout;
- left the existing non-mode-1 timeout path unchanged;
- changed `currentPointRelease` from `24.4` to `24.5`;
- added the exact startup log `Firmware point release: 24.5` before cloud connection work; and
- added point-release and current-connectivity-mode logs at entry to `CONNECTING_STATE` and at its ten-minute timeout.

No RR-003 behavior, mode-1 state-boundary guard, startup/cloud-success/time-sync redesign, persistence or protocol change, library change, or Reference Gateway change was included. Bench validation has not been performed.

Local validation results:

- `git diff --check` passed.
- A local P2 user-module build completed successfully with Particle Device OS 6.4.1, Workbench buildscripts 1.17.2, and ARM GCC 10.2.1.
- Particle binary inspection reported a valid P2 application module number 1 at module version 10, Product firmware version 24, dependent on system module 1 version 6401, with module UUID `6b7f5904f1aa1c5336e6496484790163fb6e2cb2451320311c6044bd932c921c`.
- The generated binary is `/tmp/rr002-candidate-24.5/LoRA-Particle-Gateway.bin` (94,208 bytes).
- Binary string inspection found the exact startup literal `Firmware point release: 24.5` and both new `CONNECTING_STATE` diagnostic formats.
- Binary SHA-256: `54b7b6f30e6bbce95983a070d148f40db3e91e4c005fddbbf166739ebe8a6c51`.
- Source state: branch `maintenance/rr-002-cloud-failure-lora`, base `7b316b8`, with candidate source and documentation changes left uncommitted for Chip. No candidate implementation commit exists yet.

Only Chip may commit the candidate and flash it. The handoff target for this bench test is the P2 that identifies itself as `0a10aced202194944a04084c` (`LoRA-Gateway-Dev-2`, Product 38211), subject to Chip's final physical-device confirmation before flashing.

### Candidate 24.5 bench result

Chip flashed candidate 24.5 to the confirmed P2 bench device and performed the scoped RR-002 test on 2026-07-19:

- startup logged `Firmware point release: 24.5`;
- the application entered `CONNECTING_STATE` with connectivity mode 1 while internet access was unavailable;
- a node report during `CONNECTING_STATE` failed, as expected and recorded under RR-003;
- at tick `0000603774`, the application logged the ten-minute timeout with point release 24.5 and connectivity mode 1;
- it selected `Failed to connect in 10 minutes - returning to LoRA and starting cloud retry cooldown` and transitioned directly `From Connecting to LoRA`;
- no `DISCONNECTING_STATE`, `SLEEPING_STATE`, LoRa radio shutdown, or `System.sleep()` followed the timeout;
- a node data report was received and acknowledged while internet remained unavailable; and
- after Chip restored internet, Particle Cloud reconnected automatically without a gateway reset and node data reports continued to be received.

All bounded RR-002 bench criteria passed. RR-002 is **Bench Validated**. The specified 48-hour soak remains outstanding, so the item is not Soak Validated or Closed.

## RR-003 — Mode-1 cloud connection attempts suspend LoRa reception

- **Status:** Evidence Collected (proposal ready for Chip's decision)
- **Date opened:** 2026-07-19
- **Owner:** Architect; investigation evidence collected under RR-002

### Observed symptom

During the 2026-07-19 internet-outage bench test, the application transitioned from `LoRA_STATE` to `CONNECTING_STATE` at approximately 17:01. Node reports at approximately 17:02 and 17:06 failed because `CONNECTING_STATE` does not call `listenForLoRAMessageGateway()`. LoRa reception resumes only after the application leaves that state.

### Reliability hypothesis

If mode-1 Particle connection and recovery work is serviced asynchronously while the application remains in `LoRA_STATE`, then Device OS can manage cloud retry in the system thread while the gateway continues receiving and acknowledging nodes.

### MAFC and Particle evidence

- The 2026-07-19 bench test proved that node reports fail while the application remains in `CONNECTING_STATE` and resume after it returns to `LoRA_STATE`.
- The earlier outage test proved that `listenForLoRAMessageGateway()` can receive and acknowledge nodes while the Particle system thread repeatedly attempts cloud recovery, provided the application remains in `LoRA_STATE`.
- MAFC enables `SYSTEM_THREAD(ENABLED)` and `SYSTEM_MODE(SEMI_AUTOMATIC)`. Particle documentation states that application `loop()` continues independently of network/cloud connection and that Device OS handles automatic reconnection after `Particle.connect()`.
- Occupancy reports carry absolute current net and gross values. Receiving intermediate reports while offline keeps the gateway's local per-node and per-space state current, but current application publish guards do not guarantee immediate cloud replay of those reports after recovery.
- Data acknowledgements also return time, reporting frequency, pending alert/configuration information, sensor type, and retry information. Continuous listening therefore preserves node protocol service and avoids systematic missed acknowledgements even when cloud history is unavailable.
- The hourly application connection transition overlaps the nodes' hourly reporting cadence, making the present receive gap operationally significant for up to 50 relatively dense nodes.

### Applicability and design decision

A second application thread is not proposed. Particle already provides the necessary concurrency through its system thread. Adding a dedicated LoRa thread would introduce synchronization and ownership risks around the SPI radio, RadioHead manager, `current`, `sysStatus`, `nodeDatabase`, acknowledgements, and persistent writes.

The smallest applicable design is to make cloud connection status cooperative background maintenance within normal mode-1 LoRa operation. Mode 1 remains in `LoRA_STATE`; `Particle.connect()` and automatic reconnection run on the Particle system thread; the application polls bounded cloud-maintenance flags without suspending `listenForLoRAMessageGateway()`.

### Proposed candidate 24.6 scope

1. Set `currentPointRelease` to `24.6` and retain startup point-release logging.
2. In mode 1, initiate `Particle.connect()` after Particle functions/variables are registered, but enter `LoRA_STATE` rather than `CONNECTING_STATE`.
3. Remove mode-1 hourly, cooldown-expiry, and background-recovery transitions from `LoRA_STATE` into `CONNECTING_STATE`. Preserve legacy `CONNECTING_STATE` behavior for non-mode-1 operation.
4. Add bounded mode-1 cloud-maintenance state that observes connection loss/recovery and schedules hourly connected maintenance without owning the main application state.
5. Preserve gateway webhook/reporting, daily message/reset counter reset, successful `lastConnection`, connection-duration semantics, time synchronization, signal logging, and retry/cooldown observability without blocking LoRa reception.
6. Replace `waitUntil(Particle.syncTimeDone)` for mode 1 with time-sync initiation and later completion polling from normal loop processing.
7. Ensure a received LoRa packet and its `REPORTING_STATE` handling retain priority over cloud maintenance.
8. Add defensive mode-1 guards so unexpected entry into `CONNECTING_STATE`, `DISCONNECTING_STATE`, or `SLEEPING_STATE` returns to `LoRA_STATE` before Particle disconnect, LoRa radio shutdown, or `System.sleep()`.
9. Log point release, connectivity mode, background connection loss/recovery, time-sync start/completion, hourly maintenance, and any defensive-guard activation with concise application logs.
10. Remove the RR-002 cooldown variables and branches only where the new mode-1 background-maintenance design makes them unreachable or obsolete. Preserve the bench-validated non-mode-1 timeout behavior.

Expected implementation files:

- `src/LoRA_Particle_Gateway.cpp`
- this RR record for implementation and validation results

Stop and return for approval if another firmware/library file is required or if protocol/persistence behavior must change.

### Explicit non-goals

- Do not add a second application thread.
- Do not change Node firmware, LoRa payloads, acknowledgement formats, timing/configuration fields, tokens, or alert codes.
- Do not redesign non-mode-1 power-managed behavior.
- Do not change libraries, persistent-data layout, Particle event names, integrations, or Reference Gateway files.
- Do not enqueue or replay every offline occupancy event.
- Do not add the proposed post-recovery latest-state snapshot in this item. That is a separate product/reliability decision after continuous radio service is validated.
- Do not reduce Device OS retry noise as part of this functional change.

### Risks

- Moving connected maintenance could omit or duplicate hourly gateway webhook, daily reset, connection metrics, signal, or time-sync work.
- Any synchronous cloud API call or blocking wait placed in the mode-1 path could recreate the receive outage.
- Cloud-maintenance flags could retrigger continuously unless rising-edge and once-per-hour behavior are explicit.
- A received packet could be overwritten by cloud state changes unless reporting priority is retained.
- Defensive guards could hide an upstream state error; each activation must be logged for diagnosis.
- Continuous acknowledgements during cloud loss can keep local state current without making that state visible remotely; cloud convergence remains dependent on a later node report until a separately approved snapshot mechanism exists.

### Protocol and compatibility impact

None expected. Node firmware and the gateway-to-node protocol remain unchanged. The intended change is increased availability of the existing receive, acknowledgement, configuration, and local-state paths while Particle Cloud is connecting or unavailable.

### Bench validation

1. Confirm startup logs point release `24.6`, mode 1, and entry into `LoRA_STATE` without an application transition into `CONNECTING_STATE`.
2. Boot with internet unavailable. Send multiple node reports throughout the first ten minutes and confirm every representative report is received and acknowledged.
3. Keep internet unavailable beyond an hour boundary. Send reports before, during, and after the former hourly ten-minute receive gap.
4. Confirm Device OS continues background cloud attempts while the application never enters `CONNECTING_STATE`, `DISCONNECTING_STATE`, or `SLEEPING_STATE` for mode-1 cloud work.
5. Restore internet. Confirm recovery is detected once; time sync starts and completes without blocking node reports; gateway webhook, connection metrics, signal logging, and queue service occur once as designed.
6. Send node reports during recovery and time synchronization and confirm receipt, acknowledgement, local occupancy updates, and reporting priority.
7. Leave the gateway connected through another hour boundary and confirm hourly maintenance runs once without interrupting LoRa.
8. Exercise or instrument each defensive guard and confirm it returns mode 1 to LoRa before any disconnect, radio shutdown, or sleep side effect.
9. Confirm closing-hours and cleaning-period resets retain existing behavior.

Pass criteria are continuous representative LoRa acknowledgements throughout boot, outage, hourly boundary, recovery, time sync, and normal hourly maintenance; no cloud-driven mode-1 disconnect/sleep; preserved connected housekeeping; and no reset, watchdog, persistent-data, protocol, or occupancy regression.

### Soak validation

Run at least 48 hours with representative MAFC node traffic, including a cloud outage longer than one hour, an hour boundary while offline, restoration during node traffic, and normal connected hourly maintenance. Monitor acknowledgement coverage, state and guard logs, connection recovery, time-sync completion, resets, free memory, queue depth, and node/space state.

### Rollback

Chip re-flashes the bench-validated point release 24.5 binary and records the reason. No FRAM migration or cleanup is expected. Keep the RR-003 evidence and failed-candidate identity if rollback occurs.

### Decision

Evidence and a bounded proposal are ready for Chip's approval. RR-003 is not yet approved for implementation. If approved, the Implementer prepares candidate 24.6, leaves it uncommitted, and does not flash it.

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

## RR-002 — Cloud connection failure must not disable continuous LoRa listening

- **Status:** Implemented
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

The candidate is cleared for bench flashing but is not bench validated. The full outage, LoRa acknowledgement, recovery, reboot-during-cooldown, and soak procedures remain outstanding.

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

On 2026-07-19, Chip authorized flashing the reviewed candidate. Particle Cloud accepted the OTA flash to `Morrisville-MAFC` (`e00fce68772083a2d0b7df4e`, Product 20952), and the device subsequently reported online. This confirms delivery and reboot recovery only; it does not satisfy the RR-002 cloud-outage bench validation or change the item status from **Implemented**.

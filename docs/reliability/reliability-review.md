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

# MAFC Gateway Reliability Engineering Workflow

## Purpose

This document governs the MAFC Gateway reliability effort. The objective is to improve MAFC Gateway reliability through evidence-based investigation and selective transplantation of proven reliability behaviors from the Reference Gateway, using the smallest safe changes.

The Reference Gateway is a source of proven ideas, not code. Its behavior may inform an MAFC-specific change only after evidence, applicability, risk, and validation have been documented. This workflow does not authorize a repository merge, history merge, architectural replacement, or broad refactor.

## Repository roles

| Role | Repository | Local path | Baseline | Mutability |
|---|---|---|---|---|
| Writable implementation target | **MAFC Gateway** (`LoRA-Particle-Gateway`) | `/Users/chipmc/Documents/Maker/Particle/Projects/LoRA-Particle-Gateway` | branch `master`, commit `87e72a0` | Documentation may be changed during the initial investigation; firmware only after approval |
| Read-only source of reliability ideas | **Reference Gateway** (`LoRA-Particle-Gateway-Legacy`) | `/Users/chipmc/Documents/Maker/Particle/Projects/LoRA-Particle-Gateway-Legacy` | production tag `v27.0.0-production`, commit `2b90ac8` | Read-only |
| Out of scope | Node firmware | Not part of either gateway repository role | Existing deployed firmware | Must not change |

The repositories have diverged substantially. A difference between them is not, by itself, evidence of a defect or a reason to backport code.

## Project constraints

The following are governing requirements for this effort:

- The MAFC Gateway is installed indoors.
- It is wall-powered, remains awake, and does not sleep.
- It supports up to 50 nearby, relatively dense nodes.
- Nodes report hourly and on every occupancy change.
- Occupancy represents the number of people in a room.
- The MAFC Gateway performs extensive per-node configuration, including space number, scanning pattern, and in/out orientation.
- The system resets during closing hours.
- The system also resets during a midday cleaning period.
- Existing Node firmware must remain unchanged.
- The existing gateway-to-node protocol must remain compatible.
- MAFC-specific application behavior must be preserved.
- The previously known FRAM erase hard fault has already been fixed in the MAFC Gateway. It is not a new implementation target; its history may be documented for accuracy.

## Guiding principles

1. Reliability over elegance.
2. Evidence before implementation.
3. Maintain a minimal-change bias.
4. Preserve MAFC behavior.
5. Preserve Node firmware and gateway-to-node protocol compatibility.
6. Differences between repositories are not automatically defects.
7. The Reference Gateway is a source of ideas, not code.
8. Use one approved reliability change per focused commit where practical.
9. Instrument before restructuring when evidence is insufficient.
10. Do not perform speculative cleanup during reliability work.

## AI and human roles

### Architect / Technical Lead

**Assigned to:** ChatGPT using Auto model selection

Responsibilities:

- Define the investigation strategy.
- Perform architectural reasoning.
- Prioritize candidate reliability work.
- Assess risk.
- Accept or reject findings.
- Independently review implementations.

### Investigator

**Assigned to:** GitHub Copilot Chat using GPT-5.5

Responsibilities:

- Explore repositories.
- Trace call chains.
- Trace ownership and lifecycle.
- Compare repository behavior semantically.
- Investigate allocation and persistence paths.
- Collect evidence.

The Investigator produces findings and evidence, not production code, unless explicitly reassigned.

### Implementer

**Assigned to:** Codex using GPT-5.6 Sol with High reasoning

Responsibilities:

- Implement only approved, bounded tasks.
- Follow this governing documentation and the approved reliability item.
- Avoid unrelated changes.
- Run the specified validation.
- Report deviations, uncertainty, and incomplete validation.

### Human approver

**Assigned to:** Chip

Responsibilities:

- Give final scope approval.
- Perform or approve bench validation.
- Flash hardware.
- Apply field judgment.
- Approve merges and releases.

No AI role is the final approver for scope, production readiness, merge, or release.

## Engineering lifecycle

Each candidate proceeds through this lifecycle:

```text
Observe
  -> Investigate
  -> Document
  -> Form hypothesis
  -> Approve
  -> Implement
  -> Review
  -> Bench test
  -> Soak test
  -> Merge
  -> Release
```

Before implementation begins, the reliability item must contain documented evidence, a reliability hypothesis, bounded scope, risk assessment, and validation method. Approval must be explicit. A finding may instead be rejected or returned for more investigation.

The implementation must receive independent review before bench and soak results are used to support merge or release. Bench success does not substitute for soak validation when soak validation is specified.

## Reliability item identifiers and statuses

Use sequential identifiers of the form `RR-001`, `RR-002`, and `RR-003`. Never reuse an identifier, including for a rejected item.

Allowed statuses are:

- **Open** — recorded but investigation has not begun.
- **Investigating** — evidence collection is active.
- **Evidence Collected** — evidence and applicability are ready for a decision.
- **Rejected** — not approved, with the reason recorded.
- **Approved** — scope and validation are approved for implementation.
- **Implemented** — implementation exists but required validation is incomplete.
- **Bench Validated** — the approved bench validation passed.
- **Soak Validated** — the approved soak validation passed.
- **Closed** — the final decision and outcome are recorded; no further work is planned.

Status changes must be dated in the item or its decision/results fields. Rejection is a valid outcome and must not be treated as implementation backlog.

## Implementation gates

Every item proposed for approval must state all of the following:

- Observed or credible failure mode.
- Evidence from the MAFC Gateway or its operating environment.
- Why the Reference Gateway behavior is relevant.
- Why the behavior applies to the MAFC architecture and constraints.
- The smallest proposed MAFC-specific change.
- Files expected to change.
- Explicit non-goals.
- Gateway protocol and Node firmware compatibility impact.
- Bench validation procedure and pass criteria.
- Soak validation procedure, duration, and pass criteria.
- Rollback method.

An item does not pass the gate if a required field is unknown. Record the uncertainty and continue investigation or instrumentation instead.

During implementation, stop and return the item for approval if evidence changes, the expected file set expands materially, protocol behavior may change, or the minimal change is no longer sufficient. Validation failures must be recorded rather than worked around with unapproved scope.

## Git discipline and traceability

- The initial investigation remains documentation-only.
- Create a maintenance branch only when the first implementation item is approved. Name the branch when implementation begins; no branch name is prescribed here.
- Do not commit changes to the Reference Gateway.
- Do not combine unrelated reliability items in one commit.
- Use one approved reliability change per focused commit where practical.
- Do not mix speculative cleanup, formatting, dependency updates, or unrelated refactoring into reliability commits.
- Preserve traceability from each RR item to its investigation prompt, evidence, approval decision, implementation commit, validation results, and final decision.
- Record deviations from the approved scope before requesting review.
- Chip retains merge and release authority.

The living record for findings, decisions, and results is [`reliability/reliability-review.md`](reliability/reliability-review.md).

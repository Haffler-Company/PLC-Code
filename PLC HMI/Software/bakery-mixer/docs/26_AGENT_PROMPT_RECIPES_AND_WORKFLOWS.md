# 26. Autonomous Agent Prompt Recipes & Factory Workflows

## Executive Summary
In fast-paced factory development environments, machine owners and senior engineers lack the time to type verbose prompts or reiterate engineering constraints. This cookbook provides **copy-paste golden prompts** that immediately guide any autonomous AI agent (Gemini, Claude, Antigravity, Cursor) to execute tasks with maximum precision and zero conversational friction.

---

## 1. The 6 Factory Golden Prompts

### Prompt 1: Bootstrap a New Machine Project in One Line
> **Prompt Text:**  
> "Bootstrap a new project based on the Haffler Desktop Agent Kit and Document 20.  
> **Machine Name:** [e.g. UPVC Double-Head Miter Saw]  
> **Workshop Scenario:** [e.g. Operator taps foot pedal, profile clamps advance, two saw motors start, heads descend at 45 degrees, cut profile, retract, motors stop, unclamp].  
> Draft the technical specification in `docs/MACHINE_SPEC.md` and wait for my confirmation."

### Prompt 2: Generate Full Apple HIG UI
> **Prompt Text:**  
> "Implement the 3-tier user interface following the Component Cookbook (Doc 21) and Screen Flow (Doc 12):  
> 1. Startup Splash with Haffler logo, version string, and 1.5s self-test  
> 2. Main Dashboard with mathematically centered Hero Metric  
> 3. Settings Cards (F1..F5) with 16px breathing margin parameter steppers  
> Enforce matte dark slate theme, 60 FPS, and zero software shadows."

### Prompt 3: Implement 100% Touchless Front-Panel Navigation
> **Prompt Text:**  
> "Implement touchless panel navigation. Wire up the 10 tactile keys and the `MODE` button strictly following the 3-state FSM in Doc 03 so the operator can adjust parameters without touching the screen."

### Prompt 4: Root-Cause Debugging Without Regression
> **Prompt Text:**  
> "[e.g. Profile clamp cylinder is timing out and throwing Alarm E02].  
> Investigate root cause in the reed switch debouncing and timeout logic (Doc 16). Forward-fix only; do NOT break previously verified features."

### Prompt 5: Generate Cloud OTA Release Package
> **Prompt Text:**  
> "Give server release files."  
> *(Agent increments version, builds `firmware.bin`, runs `gen_lastfile.py`, places strictly 2 files inside release/ (Zero-Desktop), and verifies ArvanCloud S3 health).*

### Prompt 6: Structured Product Brainstorming
> **Prompt Text:**  
> "Propose feature enhancements for this machine based on the 4-pillar framework in Doc 12."  
> *(Agent returns 4 concrete proposals across Ergonomics, OEE Efficiency, Predictive Maintenance, and Connected Smart Factory).*

---

## 2. Mandatory Agent Response Protocol (Ultra-Brevity)

When receiving these prompts, the agent must reply strictly with:
1. Completion Progress Percentage (%)
2. Clickable Links to modified files (`file:///...`)
3. A single decisive question or a green PASS verification summary

*(Zero conversational pleasantries, zero flattery, zero restating the user prompt).*

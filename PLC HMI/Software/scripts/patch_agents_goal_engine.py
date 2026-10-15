#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Patch all AGENTS.md files across all machine projects in PLC HMI Software
to include the DOTS Autonomous Goal Engine directives (both English and Persian).
"""

import os
import sys

BASE_DIR = r"C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software"

PROJECTS = [
    "Aluminum-punching",
    "Bakery",
    "bakery-mixer",
    "config-test",
    "Copy_router",
    "Corner_cleaning",
    "Cutter double_head(Digital_metr)",
    "Cutter_single_head(Digital_metr)",
    "Gas-filling",
    "ICE CREEAM",
    "Welding_double_head",
    "Welding_single_head",
    "Haffler_PLC_Universal_Agent_Kit_EN"
]

GOAL_EN_SECTION = """### 🎯 G. AUTONOMOUS GOAL ENGINE (DOTS Paradigm)
- Zero Micromanagement: Never halt mid-task to ask for piecemeal instructions ("کار تیکه تیکه").
- "State Your Goal": On receiving the high-level objective, extract the Definition of Done (DoD), formulate execution graph, and autonomously drive code editing, debugging, compilation, and physical simulation until 100% verified.
- Active Skill: `.agents/skills/haffler-autonomous-goal-engine/SKILL.md`.
"""

GOAL_FA_SECTION = """
---

## 🎯 پروتکل اقدام خودمختار تا تحقق هدف نهایی (Haffler Autonomous Goal Engine - DOTS Paradigm)
- **منع ریزمدیریت و کار قاشق‌به‌قاشق:** ایجنت‌ها نباید در میانه کار متوقف شده یا کار را تکه‌تکه مطالبه کنند.
- **«هدفت رو بگو»:** با دریافت هدف نهایی کارفرما، ایجنت قرارداد خروجی (Definition of Done) را استخراج کرده و کل زنجیره طراحی، کدنویسی، رفع باگ، لینتر و تست را به صورت پیوسته، خودترمیم‌کننده و بدون وقفه تا تحویل ۱۰۰٪ طی می‌کند.
- **مهارت فعال:** `.agents/skills/haffler-autonomous-goal-engine/SKILL.md`.
"""

def patch_file(file_path):
    if not os.path.exists(file_path):
        print(f"[SKIP] Not found: {file_path}")
        return

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    modified = False

    if "AUTONOMOUS GOAL ENGINE" not in content and "haffler-autonomous-goal-engine" not in content:
        # Check for section F
        marker = '### ⚡ F. VERIFICATION GATE (before saying "done")'
        if marker in content:
            idx = content.find(marker)
            # Find the end of item 3 under F
            end_f_item = content.find("3. No warnings promoted silently — fix or explicitly explain why safe.", idx)
            if end_f_item != -1:
                insert_pos = end_f_item + len("3. No warnings promoted silently — fix or explicitly explain why safe.")
                content = content[:insert_pos] + "\n\n" + GOAL_EN_SECTION + content[insert_pos:]
                modified = True
            else:
                content = content + "\n\n" + GOAL_EN_SECTION
                modified = True
        else:
            content = content + "\n\n" + GOAL_EN_SECTION
            modified = True

    if "پروتکل اقدام خودمختار" not in content:
        # Add Persian section before graphify or at end
        graphify_marker = "## ⚡ پروتکل ارتقای راندمان"
        if graphify_marker in content:
            idx = content.find(graphify_marker)
            content = content[:idx] + GOAL_FA_SECTION.strip() + "\n\n---\n\n" + content[idx:]
            modified = True
        else:
            content = content + "\n\n" + GOAL_FA_SECTION
            modified = True

    if modified:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"[UPDATED] {file_path}")
    else:
        print(f"[OK] Already up to date: {file_path}")

def main():
    # Patch Software root AGENTS.md
    root_agents = os.path.join(BASE_DIR, "AGENTS.md")
    patch_file(root_agents)

    # Patch all project AGENTS.md
    for p in PROJECTS:
        p_agents = os.path.join(BASE_DIR, p, "AGENTS.md")
        patch_file(p_agents)

if __name__ == "__main__":
    main()

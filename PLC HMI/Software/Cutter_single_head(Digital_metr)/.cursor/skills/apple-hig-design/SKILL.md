---
name: apple-hig-design
description: >-
  Apple Human Interface Guidelines craft for exceptional UI/UX: clarity,
  deference, depth, typography, color, layout, SF Symbols thinking, motion,
  accessibility, and anti-AI-slop checks. Use when designing, polishing, or
  reviewing any UI, HMI, simulator HTML, or when user mentions Apple design,
  HIG, زیبا، طراحی بی‌نظیر, or visual quality.
---

# Apple HIG Design Craft

For Metr & Cutter industrial mapping also read:
`docs/OPERATIONS.md` and `docs/REFERENCE.md`.

## Mindset (non-negotiable)
1. **Clarity** — content legible at a glance; one primary action per region.
2. **Deference** — chrome serves content; never compete with the hero number/list.
3. **Depth** — soft hierarchy via layers, blur-like separation, subtle shadow — not decoration.
4. **Consistency** — same control = same look/behavior everywhere.
5. **Direct manipulation** — touch targets feel hit; feedback is immediate and honest.
6. **User control** — cancel paths obvious; destructive actions confirmed.
7. **Feedback** — every tap acknowledges (state, motion, or sound policy).

## Layout
- Prefer **8pt grid** (4pt for fine optical alignment only).
- Margins ≥ 8; comfortable 12–16 on cards.
- Align columns to invisible grid; avoid “almost aligned”.
- **Visual weight**: hero > title > secondary > chrome.
- One composition per viewport — not a dashboard of equal cards.
- Group related controls; separate unrelated with space (not extra boxes).

## Touch & controls
- Minimum hit area **44×44 pt** spirit → on 480×320 HMI use **≥40×40 px**.
- Primary button visually strongest; secondary quieter; tertiary text/icon only.
- Destructive = red system role; never style cancel as primary blue.
- Pressed state: slight darken / 1–2px translate — no cartoon bounce.

## Color (system roles)
| Role | Light-ish hex | Use |
|------|---------------|-----|
| Blue / tint | `#007AFF` / `#0A84FF` | Interactive primary |
| Green | `#34C759` | Success / confirm |
| Orange | `#FF9500` | Warning / non-fatal |
 | Red | `#FF3B30` | Destructive / stop |
| Label | `#1C1C1E` | Primary text |
| Secondary label | `#8E8E93` | Units, hints |
| Tertiary | `#C7C7CC` | Disabled hints |
| Fill / card | `#FFFFFF` | Surfaces |
| Grouped bg | `#F2F2F7` | Page behind cards |
| Separator | `#C6C6C8` | 1px rules |

Rules: one tint per screen; never rainbow headers; text contrast must stay readable on grey industrial panels.

## Typography
- Clear hierarchy: Large Title / Title / Headline / Body / Caption / Footnote spirit.
- Numbers for machines may use digit fonts; UI chrome stays calm sans.
- Avoid more than **3 size steps** visible in one region.
- Secondary labels lighter/smaller — never same weight as hero.
- Persian: respect RTL labels; keep Latin units (`mm`) optically balanced.

## Icons
- Think **SF Symbols**: simple, monochrome or dual-tone, optically aligned.
- Icon + short label when space allows; icon-only only if meaning is universal.
- Same metaphor everywhere (trash = delete, house = home).
- No emoji as UI icons on industrial HMI.

## Shape & materials
- Continuous corner feel: radius 8–12 small, 14–16 large cards; pill only for true chips.
- Separators 1px — prefer space over heavy borders.
- Shadows: soft, single layer, low opacity; offset mostly Y.
- Avoid: neon glow, multi-colored gradients, glassmorphism soup, purple AI defaults.

## Motion (when LVGL allows)
- Short (≤200–300ms feel), ease-out, purposeful.
- Motion explains hierarchy (sheet in, dim behind) — not entertainment.
- Prefer no fade screen transitions on this device (bleed history).

## Accessibility / operator reality
- Large gloves / factory lighting → bigger targets, higher contrast.
- Don’t rely on color alone for state (pair with icon/text).
- Critical safety (E-STOP) must be unmistakable vs normal chrome.

## Anti-slop checklist (fail = redesign)
- [ ] Purple-on-white / generic AI gradient theme?
- [ ] Too many cards/pills/badges?
- [ ] Hero buried under chrome?
- [ ] Uneven gaps, overlapping hit targets?
- [ ] Shadows harsh or colored oddly?
- [ ] More than one loud primary CTA?
- [ ] “Cute” decoration instead of operator clarity?

## Review output template
```markdown
## Apple craft review
- Clarity:
- Deference (content vs chrome):
- Depth / materials:
- Type / color / icon:
- Touch / consistency:
- Anti-slop fails:
- Fix order (max 3):
```

## Detail reference
Read `docs/REFERENCE.md` for the project-specific HMI constraints.

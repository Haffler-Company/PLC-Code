# این استاندارد برای همه‌ی ریلیزهای HAFFLER الزامی است — ابزار تولید: tools/gen_lastfile.py

LASTFILE STANDARD — PROMPT FOR ANY AGENT
(بگذارید این متن را کامل به ایجنت بدهید — هم برای ساخت فایل، هم برای خواندن/نمایش آن)

You are working on the Haffler "Digital Cutter Single-Head HMI" firmware project.
Follow this standard whenever you BUILD A RELEASE or DISPLAY RELEASE INFO.

--------------------------------------------------------------------
1. PURPOSE
--------------------------------------------------------------------
Every firmware release is described by exactly ONE descriptor file named
`lastfile` (no extension), placed next to the firmware binary.
It is the single machine-readable source of release information.
The F5/update page and any tool must read ONLY this file to show release info.

--------------------------------------------------------------------
2. FILE FORMAT (fixed grammar — never change key names)
--------------------------------------------------------------------
- UTF-8, English only, plain text, LF or CRLF.
- Lines starting with `#` are comments. Blank lines allowed.
- Sections in EXACTLY this order:
  [program] [version] [binary] [description] [inputs] [outputs] [install]
- Inside a section: `key=value`, one per line, no duplicates.

Mandatory keys:
[program]   name, project, platform, display, language
[version]   code, display, build_date (YYYY-MM-DD), build_time (HH:MM)
[binary]    file, size_bytes, sha256 (uppercase), signed (yes/no),
            partition, flash_compatible
[description] line1..lineN (one short change note per line, max 8 lines)
[inputs]    in1..inN  (order = physical/priority order; GPIO number first)
[outputs]   out1..outN (order = Q0,Q1,... ; GPIO number first; include a
            final `note=` line for polarity)
[install]   method, command, status

Optional key (append only at the very end of the file):
[extra]     k=v pairs for anything the owner did not list but is needed
            (e.g. rollback, known issues). Never invent keys inside other
            sections.

--------------------------------------------------------------------
3. GENERATION RULES (when building a release)
--------------------------------------------------------------------
- version.code/display <- firmware/include/version.h
- build_date/build_time <- mtime of the built firmware.bin
- file/size_bytes/sha256 <- the produced binary (hash AFTER copy check)
- description lines <- fw_changelog.h CUR block, translated to short
  English sentences (one line each)
- inputs/outputs <- st_IO_PARAM in firmware/src/main.ino (Q map) and
  docs/REFERENCE.md hardware section (inputs). Verify GPIO numbers in
  source before writing; never copy from memory.
- Regenerate `lastfile` on EVERY release; overwrite fully; never append
  history (history lives in fw_changelog.h, not here).
- Keep the whole file under 70 lines.

--------------------------------------------------------------------
4. PARSING / DISPLAY RULES (update page, tools, agents)
--------------------------------------------------------------------
- Parse top-down; unknown sections/keys must be ignored, not fatal.
- Render, in this order: program name; version display + build date/time;
  description lines; then two tables (inputs, outputs) with GPIO column;
  then install/status.
- If sha256 of the companion binary verifies, show a "verified" marker;
  if it fails or the binary is missing, show "unverified" and hide
  install/status.
- Never display raw comments or section headers to the operator.

--------------------------------------------------------------------
5. FORBIDDEN
--------------------------------------------------------------------
- No extra files (no summaries, no per-session notes, no side reports).
- No key renames, no new sections except [extra], no non-English values.
- Never store secrets, PINs, or passwords in lastfile.

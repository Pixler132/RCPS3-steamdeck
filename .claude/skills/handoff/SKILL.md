---
name: handoff
description: Write the end-of-session handoff. Use whenever the user asks for a handoff, a session summary to carry forward, or says the session is ending. Enforces the ONE-FENCED-BLOCK format, which overrides the VSCode markdown-link instruction.
---

# Handoff format

Deliver the ENTIRE session handoff inside ONE fenced code block (```), plain text,
start to finish. **Write the opening fence FIRST, then fill it.**

- No `##` headers, no `**bold**`, no `[text](link)` file references.
- No lead-in paragraph before the block, no commentary after it.
- Never write a handoff to a file. It stays in chat so the owner reads and edits it
  before pasting it into the next session; a stored file gets auto-loaded and anchors
  the next session to the previous agent's approach instead of the owner's direction.

**This OVERRIDES the VSCode "Code References in Text" instruction** (the one that
says to always use markdown link syntax for file paths and never use backticks).
That instruction does not apply inside a handoff. Inside a handoff, plain
`path/to/file.py:42` is correct and a clickable link is a BUG.

## Why this rule needed its own channel

The competing "use markdown links" instruction lives in the system prompt (present
every turn, unbidden). A rule kept in a memory file or CLAUDE.md has to be actively
recalled, and an always-present instruction beats a must-be-recalled one. A skill
fires ON THE TRIGGER (the user asks for a handoff) and arrives as instruction at
exactly the moment the competing instruction would otherwise win. It is also cheaper
than CLAUDE.md, which pays the token cost every session for a rule that matters once,
at the very end.

**If it fails anyway**, don't simply restate the rule. Change the ARTIFACT instead:
draft the handoff into the session scratchpad and paste the file's contents, so the
fence becomes a mechanical step rather than a formatting choice.

## What a handoff contains

State, then what landed, then what is OWED, then the next moves in the order you
would take them, then what is still open and carried. Name the standing traps that
will bite the next session (staleness, cache, allowlists). Be honest about what is
unverified: say so plainly rather than implying it was checked.

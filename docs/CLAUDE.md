# docs/ — rules for writing documentation

Applies to every file under `docs/`. 

## Content
- Document verified facts only. Every documented statement must be supported by confirmed evidence from the source code, configuration, tests, existing documentation, or explicit decisions.
- Do not document assumptions, speculation, or unverified information as facts.
- Write in English. Keep it short: a reader should understand the design and **why** it is that way, not
  re-read the code.

## Folder tree
- Folder/file trees in documents use the symbols `├──`, `└──`, `│`.

## Diagrams
- Every diagram is **Mermaid** code in a ```` ```mermaid ```` block. No images of diagrams.
- Every sequence diagram uses `autonumber`, and is followed by a table whose `#` column matches the numbers
  and explains each step. Component diagrams with numbered edges get the same kind of table.
- **Split large diagrams** into several small ones so each can be read at a glance: split by phase
  (start / request / stop), by side (frontend / backend) or by layer. As a guide, aim for at most about
  10 numbered steps and 6 participants per sequence diagram.

## Checks before reporting
- Relative links and anchors resolve.
- Every Mermaid block renders (Mermaid 12.x for `usecase-beta`).

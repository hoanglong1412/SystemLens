# docs/ — rules for writing documentation

Applies to every file under `docs/`. Project-wide rules (file format UTF-8 without BOM + CRLF, HISTORY.md,
"spec first") are in the root `CLAUDE.md`.

## Content
- Describe only what the **current source code** does. Do not add features, requirements or suggestions.
- Requirements (`requirements/`) are written by the project owner. Never add or change a requirement on your
  own; where information is missing, leave a placeholder and ask.
- Write in English. Keep it short: a reader should understand the design and **why** it is that way, not
  re-read the code.

## Structure
```
docs/
├── README.md                            index, ID scheme, requirement files, traceability matrix
├── CLAUDE.md                            this file
├── api-schema.md                        api/openapi.yaml as tables (the YAML wins on conflict)
├── api-convention.md                    rules every API follows
├── requirements/
│   └── RQ<NNN>_<Name>.md                one file per work item: FR, UC, NFR
├── design/
│   └── RQ<NNN>_<Name>/                  one folder per requirement file, same name
│       ├── RQ<NNN>_HLD.md               high-level design of the work item
│       └── RQ<NNN>_LLD-NN-<name>.md     low-level design, only as many as the scope needs
└── ui-design/                           Figma / UI design files
```
- Do not create files the scope does not need. Prefer one more section over one more file.
- Folder/file trees in documents use the symbols `├──`, `└──`, `│`.

## IDs and traceability
- IDs: `RQ<NNN>-FR-NN`, `RQ<NNN>-UC-NN`, `RQ<NNN>-NFR-NN`, `RQ<NNN>-HLD-NN`, `RQ<NNN>-LLD-NN`, API by `operationId`.
  A step of a numbered sequence is referenced as `<ID> §<section> #<step>`.
- Anchors are the lowercase ID, set with `<a id="..."></a>` before the heading.
- Every design document starts with a metadata table: **Traces to** (requirements it satisfies) and
  **Detailed in** (downstream documents). Requirement files do not link to design; the mapping lives in the
  design metadata and in the traceability matrix of `docs/README.md`.
- When something is added, renamed or removed, update `docs/README.md` (layout, requirement files, matrix).

## Diagrams
- Every diagram is **Mermaid** code in a ```` ```mermaid ```` block. No images of diagrams.
- Every sequence diagram uses `autonumber`, and is followed by a table whose `#` column matches the numbers
  and explains each step. Component diagrams with numbered edges get the same kind of table.
- **Split large diagrams** into several small ones so each can be read at a glance: split by phase
  (start / request / stop), by side (frontend / backend) or by layer. As a guide, aim for at most about
  10 numbered steps and 6 participants per sequence diagram.
- Do not draw the same flow twice in different files; draw it once at the right level and link to it.
- Use case diagrams use `usecase-beta` (needs Mermaid 12.0.0 or later).

## Design documents
- Lead with the design, then the reasons:
  - HLD: components, the design decisions with their reasons, how each requirement of the RQ is satisfied
    (or "no design yet" when it is not implemented), one overview flow.
  - LLD: the flows of one area, protocols / error handling as tables, notes on constraints and pitfalls.
- Reasons come from the code (comments, structure). If the code does not record a reason, say so instead of
  inventing one.
- Do not copy what the code already documents: no tables of every function signature, parameter or constant.
  Mention a function or constant only when it matters to the design.
- Flowcharts only for functions with real decision logic (several branches / loops), not for trivial ones.

## Checks before reporting
- Relative links and anchors resolve.
- Every Mermaid block renders (Mermaid 12.x for `usecase-beta`).
- `Tool\check_utf8.ps1` and `Tool\check_crlf.ps1` exit 0 on the changed files.

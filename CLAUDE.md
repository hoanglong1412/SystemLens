# SystemLens — guide for Claude

Windows task manager clone. Learning project: the owner wants to understand OS internals, practice C++,
and practice spec-driven design with a human in the loop.

## Rules
- **Spec first** Update document and review before coding
- **Every commit updates `HISTORY.md`** (in the same commit): add an entry at the top with the reason,
  what the commit does, and the relevant user ↔ AI exchanges (requests, decisions, issues found).
  Follow the template at the top of that file. Write it in Vietnamese.
- **File format: every text file is UTF-8 without BOM + CRLF**, in the working tree and in the repo
  (`.gitattributes` has `* -text`, so git stores bytes as-is).
  - Write/Edit: the project hook `.claude/hooks/fix-format.ps1` converts the file automatically.
  - Any other way of changing files (Bash, sed, scripts, `npm install`…): run
    `Tool\check_utf8.ps1 -Path <file> -Fix` and `Tool\check_crlf.ps1 -Path <file> -Fix`.
  - Before reporting results to the user, run both tools (no `-Fix`) and make sure they exit 0.
  - The git pre-commit hook shows a Yes/No dialog listing invalid staged files; it never auto-fixes.
- **No new tooling/installs without asking first** (compilers, extensions, global packages, vcpkg ports).
- Code quality is reviewed manually: no linters/formatters in the pipeline.

## Coding convention

1. **File header comment** on every file: file name + what the file contains / is responsible for.
2. **Function header comment** on every function: what it does, params, return value, errors thrown
   (C++: Doxygen at the declaration in the `.cpp`; JS: JSDoc; do not comment header function in the `.h`,
   except pure virtual interface functions, which have no `.cpp` and keep their comment in the `.h`).
3. **Comment non-obvious code**: explain *why* (reason, constraint, pitfall), not *what*.
4. **Clean, maintainable, extensible code**: single responsibility, depend on interfaces injected via
   constructor, respect the layering (HttpServer → Controller → Service → Provider), no magic numbers,
   explicit error handling, extend by adding files rather than modifying infrastructure.

# docs/ — rules for writing documentation

Applies to every file under `docs/`. 

## Content
- Write in a formal, objective, and technical style.
- Include only factual, verified, and relevant information.
- Present information directly, focusing on requirements, decisions, architecture, implementation, and technical constraints.
- Documentation must stand independently without relying on the conversation history for context.
- Do not include conversational content or references to interactions with the user or AI.
- Do not mention that something was discussed, requested, confirmed, agreed upon, or clarified with the user.
- Do not include personal opinions, subjective judgments, emotional language, or unnecessary commentary.
- Do not document assumptions, speculations, or unverified information as facts.
- Do not duplicate information unnecessarily. Use relative links to reference existing documentation.

## Folder tree
- Use the following symbols for folder and file trees:
  - `├──` for non-final entries.
  - `└──` for the final entry at each level.
  - `│` for vertical continuation lines.
- Maintain consistent indentation and tree structure.

## Diagrams
- Use Mermaid for all diagrams. Do not use diagram images or screenshots.
- Every sequence diagram uses `autonumber`, and is followed by a table whose `#` column matches the numbers
  and explains each step. Component diagrams with numbered edges get the same kind of table.
- Split large diagrams into smaller diagrams based on logical boundaries, such as:
  - Processing phases (start / request / stop).
  - System boundaries (frontend / backend).
  - Architectural layers.
- As a guideline, keep sequence diagrams to approximately 10 numbered steps and 6 participants or fewer.
- Each diagram must have a clear scope and title.

## Checks before reporting
- Relative links and anchors resolve.
- Every Mermaid block renders (Mermaid 12.x for `usecase-beta`).

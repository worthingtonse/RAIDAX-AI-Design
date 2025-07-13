### Project Awareness & Context
- **Use consistent naming conventions, file structure, and architecture patterns** as described in `PLANNING.md`
- **Follow the best practices in C progrmming language**
- **Always make security priority over performance**

### Code Structure & Modularity
- **Never create a file longer than 500 lines of code.** If a file approaches this limit, refactor by splitting it into modules or helper files.
- **Organize code into clearly separated modules**, grouped by feature or responsibility.
- **Put every module in a separate folder**
- **Always create Makefile in the root folder**
- **Use clear, consistent includes** (prefer relative imports within packages).

### Testing & Reliability
- **Always create unit tests in the tests folder** (functions, classes, routes, etc).
- **After updating any logic**, check whether existing unit tests need to be updated. If so, do it.
- **Tests should live in a `/tests` folder** mirroring the main app structure.
  - Include at least:
    - 1 test for expected use
    - 1 edge case
    - 1 failure case

### Style & Conventions
- **Use C** as the primary language.
- **Follow the One True Brace Style**.
- **Put comments before the declaration of a function**.
- **Declare variables in the beginning of a function**
- **Put global variables in the beginning of every file**
- **Function prototypes and types must used in a c-file must be defined in the .h file with the same name**
- **File names must be lowercase**


### Documentation & Explainability
- **Update `README.md`** when new features are added, dependencies change, or setup steps are modified.
- **Comment non-obvious code** and ensure everything is understandable to a mid-level developer.
- When writing complex logic, **add an inline `# Reason:` comment** explaining the why, not just the what.
- **Put comments in the begginning of the file** - to get an overview of its contents

### AI Behavior Rules
- **Never assume missing context. Ask questions if uncertain.**
- **Never hallucinate libraries or functions** – only use known, verified C function.
- **If a library is missing download it from a trustworthy repository**

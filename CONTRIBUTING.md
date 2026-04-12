# Contributing to Portable-LCE

Thank you for considering contributing to Portable-LCE! We appreciate all types of contributions, including bug fixes, new features, and documentation improvements.

Before you start contributing, please take a moment to review the guidelines outlined here. This ensures a smooth collaboration and helps maintain the project's quality.

## How to Contribute

There are several ways you can contribute:

- **Report Bugs:** If you find a bug, please open an issue to describe the problem.
- **Feature Requests:** If you have an idea for a new feature, feel free to suggest it by opening an issue.
- **Submit Code:** If you want to contribute code, fork the repository, create a new branch, and submit a pull request.

Make sure to follow the guidelines below when submitting code or issues.

## Submitting code

If you are submitting a pull request to this repository, here are some guidelines to keep in mind.

### Test your changes.

Please run the game and make sure your code runs as expected before marking a pull request ready for review.

### Keep scope to a minimum.

Pull requests should ideally do one thing in one place. Avoiding opening massive pull requests that change multiple components of the game. These are often not reviewable and result in unmanageable conflicts with other active PRs.

### Use common sense with commits.

Commit names should clearly describe what was changed in the commit. [Conventional commits](https://www.conventionalcommits.org/en/v1.0.0/) are generally appreciated, but by no means required. Similar to pull requests, commits should ideally be limited in scope and easy to track over time.

### Keep code clean and readable.

Code formatting is defined by the repository's [`.clang-format`](./.clang-format) file. If you are touching C or C++ source, format the files you changed before opening or updating a pull request.

CI checks formatting on changed C and C++ files, so local formatting mismatches will fail the relevant workflow.

### Avoid changing in-game behavior.

At this time, we are trying to stay as close to the vanilla game as close as possible. Anything that changes game behavior in breaking ways will likely be closed. This includes adding new features or backporting newer functionality from other (non-LCE) versions of the game.

> Some exceptions to this guideline apply. Features required for platform porting (such as adding new input methods) are considered in-scope. In some cases, temporary "stop-gap" implementations or additional functionality may be accepted on a case-by-case basis, provided it is clearly documented and possible to disable through a compiler flag.

### Bugfixes should be clearly commented.

If you are submitting a minor addition to *game-relevant code*, clearly comment the purpose of this in a comment starting with `// PLCE:`. This indicates a patch over the original game code and lets us keep track of what has been modified in case it needs to be reverted later.

> **Example:**
>
> ```cpp
> // PLCE: masking the id to 11 bits before writing to account for entity ids > 4095.
> // This fixes a connection drop when loading the tutorial world on linux.
> //
> // FIXME: find the root cause of this, since there shouldn't be more than 4095 entities.
> dos->writeShort((id & 0x07FF) | 0x800);
> ```

This rule may not apply to contributions fixing *trivially incorrect* 4J code that affects all platforms, such as instances of undefined behavior.

### Try to fix the cause, not the effect.

If you are fixing a bug, avoid submitting "hacks" that attempt to patch the effects of the bug rather than the root cause.

> Exceptions to this rule may apply depending on the severity of the bug, provided that the code is clearly commented as a hack with a relevant `// PLCE:` comment.

Everything previously written before the hardfork from 4jcraft is commented with a "4JCraft" comment prefix.

### Don't submit code written by AI.

Submitting code to this repository authored by generative AI tools (LLMs, agentic coding tools, etc...) is strictly forbidden. Pull requests that are clearly vibe-coded or written by an LLM will be closed.

> **Rationale:** Contributors are expected to both fully understand the code that they write **and** have the necessary skills to *maintain it*. Opening PRs containing code that you did not write yourself more often than not fails to meet either of these expectations, therefore it is disallowed.

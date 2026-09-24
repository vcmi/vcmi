# Contributing to VCMI

Looking to contribute something to VCMI? **Here's how you can help.**

Please review this document to make the contribution process easy and effective for everyone involved.

Following these guidelines shows respect for the time of the developers who maintain this open-source project.
In return, they will respect your time when they review your issue or pull request.

If you're planning something non-trivial (a new feature, a large refactor, a new mod format extension), please talk to us on [Discord](https://discord.gg/chBT42V) or open an issue first. It's much easier to agree on an approach before code is written than to rework a finished pull request.

## Using the issue tracker

* Please **do not** use the issue tracker for help playing or using VCMI, or for general Heroes III questions.
  Please try [Discord](https://discord.gg/chBT42V) or the [forums](https://forum.vcmi.eu/) instead.

* Please **do not** derail or troll issues. Keep the discussion on topic and respect the opinions of others.

* Please **do not** post comments consisting solely of "+1" or ":thumbsup:".
  Use [GitHub's "reactions" feature](https://github.com/blog/2119-add-reactions-to-pull-requests-issues-and-comments) instead.

* Please use [Weblate](https://hosted.weblate.org/engage/vcmi/) to submit corrections and improvements to translations, not issues or pull requests against locale files directly. See [Translations](docs/translators/Translations.md).

## Bug reports

The preferred channel for bug reports is the [issue tracker](https://github.com/vcmi/vcmi/issues). Consider the following when submitting a report:

A bug is a *demonstrable problem* caused by VCMI itself, not by a specific mod or by modified game assets.

Please **do not** open issues or pull requests about bugs in mods. These are maintained by their own authors, not by VCMI. First check whether the issue also reproduces without the mod (or with only base game content) before reporting it here.

Guidelines for bug reports:

1. **Use the GitHub issue search** — check if the issue has already been reported.
2. **Check if the issue has been fixed** — try to reproduce it using the latest development build.
3. **Isolate the problem** — ideally with as few mods enabled as possible, and with reproduction steps.

See [Bug Reporting Guidelines](docs/players/Bug_Reporting_Guidelines.md) for what information is most useful to us. A map or saved game with reproduction steps is far more valuable than a log file alone. Then fill out the [bug report template](.github/ISSUE_TEMPLATE/bug_report.md).

A good bug report should include enough information for others to investigate it. Please include:

* What is your environment (OS and VCMI version)?
* What steps will reproduce the issue?
* What did you expect to happen, and what happened instead?
* Logs, a saved game, or a map, if applicable.

## Translations

Please use [Weblate](https://hosted.weblate.org/engage/vcmi/) to submit corrections and improvements to translations of the game.

You can also help translate mods on a [separate Weblate instance](https://weblate.vcmi.eu/).

## Feature requests

The preferred channel for feature requests is the [issue tracker](https://github.com/vcmi/vcmi/issues). Use the [feature request template](.github/ISSUE_TEMPLATE/feature_request.md).

It is up to *you* to make a strong case for your idea. Provide as much detail and context as possible. Describe the problem you see and explain why it should be fixed instead of only proposing a solution.

For any non-trivial request, such as new mechanics, new mod format capabilities, or engine-level changes, consider discussing it with the team on [Discord](https://discord.gg/chBT42V) first. See the note at the top of this document. This avoids spending time on a design that the maintainers are unlikely to accept.

## Pull requests

Good pull requests, including patches, improvements, and new features, are very helpful.

Every pull request should have a clear scope, with no unrelated commits.

**Please ask first** before starting any significant pull request, such as a new feature, a large refactor, or a port to a new platform. Otherwise, you risk spending a lot of time on something the maintainers might not want to merge.

1. Set up your development environment using the build guide for your platform:
   [Windows](docs/developers/Building_Windows.md),
   [Linux](docs/developers/Building_Linux.md),
   [macOS](docs/developers/Building_macOS.md),
   [Android](docs/developers/Building_Android.md),
   [iOS](docs/developers/Building_iOS.md).

2. Fork the project and clone your fork:

   ```bash
   git clone --recursive https://github.com/<your-username>/vcmi.git
   cd vcmi
   git remote add upstream https://github.com/vcmi/vcmi.git
   ```

3. Branch off `develop` — that's where all active development happens:

   ```bash
   git fetch upstream
   git checkout upstream/develop -b <topic-branch-name>
   ```

4. Make your changes:
   * Use `.clang-format` for recommended formatting and `.clang-tidy` for recommended static analysis. Neither is enforced; focus on readability rather than strict adherence.
   * Add or update unit tests under `test/` when changing testable logic. Tests use GoogleTest and build into a single `vcmitest` binary. Automated tests primarily cover core, server, and AI logic; client UI coverage is currently limited.
   * Only the server may mutate `CGameState`. Changes that affect saved game state need serialization-compatibility handling for existing saves — see [Serialization](docs/developers/Serialization.md).
   * Follow the conventions in [`AGENTS.md`](AGENTS.md) — error handling, comments, avoiding duplication, internationalization.
   * Commit your changes in logical chunks, with clear commit messages.

5. Push your topic branch to your fork and [open a pull request](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/creating-a-pull-request-from-a-fork) against `develop`.
   * Target `develop` if you want your change in the next *major* release. If a `beta` branch exists, you can target it instead for a fix intended for the next minor ("hotfix") release — but please verify with the team first, since changes to `beta` are usually limited to avoid regressions.
   * Fill out the [pull request template](.github/PULL_REQUEST_TEMPLATE.md). It is a short checklist. Include a section on user-facing changes that can be copied to [ChangeLog.md](ChangeLog.md) later, but do not edit the changelog yourself.

### Pull request validation

CI (GitHub Actions) builds and runs the test suite against your pull request; the results show as checks on the pull request page. A failing check usually means a compile error or a test failure on one of the supported platforms — click through to the run's log to see why.

A maintainer will merge your pull request once it's approved and CI passes. Please be responsive to review feedback. If you have been waiting for a maintainer for too long (more than a day or two), feel free to ask for a review on Discord (preferably) or add a comment to your pull request.

### Are there any development docs?

[`docs/developers/`](docs/developers/) contains our developer documentation, but much of it is outdated. [`Code_Structure.md`](docs/developers/Code_Structure.md) is the best starting point for an overview of the codebase; [`AGENTS.md`](AGENTS.md) is a shorter architecture reference. For modding-related contributions, see [`docs/modders/Readme.md`](docs/modders/Readme.md).

## Use of AI

You may use AI tools to help write code, tests, translations, or documentation. However, we do not accept submissions that the contributor does not understand. Asking a model to "fix issue #12345" and opening a pull request with its result transfers the work to the maintainers.

The line is ownership, not tooling:

* **You must be the author of the text.** Issue and pull request descriptions, and comments in discussions, have to be written by you and reflect your own understanding. Using an AI tool to proofread or translate your text is fine; generating it is not. You need to be able to answer questions about what you wrote.

* **You must understand every line of code you submit.** If you cannot explain what your change does, why it works, and how it can fail, do not open the pull request, regardless of who or what wrote it.

* **Match the scope to what you can vouch for.** An AI-assisted patch to an area you know well is welcome. An AI-assisted patch to an unfamiliar part of the engine, submitted because the model said it works, is not — please ask on [Discord](https://discord.gg/chBT42V) first.

Fully automated submissions will be closed. People who repeatedly submit them may be blocked from this repository without warning. The team may approve exceptions for regular maintenance tasks, but the submission must clearly state that it was generated automatically.

This policy keeps the workload balanced. A human must carefully review every submission. A change that its contributor cannot explain transfers all the work to the reviewer, who cannot resolve questions by talking to the contributor.

## Project goals

VCMI is an open-source engine that reimplements the Heroes of Might and Magic III engine. The goals of the core project are:

* Stay faithful to the original game's mechanics and feel, while fixing bugs and limitations of the original engine.
* Be cross-platform, running on the same content across Windows, Linux, macOS, Android and iOS.
* Provide a stable, well-documented foundation — through the mod format and content APIs — for the community to extend gameplay without needing to fork the engine.

In contrast, we do not encourage extending or changing the gameplay of the base game. Changes to the default gameplay that are not present in the original game must be implemented as a mod or as a game setting that a mod can change.

For a long time, the official branch also accepted features that users could enable or disable. However, interactions between some configurations made parts of the code very complicated. New features must now work with all existing features. Handling these interactions requires considerably more work than making a feature work in only one game mode.

The preferred way to introduce new gameplay features is to extend the content APIs so that they support more add-on content and mods.

If you'd like to build something that doesn't fit these goals, a mod is usually the right place for it — see [Modding Guidelines](docs/modders/Readme.md).

## Legal stuff

VCMI source code is licensed under GPL version 2 or later. By submitting a pull request, you agree to license your contribution under the same terms.

VCMI assets such as graphics and sounds are licensed under CC-BY-SA 4.0. If applicable, also provide the source assets in the [vcmi-assets](https://github.com/vcmi/vcmi-assets) repository.

This contributing guide is adapted from [OpenTTD's guide](https://github.com/OpenTTD/OpenTTD/blob/master/CONTRIBUTING.md).

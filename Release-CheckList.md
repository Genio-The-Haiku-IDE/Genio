# Release Checklist

## Before release

* Test for critical problems and fix them
* Update Documentation (README, changelog and screenshots)

## Release

* Create release branch (only for major releases, 10/15 days before release)
* Bump version
* Update translations
  * Regenerate english catkeys (with 'make catkeys').
  * Upload en.catkeys to polyglot.
  * Wait some time for updated translations.
  * Download completed translations and import into repository.
  * Update makefile to include new translations.
* Retest build on supported platforms (x86_64 and x86).
* Create GitHub release from the release branch.
* Review haikuports recipe.
  * Update dependencies (if changed).
  * Change recipe to point at the new release tag.
  * Test recipe by building with haikuporter.
  * Install and briefly test package.
  * Make a PR to haikuports with the new recipe.

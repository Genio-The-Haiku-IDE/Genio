# Release Checklist

*   Check for critical problems and fix them
*   Documentation
    *   Update README, changelog and screenshot
*   Release
    *   Create release branch (10/15 days before release)
    *   Bump version
    *   Regenerate english catkeys and upload them in polyglot
    *   Wait for updated translations
    *   Create release from the release branch
    *   Review haikuports recipe
        *   Change it to point at the new release tag
        *   Make a PR to haikuports with the new recipe

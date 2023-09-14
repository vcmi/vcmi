< [Documentation](../Readme.md) / Release Process

# Branches
Our branching strategy is very similar to GitFlow
- `master` branch has release commits. One commit - one release. Each release commit should be tagged with version `maj.min.patch`
- `beta` branch is for stabilization of ongoing release. Only safe changes should be targeted into this branch. Breaking changes (e.g. save format changes) are forbidden in beta.
- `develop` branch is a main branch for ongoing development. Pull requests with new features should be targeted to this branch, `develop` version is one major release ahead of `beta`

# Release process step-by-step

### Initial release setup (major releases only)
Should be done immediately after start of stabilization stage for previous release

- Create project named `Release 1.X`
- Add all features and bugs that should be fixed as part of this release into this project

### Start of stabilization stage (major releases only)
Should be done 2-4 weeks before planned release date. All major features should be finished at this point.

- Create `beta` branch from `develop`
- Bump vcmi version in CMake on `develop` branch
- Bump version for Linux on `develop` branch
- Bump version and build ID for Android on `develop` branch

### Initial release setup (patch releases only)

- Bump vcmi version in CMake on `beta` branch
- Bump version for Linux on `beta` branch
- Bump version and build ID for Android on `beta` branch

### Release preparation stage
Should be done 1 week before release. Release date should be decided at this point.

- Make sure to announce codebase freeze deadline (1 day before release) to all developers
- Create pull request for release preparation tasks targeting `beta`:
- - Update [changelog](https://github.com/vcmi/vcmi/blob/develop/ChangeLog.md)
- - Update release date for Linux packaging. See [example](https://github.com/vcmi/vcmi/pull/1258)
- - Update build ID for Android packaging. See [example](https://github.com/vcmi/vcmi/pull/2090)
- - Update downloads counter in readme.md. See [example](https://github.com/vcmi/vcmi/pull/2091)

### Release preparation stage
Should be done 1 day before release. At this point beta branch is in full freeze.

- Merge release preparation PR into `beta`
- Merge `beta` into `master`. This will trigger CI pipeline that will generate release packages
- Create draft release page, specify `1.x.y` as tag for `master` after publishing
- Check that artifacts for all platforms have been build by CI `master` branch
- Attach build artifacts for all platforms to release page
- Manually extract Windows installer, remove `$PLUGINSDIR` directory and repackage data as .zip archive
- Attach produced zip archive to release page as an alternative Windows installer
- Upload built AAB to Google Play and send it for review
- Prepare pull request for [vcmi-updates](https://github.com/vcmi/vcmi-updates)
- (major releases only) Prepare pull request with release update for web site https://github.com/vcmi/VCMI.eu

### Release publishing phase
Should be done on release date

- Trigger builds for new release on Ubuntu PPA
- Publish new release on Google Play
- Publish release page
- Merge `master` into `develop`
- Merge prepared PR's for vcmi-updates and for website (if any)
- Close `Release 1.x` project
- Write announcements in social networks

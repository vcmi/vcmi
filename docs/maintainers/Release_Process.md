## Versioning
For releases VCMI uses version numbering in form "1.X.Y", where:
- 'X' indicates major release. Different major versions are generally not compatible with each other. Save format is different, network protocol is different, mod format likely different.
- 'Y' indicates hotfix release. Despite its name this is usually not urgent, but planned release. Different hotfixes for same major version are fully compatible with each other.

## Branches
Our branching strategy is very similar to GitFlow:
- `master` branch has release commits. One commit - one release. Each release commit should be tagged with version `1.X.Y` when corresponding version is released. State of master branch represents state of latest public release.
- `beta` branch is for stabilization of ongoing release. Beta branch is created when new major release enters stabilization stage and is used for both major release itself as well as for subsequent hotfixes. Only changes that are safe, have minimal chance of regressions and improve player experience should be targeted into this branch. Breaking changes (e.g. save format changes) are forbidden in beta.
- `develop` branch is a main branch for ongoing development. Pull requests with new features should be targeted to this branch, `develop` version is one major release ahead of `beta`.

## Release process step-by-step

### Initial release setup (major releases only)
Should be done immediately after start of stabilization stage for previous release

- Create project named `Release 1.X`
- Add all features and bugs that should be fixed as part of this release into this project

### Start of stabilization stage (major releases only)
Should be done 2 weeks before planned release date. All major features should be finished at this point.

- Create `beta` branch from `develop`
- Bump vcmi version in CMake on `develop` branch
- Bump version for Linux on `develop` branch
- Bump version and build ID for Android on `develop` branch

### Initial release setup (hotfix releases only)

- Bump vcmi version in CMake on `beta` branch
- Bump version for Linux on `beta` branch
- Bump version and build ID for Android on `beta` branch

### Release preparation stage
Should be done 1 week before release. Release date should be decided at this point.

- Make sure to announce codebase freeze deadline (1 day before release) to all developers
- Create pull request for release preparation tasks targeting `beta`:
- - Update [changelog](https://github.com/vcmi/vcmi/blob/develop/ChangeLog.md)
- - Update release date in `debian/changelog`
- - Update release date in `launcher/eu.vcmi.VCMI.metainfo.xml`
- - Update build ID `android/vcmi-app/build.gradle`
- - Update downloads counter in `docs/readme.md`

### Release preparation stage
Should be done 1 day before release. At this point beta branch is in full freeze.

- Merge release preparation PR into `beta`
- Merge `beta` into `master`. This will trigger CI pipeline that will generate release packages
- Create draft release page, specify `1.x.y` as tag for `master` after publishing
- Check that artifacts for all platforms have been built by CI on `master` branch
- Download and rename all build artifacts to use form "VCMI-1.X.Y-Platform.xxx" 
- Attach build artifacts for all platforms to release page
- Manually extract Windows installer, remove `$PLUGINSDIR` directory which contains installer files and repackage data as .zip archive
- Attach produced zip archive to release page as an alternative Windows installer
- Upload built AAB to Google Play and send created release draft for review (usually takes several hours)
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

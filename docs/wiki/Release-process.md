## Branches
Our branching strategy is very similar to GitFlow
* `master` branch has release commits. One commit - one release. Each release commit should be tagged with version `maj.min.patch`
* `beta` branch is for stabilization of ongoing release. We don't merge new features into `beta` branch - only bug fixes are acceptable. Hotfixes for already released software should be delivered to this branch as well.
* `develop` branch is a main branch for ongoing development. Pull requests with new features should be targeted to this branch, `develop` version is one step ahead of 'beta`

## Release process step-by-step
Assuming that all features planned to be released are implemented in `develop` branch and next release version will be `1.x.0`

### Start of stabilization stage
Should be done several weeks before planned release date

- Create [milestone](https://github.com/vcmi/vcmi/milestones) named `Release 1.x`
- - Specify this milestone for all regression bugs and major bugs related to new functionality
- Create `beta` branch from `develop`
- Bump vcmi version on `develop` branch
- Bump version for linux on `develop` branch

### Release preparation stage
Should be done several days before planned release date

- Update [release notes](https://github.com/vcmi/vcmi/blob/develop/ChangeLog.md)
- Update release date for Linux packaging. See [example](https://github.com/vcmi/vcmi/pull/1258)
- Update release date for Android packaging. See [example](https://github.com/vcmi/vcmi/pull/2090)
- Create draft release page, specify `1.x.0` as tag for `master` after publishing
- Prepare pull request with release update for web site https://github.com/vcmi/VCMI.eu
- Prepare pull request for [vcmi-updates](https://github.com/vcmi/vcmi-updates)

### Release publishing phase
Should be done on release date

- Check that all items from milestone `Release 1.x` completed
- Merge `beta` into `master`
- Check that artifacts for all platforms are available on `master` branch
- Trigger builds for new release on Ubuntu PPA
- Attach build artifacts for all platforms to release page
- - Android build should be prepared manually from custom branch (master)
- Merge prepared pull requests
- Publish release page

### After release publish

- Close `Release 1.x` milestone
- Write announcements in social networks
- Merge `master` into `develop`
- Update downloads counter in readme.md. See [example](https://github.com/vcmi/vcmi/pull/2091)

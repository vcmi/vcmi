## Main links
- [Team](https://launchpad.net/~vcmi)
- [Project](https://launchpad.net/vcmi)
- [Sources](https://code.launchpad.net/~vcmi/vcmi/+git/vcmi)
- [Recipes](https://code.launchpad.net/~vcmi/+recipes)
- - [Stable recipe](https://code.launchpad.net/~vcmi/+recipe/vcmi-stable)
- - [Daily recipe](https://code.launchpad.net/~vcmi/+recipe/vcmi-daily)
- PPA's
- - [Stable PPA](https://launchpad.net/~vcmi/+archive/ubuntu/ppa)
- - [Daily PPA](https://launchpad.net/~vcmi/+archive/ubuntu/vcmi-latest)

## Automatic daily builds process
### Code import
- Launchpad performs regular (once per few hours) clone of our git repository.
- This process can be observed on [Sources](https://code.launchpad.net/~vcmi/vcmi/+git/vcmi) page.
- If necessary, it is possible to trigger fresh clone immediately (Import Now button)
### Build dependencies
- All packages required for building of vcmi are defined in [debian/control](https://github.com/vcmi/vcmi/blob/develop/debian/control) file
- Launchpad will automatically install build dependencies during build
- Dependencies of output .deb package are defined implicitly as dependencies of packages required for build
### Recipe building
- Every 24 hours Launchpad triggers daily builds on all recipes that have build schedule enable. For vcmi this is [Daily recipe](https://code.launchpad.net/~vcmi/+recipe/vcmi-daily)
- Alternatively, builds can be triggered manually using "request build(s) link on recipe page. VCMI uses this for [Stable recipe](https://code.launchpad.net/~vcmi/+recipe/vcmi-stable)
### Recipe content (build settings)
- Version of resulting .deb package is set in recipe content, e.g `{debupstream}+git{revtime}` for daily builds
- Base version (referred as `debupstream` on Launchpad is taken from source code, [debian/changelog](https://github.com/vcmi/vcmi/blob/develop/debian/changelog) file
- CMake configuration settings are taken from source code, [debian/rules](https://github.com/vcmi/vcmi/blob/develop/debian/rules) file
- Branch which is used for build is specified in recipe content, e.g. `lp:vcmi master`
## Workflow for creating a release build
- if necessary, push all required changes including `debian/changelog` update to `vcmi/master` branch
- Go to [Sources](https://code.launchpad.net/~vcmi/vcmi/+git/vcmi) and run repository import.
- Wait for import to finish, which usually happens within a minute. Press F5 to actually see changes.
- Go to [Stable recipe](https://code.launchpad.net/~vcmi/+recipe/vcmi-stable) and request new builds
- Wait for builds to finish. This takes quite a while, usually - over a hour, even more for arm builds
- Once built, all successfully built packages are automatically copied to PPA linked to the recipe
- If any of builds have failed, open page with build info and check logs. 
## People with access
- [alexvins](https://github.com/alexvins) (https://launchpad.net/~alexvins)
- [ArseniyShestakov](https://github.com/ArseniyShestakov) (https://launchpad.net/~sxx)
- [IvanSavenko](https://github.com/IvanSavenko) (https://launchpad.net/~saven-ivan)
- (Not member of VCMI, creator of PPA) (https://launchpad.net/~mantas)
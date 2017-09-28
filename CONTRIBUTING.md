# Contributing to vtzero

## Releasing

To release a new vtzero version:

 - Make sure all tests are passing locally and on travis
 - Update version number in
   - include/vtzero/version.hpp (two places)
 - Update CHANGELOG.md
 - Update UPGRADING.md if necessary
 - Tag a new release and push to github `git tag vX.Y.Z && git push --tags`
 - Go to https://github.com/mapbox/vtzero/releases
   and edit the new release. Put "Version x.y.z" in title and
   cut-and-paste entry from CHANGELOG.md.

## Code Formatting

We use [this script](/scripts/setup.sh#L7%60) to install a consistent version of [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) to format the code base. The format is automatically checked via a Travis CI build as well. Run the following script locally to ensure formatting is ready to merge:

    make format

## Updating submodules

Call `git submodule update --recursive --remote` to update to the newest
version of the mvt fixtures used for testing.


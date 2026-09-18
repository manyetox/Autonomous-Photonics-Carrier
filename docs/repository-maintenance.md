# Repository maintenance notes

## Public-source hygiene

The repository intentionally omits KiCad local history repositories, autosave backup folders, lock files, and personal project-local settings. Git history should be used for public revision tracking instead.

## Updating hardware

When a board changes:

1. increment that board's revision only;
2. do not create a new Platform generation unless the system architecture itself changes;
3. move the replaced fabrication package to `archive/`;
4. generate/update the board BOM CSV;
5. update `VERSIONING.md`, the board README, and `CHANGELOG.md`;
6. record validation status honestly;
7. update fabrication checksums.

## Image policy

Photographs should identify the board revision in the caption or filename when practical. Avoid using a photo of an older board next to a current fabrication link without clearly labelling it.

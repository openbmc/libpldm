# Checklist for making releases of `libpldm`

## Meson

- [ ] Update the version in `meson.build`

## ABI

- [ ] Generate the ABI dump for the release

  - This must be done from a shell session inside the OpenBMC CI Docker
    container for consistency

## Evolutions

- [ ] Rename the directory for unreleased evolutions

## CHANGELOG

- [ ] Replace the `Unreleased` header with the tag value and the date
- [ ] Remove headers of empty sections from current release
- [ ] Introduce new `Unreleased` header
- [ ] Add the usual list of headers with empty sections

## Tagging

- [ ] Commit the changes above with the subject `libpldm: Release <version>`
- [ ] Push the release commit for review in Gerrit
- [ ] Submit the release commit once approved
- [ ] Create the release tag
- [ ] Push the release tag

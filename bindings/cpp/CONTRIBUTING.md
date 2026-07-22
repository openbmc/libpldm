# Checklist and design considerations for libpldm++

Most of this is in addition to libpldm's `CONTRIBUTING.md` and there may be some
overlap.

## Adding support for a new PLDM fw update package revision

- [ ] A new PackagePin enum member has been created
- [ ] Unit tests cover parsing of the newly added struct members
- [ ] The fuzz test has been updated
- [ ] A changelog entry has been made

## Adding support for new struct members

- [ ] New struct members are named the same as they appear in the spec
- [ ] New struct members are appended at the end of the struct
- [ ] New struct members have a comment as to which `PackagePin` introduced them
- [ ] The overloaded operators have been updated

## Choosing a type for new struct members

- If the struct member is itself a structure that may grow in the future,
  `unique_ptr` should be used

- If a container or map member type may grow in the future and has effects on
  the container/map size, `unique_ptr` should be used to wrap it

## Namespaces

- [ ] Added code lives under the `pldm` namespace
- [ ] There are no unresolved name conflicts with any types defined in known
      libpldm(++) users

## ABI surface

The existing design has been done to minimize the ABI surface. Functions are
exposed as-needed.

## Influences on the design

- [Mighty Strucit][mighty-struct]

[mighty-struct]:
  https://github.com/lotem/mighty_struct/blob/master/mighty_struct_explained.md

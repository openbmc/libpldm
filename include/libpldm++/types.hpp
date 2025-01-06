#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

// namespace libpldm is for things which are not directly related to PLDM specifications
namespace libpldm
{
// define this base class to avoid library users having
// copy constructors and move constructors auto-generated, which could mismatch
// with the library ABI
struct NonCopyableNonMoveable {
	NonCopyableNonMoveable() = default;
	NonCopyableNonMoveable(const NonCopyableNonMoveable &) = delete;
	NonCopyableNonMoveable &
	operator=(const NonCopyableNonMoveable &) = delete;
	NonCopyableNonMoveable(NonCopyableNonMoveable &&) = delete;
	NonCopyableNonMoveable &operator=(NonCopyableNonMoveable &&) = delete;

    protected:
	~NonCopyableNonMoveable() = default;
};

// note (does not apply to library users):
// users of derived classes must use 'HasMember' functions
// before read/write member access to fields
template <class Derived> struct GrowableStruct {
    protected:
	// size of the derived class in bytes
	const size_t struct_size = sizeof(Derived);

	// reserved for future use
	uint64_t reserved_1 = 0;

	char *address() const
	{
		return (char *)this;
	}

	template <class T> bool HasMember(const T Derived::*member) const
	{
		return struct_size >
		       (char *)&(static_cast<const Derived *>(this)->*member) -
			       address();
	}

	// checked access
	template <class T>
	bool CompareEQ(const T Derived::*member, const Derived &other) const
	{
		if (HasMember(member) && !other.HasMember(member)) {
			return false;
		}
		if (!HasMember(member) && other.HasMember(member)) {
			return false;
		}
		if (!HasMember(member) && !other.HasMember(member)) {
			return true;
		}

		return *(GetPtr(member)) == *(other.GetPtr(member));
	}

	// checked access
	template <class T>
	bool CompareNEQ(const T Derived::*member, const Derived &other) const
	{
		if (HasMember(member) && !other.HasMember(member)) {
			return true;
		}
		if (!HasMember(member) && other.HasMember(member)) {
			return true;
		}
		if (!HasMember(member) && !other.HasMember(member)) {
			return false;
		}

		return *(GetPtr(member)) != *(other.GetPtr(member));
	}

    private:
	// unchecked access
	template <class T> T Get(const T Derived::*member) const
	{
		return static_cast<const Derived *>(this)->*member;
	}

	// unchecked access
	template <class T> const T *GetPtr(const T Derived::*member) const
	{
		return &((static_cast<const Derived *>(this))->*member);
	}
};

} // namespace libpldm

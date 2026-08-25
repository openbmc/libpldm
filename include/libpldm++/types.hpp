#pragma once

#include <algorithm>
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

	// compare K,V pairs in maps where V is a pointer
	template <class T> static bool compareKV(const T &a, const T &b)
	{
		if (a.first != b.first) {
			return false;
		}

		if (!a.second || !b.second) {
			return !a.second && !b.second;
		}

		return *a.second == *b.second;
	}

	// deep compare maps for equality where the values are unique_ptr.
	// Values with the same key may appear in any order, but their
	// multiplicities must match. This works for maps and multimaps.
	template <class T>
	bool CompareMapEq(const T Derived::*member, const Derived &other) const
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

		const auto &lhs = *GetPtr(member);
		const auto &rhs = *other.GetPtr(member);

		if (lhs.size() != rhs.size()) {
			return false;
		}

		for (auto first = lhs.begin(); first != lhs.end();) {
			const auto last = lhs.upper_bound(first->first);
			const auto [otherFirst, otherLast] =
				rhs.equal_range(first->first);

			if (!std::is_permutation(
				    first, last, otherFirst, otherLast,
				    compareKV<typename T::value_type>)) {
				return false;
			}
			first = last;
		}
		return true;
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

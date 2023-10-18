#pragma once
#include <robin_hood.h>
#include <Core/String.h>
#include <Core/Mutex.h>

namespace fe
{
	/**
	 * The Name class is extremely light-weight String-String comparing helper class.
	 * It provides light-weight string-string comparing based on 'CRC64' hash.
	 * And also, it is convertible as std::string.
	 *
	 * The Name class will strictly check that the input string properly encoded as UTF-8.
	 *
	 * It treats empty string as 'invalid'.
	 * It treats not properly encoded as UTF-8 string as 'invalid'.
	 * It treats comparing with 'invalid' name is always 'false'.
	 *
	 * The AsString() function will return 'empty' string for the 'invalid' name.
	 *
	 * The construction of Name instances can lead to performance issues due to CRC64 hash evaluation.
	 * Therefore, it is recommended to avoid creating them anew and using of SetString in loops.
	 */
	class Name final
	{
	public:
		Name() = default;
		Name(std::string_view name);
		Name(const Name& name);
		~Name() = default;

		void SetString(std::string_view nameString);

		bool	 IsValid() const { return hashOfString != InvalidNameHash; }
		uint64_t GetHash() const { return hashOfString; }

		Name& operator=(std::string_view nameString);
		operator std::string() const
		{
			return AsString();
		}
		operator std::string_view() const { return AsStringView(); }
		std::string AsString() const;
		std::string_view AsStringView() const;

		operator bool() const { return IsValid(); }

		Name& operator=(const Name& name);

		bool operator==(std::string_view nameString) const;
		bool operator!=(const std::string_view nameString) const { return !(*this == nameString); }
		bool operator==(const Name& name) const;
		bool operator!=(const Name& name) const { return !(*this == name); }

	private:
		using HashStringMap = robin_hood::unordered_map<uint64_t, std::string>;
		static HashStringMap hashStringMap;
		static SharedMutex	 hashStringMapMutex;

		constexpr static uint64_t InvalidNameHash = 0;

	private:
		uint64_t hashOfString = InvalidNameHash;
	};

} // namespace fe

namespace std
{
	template <>
	class hash<fe::Name>
	{
	public:
		size_t operator()(const fe::Name& name) const
		{
			return name.GetHash();
		}
	};
} // namespace std

#define FE_NAME(x) fe::Name(x)
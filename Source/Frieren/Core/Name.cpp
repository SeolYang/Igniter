#include <Core/Name.h>
#include <Core/Hash.h>
#include <Core/Assert.h>

namespace fe
{
	Name::HashStringMap Name::hashStringMap = {};
	SharedMutex			Name::hashStringMapMutex;

	Name::Name(const std::string_view nameString)
		: hashOfString(InvalidNameHash)
	{
		SetString(nameString);
	}

	Name::Name(const Name& name)
		: hashOfString(name.hashOfString)
	{
	}

	void Name::SetString(const std::string_view nameString)
	{
		const bool bIsNotEmpty = !nameString.empty();
		FE_ASSERT(bIsNotEmpty, "Empty Name");

		const bool bIsEncodedAsUTF8 = IsValidUTF8(nameString);
		FE_ASSERT(bIsEncodedAsUTF8, "Invalid UTF-8 String: {}", nameString);

		const bool bIsValidString = bIsNotEmpty && bIsEncodedAsUTF8;
		if (bIsValidString)
		{
			hashOfString = Private::HashCRC64(nameString);

			WriteLock lock{ hashStringMapMutex };
			if (!hashStringMap.contains(hashOfString))
			{
				hashStringMap[hashOfString] = nameString;
			}
		}
		else
		{
			hashOfString = InvalidNameHash;
		}
	}

	std::string Name::AsString() const
	{
		if (hashOfString == InvalidNameHash)
		{
			return std::string();
		}

		ReadOnlyLock lock{ hashStringMapMutex };
		return hashStringMap[hashOfString];
	}

	Name& Name::operator=(const std::string_view nameString)
	{
		SetString(nameString);
		return *this;
	}

	Name& Name::operator=(const Name& name)
	{
		hashOfString = name.hashOfString;
		return *this;
	}

	bool Name::operator==(const std::string_view nameString) const
	{
		return (*this) == Name{ nameString };
	}

	bool Name::operator==(const Name& name) const
	{
		const bool bValidNames = IsValid() && name.IsValid();
		FE_ASSERT(bValidNames, "Invalid name comparision.");
		return bValidNames ? hashOfString == name.hashOfString : false;
	}

} // namespace fe

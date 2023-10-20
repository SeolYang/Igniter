#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>

namespace fe
{
	String::HashStringMap String::hashStringMap = {};
	SharedMutex			String::hashStringMapMutex;

	String::String(const std::string_view stringView)
		: hashOfString(InvalidHash)
	{
		SetString(stringView);
	}

	String::String(const String& other)
		: hashOfString(other.hashOfString)
	{
	}

	void String::SetString(const std::string_view nameString)
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
			hashOfString = InvalidHash;
		}
	}

	std::string String::AsString() const
	{
		if (hashOfString == InvalidHash)
		{
			return std::string();
		}

		ReadOnlyLock lock{ hashStringMapMutex };
		return hashStringMap[hashOfString];
	}

	std::string_view String::AsStringView() const
	{
		if (hashOfString == InvalidHash)
		{
			return {};
		}

		ReadOnlyLock lock{ hashStringMapMutex };
		return hashStringMap[hashOfString];
	}

	String& String::operator=(const std::string_view nameString)
	{
		SetString(nameString);
		return *this;
	}

	String& String::operator=(const String& name)
	{
		hashOfString = name.hashOfString;
		return *this;
	}

	bool String::operator==(const std::string_view nameString) const
	{
		return (*this) == String{ nameString };
	}

	bool String::operator==(const String& name) const
	{
		const bool bValidNames = IsValid() && name.IsValid();
		FE_ASSERT(bValidNames, "Invalid name comparision.");
		return bValidNames ? hashOfString == name.hashOfString : false;
	}

} // namespace fe

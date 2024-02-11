#include <Core/String.h>
#include <Core/Hash.h>
#include <Core/Assert.h>

namespace fe
{
	String::HashStringMap String::hashStringMap = {};
	SharedMutex			  String::hashStringMapMutex;

	String::String(const std::string_view stringView) : hashOfString(InvalidHash)
	{
		SetString(stringView);
	}

	String::String(const String& other) : hashOfString(other.hashOfString) {}

	void String::SetString(const std::string_view stringView)
	{
		if (!stringView.empty() && IsValidUTF8(stringView))
		{
			hashOfString = HashStringCRC64(stringView);

			WriteLock lock{ hashStringMapMutex };
			if (!hashStringMap.contains(hashOfString))
			{
				hashStringMap[hashOfString] = stringView;
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

	std::wstring String::AsWideString() const
	{
		return Wider(AsStringView());
	}

	void String::ClearCache()
	{
		WriteLock lock{ hashStringMapMutex };
		hashStringMap.clear();
	}

	String& String::operator=(const std::string& rhs)
	{
		SetString(rhs);
		return *this;
	}

	String& String::operator=(const std::string_view rhs)
	{
		SetString(rhs);
		return *this;
	}

	String& String::operator=(const String& rhs)
	{
		hashOfString = rhs.hashOfString;
		return *this;
	}

	bool String::operator==(const std::string_view rhs) const
	{
		return (*this) == String{ rhs };
	}

	bool String::operator==(const String& rhs) const
	{
		return (IsValid() && rhs.IsValid()) ? hashOfString == rhs.hashOfString : false;
	}

} // namespace fe

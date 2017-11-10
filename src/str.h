#ifndef ABYSS_STR_H
#define ABYSS_STR_H

class Str
{
public:
	char* str;
	size_t len;
	Str()
	{
		str = NULL;
		len = 0;
	}

	Str(char* p, size_t l)
	{
		str = p;
		len = l;
	}

	bool operator==(const Str& other)
	{
		if(len != other.len)
			return false;
		for(size_t i = 0; i < len; i++)
		{
			if(*(str + i) != *(other.str + i))
				return false;
		}
		return true;
	}

	bool operator!=(const Str& other)
	{
		return !((*this) == other);
	}

	bool operator<(const Str& other)//For map
	{
		if(len < other.len)
		{
			for(size_t i = 0; i < len; i++)
			{
				if(*(str + i) > *(other.str + i))
					return false;
				else if(*(str + i) < *(other.str + i))
					return true;
			}
			return true;
		}
		else
		{
			for(size_t i = 0; i < other.len; i++)
			{
				if(*(str + i) > *(other.str + i))
					return false;
				else if(*(str + i) < *(other.str + i))
					return true;
			}
			return false;
		}
	}
};

#endif
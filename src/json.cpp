#include<iostream>
#include<map>
#include<vector>
#include<string>
#include<cmath>

#include"json.h"

using namespace std;

namespace gao
{
	ostream& operator<<(ostream& out, JsonValue& json)
	{
		out << "Json Type : ";
		switch (json.JSONType())
		{
		case Type::JSON_ARRAY:
			out << "array\n";
			break;
		case Type::JSON_BOOL:
			out << "bool\n";
			break;
		case Type::JSON_FLOAT:
			out << "float\n";
			break;
		case Type::JSON_INTEGRAL:
			out << "integral\n";
			break;
		case Type::JSON_NULL:
			out << "null\n";
			break;
		case Type::JSON_STRING:
			out << "string\n";
			break;
		case Type::JSON_OBJECT:
			out << "object\n";
			break;
		default:
			;
		}
		out << json.dumps() << endl;
		return out;
	}

	JsonParseStatus parse_number(JsonValue& res, const string& str, size_t& offset)
	{
		string val = "", exp = "";
		bool isfloat = false;
		if (str[offset] == '-')
		{
			val += '-';
			++offset;
		}
		char c = str[offset];
		while (isdigit(c) || (c == '.' && !isfloat))
		{
			if (c == '.' && !isfloat)
				isfloat = true;
			val += c;
			c = str[++offset];
		}
		if (val.empty() || val == "-" || val == "." || val == "-.")
			return JsonParseStatus::JSON_PARSE_ERROR;

		if (c == 'e' || c == 'E')
		{
			++offset;
			if (str[offset] == '-')
			{
				exp += '-';
				++offset;
			}
			c = str[offset];
			while (isdigit(c))
			{
				exp += c;
				c = str[++offset];
			}
			if (exp.empty() || exp == "-")
				return JsonParseStatus::JSON_PARSE_ERROR;
		}

		pass_whitespace(str, offset);
		c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		if (exp.empty())
		{
			if (isfloat)
			{
				double dou = stod(val);
				res = JsonValue(dou);
			}
			else
			{
				long long ll = stoll(val);
				res = JsonValue(ll);
			}
		}
		else
		{
			double dou = stod(val) * pow(10, stol(exp));
			res = JsonValue(dou);
		}
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse_true(JsonValue& res, const string& str, size_t& offset)
	{
		if (!(str[offset] == 't' && str[offset + 1] == 'r' && str[offset + 2] == 'u' && str[offset + 3] == 'e'))
			return JsonParseStatus::JSON_PARSE_ERROR;
		else
			offset += 4;
		pass_whitespace(str, offset);
		char c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		res = JsonValue(true);
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse_false(JsonValue& res, const string& str, size_t& offset)
	{
		if (!(str[offset] == 'f' && str[offset + 1] == 'a' && str[offset + 2] == 'l' && str[offset + 3] == 's' && str[offset + 4] == 'e'))
			return JsonParseStatus::JSON_PARSE_ERROR;
		else
			offset += 5;
		pass_whitespace(str, offset);
		char c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		res = JsonValue(false);
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse_bool(JsonValue& res, const string& str, size_t& offset)
	{
		if (str[offset] == 't')
			return parse_true(res, str, offset);
		else
			return parse_false(res, str, offset);
	}

	JsonParseStatus parse_null(JsonValue& res, const string& str, size_t& offset)
	{
		if (!(str[offset] == 'n' && str[offset + 1] == 'u' && str[offset + 2] == 'l' && str[offset + 3] == 'l'))
			return JsonParseStatus::JSON_PARSE_ERROR;
		else
			offset += 4;
		pass_whitespace(str, offset);
		char c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		res = JsonValue();
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse_string(JsonValue& res, const string& str, size_t& offset)
	{
		string val = "";
		char c = str[++offset];
		while (c != '\"')
		{
			if (c == '\\')
			{
				switch (str[++offset])
				{
					case '\"': val += '\"'; break;
					case '\\': val += '\\'; break;
					case '/': val += '/'; break;
					case 'b': val += '\b'; break;
					case 'f': val += '\f'; break;
					case 'n': val += '\n'; break;
					case 'r': val += '\r'; break;
					case 't': val += '\t'; break;
					case 'u':
					{
						val += "\\u";
						for (int i = 1; i < 5; i++)
						{
							if (isdigit(str[offset + i]) || (str[offset + i] >= 'a' && str[offset + i] <= 'f') || (str[offset + i] >= 'A' && str[offset + i] <= 'F'))
								val += str[offset + i];
							else
								return JsonParseStatus::JSON_PARSE_ERROR;
						}
						offset += 4;
					}
					break;
					default: return JsonParseStatus::JSON_PARSE_ERROR;
				}
			}
			else
				val += c;
			c = str[++offset];
		}
		++offset;
		pass_whitespace(str, offset);
		c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}' && c != ':')
			return JsonParseStatus::JSON_PARSE_ERROR;
		res = val;
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse(JsonValue&, const string&, size_t&);

	JsonParseStatus parse_object(JsonValue& res, const string& str, size_t& offset)
	{
		res = JsonValue::make(Type::JSON_OBJECT);
		char c = str[offset];
		while (c != '}')
		{
			++offset;
			pass_whitespace(str, offset);
			if (res.getSize() == 0)
			{
				if (str[offset] == '}')
					break;
			}
			string key = "";
			JsonValue valueKey = JsonValue();
			if (parse_string(valueKey, str, offset) == JsonParseStatus::JSON_PARSE_OK)
				key = valueKey.getString();
			else
				return JsonParseStatus::JSON_PARSE_ERROR;
			c = str[offset];
			if (c == ':')
			{
				++offset;
				pass_whitespace(str, offset);
				JsonValue value = JsonValue();
				if (parse(value, str, offset) == JsonParseStatus::JSON_PARSE_OK)
					res[key] = value;
				else
					return JsonParseStatus::JSON_PARSE_ERROR;
				if (str[offset] != ',' && str[offset] != '}')
					return JsonParseStatus::JSON_PARSE_ERROR;
				c = str[offset];
			}
			else
				return JsonParseStatus::JSON_PARSE_ERROR;
		}
		++offset;
		pass_whitespace(str, offset);
		c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse_array(JsonValue& res, const string& str, size_t& offset)
	{
		res = JsonValue::make(Type::JSON_ARRAY);
		char c = str[offset];
		while (c != ']')
		{
			++offset;
			pass_whitespace(str, offset);
			if (res.getSize() == 0)
			{
				if (str[offset] == ']')
					break;
			}
			JsonValue temp = JsonValue();
			if (parse(temp, str, offset) == JsonParseStatus::JSON_PARSE_OK)
				res.append(temp);
			else
				return JsonParseStatus::JSON_PARSE_ERROR;
			c = str[offset];
			if (c != ',' && c != ']')
				return JsonParseStatus::JSON_PARSE_ERROR;
		}
		++offset;
		pass_whitespace(str, offset);
		c = str[offset];
		if (c != '\0' && c != ',' && c != ']' && c != '}')
			return JsonParseStatus::JSON_PARSE_ERROR;
		return JsonParseStatus::JSON_PARSE_OK;
	}

	JsonParseStatus parse(JsonValue& res, const string& str, size_t& offset)
	{
		pass_whitespace(str, offset);
		switch (str[offset])
		{
			case '[':
				return parse_array(res, str, offset);
			case '{':
				return parse_object(res, str, offset);
			case '\"':
				return parse_string(res, str, offset);
			case 't':
			case 'f':
				return parse_bool(res, str, offset);
			case 'n':
				return parse_null(res, str, offset);
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				return parse_number(res, str, offset);
			default:
				return JsonParseStatus::JSON_PARSE_ERROR;
		}
	}

	void loads(JsonValue& res, const string& str)
	{
		size_t offset = 0;
		JsonParseStatus status = parse(res, str, offset);
		if (status == JsonParseStatus::JSON_PARSE_OK)
		{
			if (str[offset] == '\0')
				cout << "Load succeeded" << endl;
			else
			{
				res = JsonValue();
				cerr << "Load failed" << endl;
			}
		}
		else
		{
			res = JsonValue();
			cerr << "Load failed" << endl;
		}
	}
}
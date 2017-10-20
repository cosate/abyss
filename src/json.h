#pragma once
#include<iostream>
#include<map>
#include<vector>
#include<string>
#include<initializer_list>
#include<type_traits>
#include<cmath>
using namespace std;

namespace gao
{
	enum class Type { JSON_OBJECT = 0, JSON_ARRAY, JSON_STRING, JSON_INTEGRAL, JSON_FLOAT, JSON_BOOL, JSON_NULL };
	enum class JsonParseStatus { JSON_PARSE_ERROR = 0, JSON_PARSE_OK };
	enum class JsonStringifyStatus { JSON_STRINGIFY_OK = 0, JSON_STRINGIFY_ERROR };

	class JsonValue
	{
		friend ostream& operator<< (ostream&, JsonValue&);
	private:
		union Data
		{
			Data() : i(0) {}
			Data(double dou) : d(dou) {}
			Data(long long ll) : i(ll) {}
			Data(bool bo) : b(bo) {}
			Data(string ss) : s(new string(ss)) {}

			double d;
			long long i;
			bool b;
			string* s;
			map<string, JsonValue>* dict;
			vector<JsonValue>* arr;
		}data;
		Type type;
	public:
		JsonValue() :data(), type(Type::JSON_NULL) {}

		//构造函数模板
		template<class T>
		JsonValue(T b, typename enable_if<is_same<T, bool>::value>::type* = nullptr) : data(b), type(Type::JSON_BOOL) {}

		template<class T>
		JsonValue(T i, typename enable_if<is_integral<T>::value && !is_same<T, bool>::value>::type* = nullptr) : data((long long)i), type(Type::JSON_INTEGRAL) {}

		template<class T>
		JsonValue(T d, typename enable_if<is_floating_point<T>::value>::type* = nullptr) : data((double)d), type(Type::JSON_FLOAT) {}

		template<class T>
		JsonValue(T s, typename enable_if<is_convertible<T, string>::value>::type* = nullptr) : data(string(s)), type(Type::JSON_STRING) {}

		JsonValue(nullptr_t) : data(), type(Type::JSON_NULL) {}

		template<class T>
		JsonValue(T* t) = delete;

		JsonValue(const vector<JsonValue>& vec) {}

		JsonValue(const JsonValue* ptr) {}


		template<class T, class... Args>
		JsonValue(T t, Args... args) : type(Type::JSON_ARRAY)
		{
			data.arr = new vector<JsonValue>();
			data.arr->emplace_back(t);
			append(args...);
		}

		template<class T>
		void append(T t)
		{
			setType(Type::JSON_ARRAY);
			data.arr->emplace_back(t);
		}

		template<class T, class... Args>
		void append(T t, Args... args)
		{
			append(t);
			append(args...);
		}

		//初始化列表
		JsonValue(initializer_list<JsonValue> list) :type(Type::JSON_ARRAY)
		{
			data.arr = new vector<JsonValue>(list.size());
			size_t index = 0;
			for (auto it = list.begin(); it != list.end(); it++)
			{
				data.arr->operator[](index) = *it;
				index++;
			}
		}

		//复制构造函数和赋值操作符
		JsonValue(const JsonValue& other)
		{
			type = other.type;
			switch (type)
			{
				case Type::JSON_ARRAY:
					data.arr = new vector<JsonValue>(other.data.arr->begin(), other.data.arr->end());
					break;
				case Type::JSON_OBJECT:
					data.dict = new map<string, JsonValue>(other.data.dict->begin(), other.data.dict->end());
					break;
				case Type::JSON_STRING:
					data.s = new string(*(other.data.s));
					break;
				default:
					data = other.data;
			}
		}

		JsonValue(JsonValue&& other) : type(other.type), data(other.data)
		{
			other.type = Type::JSON_NULL;
			other.data.i = 0;
		}

		JsonValue& operator=(const JsonValue& other)
		{
			setType(other.type);
			switch (other.type)
			{
				case Type::JSON_ARRAY:
					data.arr = new vector<JsonValue>(other.data.arr->begin(), other.data.arr->end());
					break;
				case Type::JSON_OBJECT:
					data.dict = new map<string, JsonValue>(other.data.dict->begin(), other.data.dict->end());
					break;
				case Type::JSON_STRING:
					data.s = new string(*(other.data.s));
					break;
				default:
					data = other.data;
			}
			return *this;
		}

		JsonValue& operator=(JsonValue&& other)
		{
			setType(other.type);
			data = other.data;
			other.type = Type::JSON_NULL;
			other.data.i = 0;
			return *this;
		}

		template<class T>
		typename enable_if<is_same<T, bool>::value, JsonValue&>::type operator=(T boo)
		{
			setType(Type::JSON_BOOL);
			data.b = boo;
			return *this;
		}

		template<class T>
		typename enable_if<is_integral<T>::value, JsonValue&>::type operator=(T i)
		{
			setType(Type::JSON_INTEGRAL);
			data.i = (long long)i;
			return *this;
		}

		template<class T>
		typename enable_if<is_floating_point<T>::value, JsonValue&>::type operator=(T d)
		{
			setType(Type::JSON_FLOAT);
			data.d = (double)d;
			return *this;
		}

		template<class T>
		typename enable_if<is_convertible<T, string>::value, JsonValue&>::type operator=(T s)
		{
			setType(Type::JSON_STRING);
			data.s = new string(s);
			return *this;
		}

		JsonValue& operator=(nullptr_t)
		{
			setType(Type::JSON_NULL);
			return *this;
		}

		static JsonValue make(Type t)
		{
			JsonValue json;
			json.setType(t);
			return json;
		}

		//析构函数
		~JsonValue()
		{
			switch (type)
			{
				case Type::JSON_ARRAY:
					delete data.arr;
					break;
				case Type::JSON_OBJECT:
					delete data.dict;
					break;
				case Type::JSON_STRING:
					delete data.s;
					break;
				default:
					;
			}
		}

		//操作
		JsonValue& operator[](string s)
		{
			setType(Type::JSON_OBJECT);
			return ((this->data).dict)->operator[](s);
		}
		JsonValue& operator[](int index)
		{
			setType(Type::JSON_ARRAY);
			return ((this->data).arr)->operator[](index);
		}

		bool operator!=(const JsonValue& other)
		{
			return !(*this == other);
		}
		
		bool operator==(const JsonValue& other)
		{
			if (type != other.type)
				return false;
			else
			{
				switch (type)
				{
					case Type::JSON_NULL:
						return true;
					case Type::JSON_BOOL:
						return data.b == other.data.b;
					case Type::JSON_INTEGRAL:
						return data.i == other.data.i;
					case Type::JSON_FLOAT:
						return data.d == other.data.d;
					case Type::JSON_STRING:
						return *(data.s) == *(other.data.s);
					case Type::JSON_ARRAY:
					{
						if (data.arr->size() != other.data.arr->size())
							return false;
						else
						{
							auto i = data.arr->begin();
							auto j = other.data.arr->begin();
							for (; i != data.arr->end(), j != other.data.arr->end(); i++, j++)
							{
								if (*i != *j)
									return false;
							}
							return true;
						}
					}
					case Type::JSON_OBJECT:
					{
						if (data.arr->size() != other.data.arr->size())
							return false;
						else
						{
							auto i = data.arr->begin();
							auto j = other.data.arr->begin();
							for (; i != data.arr->end(), j != other.data.arr->end(); i++, j++)
							{
								if (*i != *j)
									return false;
							}
							return true;
						}
					}
				}
			}
		}

		string getString() const
		{
			if (type == Type::JSON_STRING)
				return*(data.s);
			else
				return "";
		}

		int getNumber() const
		{
			return (int)data.i;
		}

		bool getBoolean() const
		{
			return data.b;
		}

		Type JSONType() const
		{
			return type;
		}

		string dumps()
		{
			return dump(0);
		}
		
		int getSize()
		{
			if (type == Type::JSON_ARRAY)
				return data.arr->size();
			else if (type == Type::JSON_OBJECT)
				return data.dict->size();
			else
			{
				cerr << "not a container" << endl;
				return -1;
			}
		}
	private:
		void setType(Type t)
		{
			if (t == type)
				return;

			clearData();
			type = t;
			switch (t)
			{
				case Type::JSON_ARRAY:
					data.arr = new vector<JsonValue>();
					break;
				case Type::JSON_OBJECT:
					data.dict = new map<string, JsonValue>();
					break;
				case Type::JSON_STRING:
					data.s = new string();
					break;
				case Type::JSON_NULL:
					data.i = 0;
					break;
				case Type::JSON_INTEGRAL:
					data.i = 0;
					break;
				case Type::JSON_FLOAT:
					data.d = 0.0;
					break;
				case Type::JSON_BOOL:
					data.b = false;
					break;
				default:
					;
			}
		}

		void clearData()
		{
			switch (type)
			{
				case Type::JSON_ARRAY:
					delete data.arr;
					break;
				case Type::JSON_OBJECT:
					delete data.dict;
					break;
				case Type::JSON_STRING:
					delete data.s;
					break;
				default:
					;
			}
		}

		string dump(int level)
		{
			switch (type)
			{
				case Type::JSON_NULL:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					return res + "null";
				}
				case Type::JSON_BOOL:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					return data.b ? res + "true" : res + "false";
				}
				case Type::JSON_INTEGRAL:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					return res + to_string(data.i);
				}
				case Type::JSON_FLOAT:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					return res + to_string(data.d);
				}
				case Type::JSON_STRING:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					return res + "\"" + escape(*(data.s)) + "\"";
				}
				case Type::JSON_ARRAY:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					res += "[\n";
					for (auto it = data.arr->begin(); it != data.arr->end(); it++)
					{
						res += (*it).dump(level + 1);
						res += ",\n";
					}
					res.pop_back();
					res.pop_back();
					res += "\n";
					for (int i = 0; i < level; i++)
						res += "	";
					res += ']';
					return res;
				}
				case Type::JSON_OBJECT:
				{
					string res = "";
					for (int i = 0; i < level; i++)
						res += "	";
					res += "{\n";
					for (auto it = data.dict->begin(); it != data.dict->end(); it++)
					{
						for (int i = 0; i < level + 1; i++)
							res += "	";
						res += "\"";
						res += escape((*it).first);
						res += "\"";
						res += " : \n";
						res += (*it).second.dump(level + 1);
						res += ",\n\n";
					}
					res.pop_back();
					res.pop_back();
					res.pop_back();
					res += "\n";
					for (int i = 0; i < level; i++)
						res += "	";
					res += '}';
					return res;
				}
				default:
					return "";
			}
		}

		string escape(const string& str)
		{
			string res = "";
			size_t index = 0;
			while (str[index] != '\0')
			{
				switch (str[index])
				{
					case '\"': res += "\\\""; break;
					case '\\': res += "\\\\"; break;
					case '\b': res += "\\b";  break;
					case '\f': res += "\\f";  break;
					case '\n': res += "\\n";  break;
					case '\r': res += "\\r";  break;
					case '\t': res += "\\t";  break;
					default: res += str[index]; break;
				}
				++index;
			}
			return res;
		}
	};

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
	
	inline bool iswhitespace(char c)
	{
		return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
	}

	inline bool isdigit(char c)
	{
		return (c >= '0' && c <= '9');
	}

	inline void pass_whitespace(const string& str, size_t& offset)
	{
		while (iswhitespace(str[offset]))
			++offset;
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
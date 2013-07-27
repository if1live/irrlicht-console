// Ŭnicode please 
#pragma once

class StringUtil {
public:
	//http://hook.tistory.com/140
	static std::string wstring2string(const std::wstring &wstr)
	{
		std::string str(wstr.length(),' ');
		std::copy(wstr.begin(), wstr.end(), str.begin());
		return str;
	}

	static std::wstring string2wstring(const std::string &str)
	{
		std::wstring wstr(str.length(),L' ');
		std::copy(str.begin(), str.end(), wstr.begin());
		return wstr;
	}
};

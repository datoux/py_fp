/*
Copyright (c) 2023 Daniel Turecek <daniel@turecek.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef STRUTILS_H
#define STRUTILS_H
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace str
{

    /// Return true if string str containes a pattern sub string. false if not.
    inline bool contains(const std::string_view str, const std::string_view pattern)
    {
        return str.find(pattern) != std::string::npos;
    }

    /// Return true if string str starts with substring pattern.
    inline bool starts_with(const std::string_view str, const std::string_view pattern)
    {
        return pattern.size() <= str.size() && str.compare(0, pattern.size(), pattern) == 0;
    }

    /// Return true if string str ends with substring pattern
    inline bool ends_with(const std::string_view str, const std::string_view pattern)
    {
        return pattern.size() <= str.size() && str.compare(str.size() - pattern.size(), pattern.size(), pattern) == 0;
    }

    /// Return substring based on index from (inclusive) and to (exclusive).
    /// If any of the indexes is negative, the index is counted from the end of string (inclusive).
    /// index==-1 -> last char of string, index==-2, second to last, etc.
    /// range(0, -1) => the whole string.
    inline std::string range(const std::string str, int from, int to = -1)
    {
        if (from < 0)
            from = static_cast<int>(str.size() + from);

        if (to < 0)
            to = static_cast<int>(str.size() - from + to + 1);
        else
            to = to - from;

        return str.substr(from, to);
    }

    /// Strip all characters (by default space) from the begininng of the string
    inline std::string lstrip(std::string str, std::string_view strip_chars=" ")
    {
        return str.erase(0, str.find_first_not_of(strip_chars));
    }

    /// Strip all characters (by default space) from the end of the string
    inline std::string rstrip(std::string str, std::string_view strip_chars=" ")
    {
        return str.erase(str.find_last_not_of(strip_chars) + 1);
    }

    /// Strip all characters (by default space) from begininng and end of the string
    inline std::string strip(std::string str, std::string_view strip_chars=" ")
    {
        auto res = str.erase(0, str.find_first_not_of(strip_chars));
        return res.erase(str.find_last_not_of(strip_chars) + 1);
    }

    /// Convert string to upper case
    inline std::string to_upper(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    /// Convert string to lower case
    inline std::string to_lower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    /// Compare two strings case non sensitive
    inline bool iequals(std::string s1, std::string s2) {
        return to_lower(s1).compare(to_lower(s2)) == 0;
    }

    /// Replace all occurences of search substring in the str string with replace string
    inline void replace_all(std::string& str, std::string_view search, std::string_view replace)
    {
        if (search.empty())
            return;

        size_t pos = str.find(search);
        while(pos != std::string::npos) {
            str.replace(pos, search.size(), replace);
            pos = str.find(search, pos + replace.size());
        }
    }

    /// Remove all non-alpha-numeric characters from the string
    inline std::string remove_non_alnum_chars(std::string_view text)
    {
        std::string result;
        std::copy_if(text.begin(), text.end(), std::back_inserter(result), (int(*)(int))std::isalnum);
        return result;
    }

    /// Convert a integer/float number to string
    template <typename T> inline std::string to_string(const T& value)
    {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    /// Convert double number to string with specified precision
    inline std::string to_string(double value, int precision)
    {
        std::ostringstream os;
        os.precision(precision);
        return (!(os << std::fixed << value)) ? "" : os.str();
    }

    /// Convert array of numbers into string separated by space
    template <typename T> std::string array_to_string(T* buff, size_t size)
    {
        std::string output;
        for (size_t i = 0; i < size; i++) {
            output += to_string(buff[i]);
            if (i != size - 1)
                output += " ";
        }
        return output;
    }

    /// Convert array of numbers into string separated by space
    template<> inline std::string array_to_string<unsigned char>(unsigned char* buff, size_t size)
    {
        std::string output;
        for (size_t i = 0; i < size; i++) {
            output += to_string(static_cast<unsigned int>(buff[i]));
            if (i != size - 1)
                output += " ";
        }
        return output;
    }

    /// Convert array of numbers into string separated by space
    template<> inline std::string array_to_string<char>(char* buff, size_t size)
    {
        std::string output;
        for (size_t i = 0; i < size; i++) {
            output += to_string(static_cast<int>(buff[i]));
            if (i != size - 1)
                output += " ";
        }
        return output;
    }

    /// Convert string to a number. optionally err_code argument will contain error, if it occurs.
    template <typename T> inline T to_num(const std::string& str, int* err_code=nullptr)
    {
        T value;
        std::stringstream ss(str);
        int errorCode = (!(ss >> value)) ? 1 : 0;
        if (err_code)
            *err_code = errorCode;
        return value;
    }

    /// Convert string to a number with default value def_val if not possible to convert.
    template <typename T> inline T to_num_def(const std::string& str, T def_val)
    {
        T value;
        std::stringstream ss(str);
        return (!(ss >> value)) ? def_val : value;
    }

    /// Convert string to integer. Optinally err_code can contain the error if if occurs.
    inline int to_int(std::string str, int* err_code=nullptr)
    {
        return to_num<int>(str, err_code);
    }

    /// Convert string to double. Optinally err_code can contain the error if if occurs.
    inline double to_double(std::string str, int* err_code=nullptr)
    {
        return to_num<double>(str, err_code);
    }

    /// Convert string to integer with default value def_val if not possible to convert.
    inline int to_int_def(std::string str, int def_val)
    {
        return to_num_def<int>(str, def_val);
    }

    /// Convert string to double with default value def_val if not possible to convert.
    inline double to_double_def(std::string str, double def_val)
    {
        return to_num_def<double>(str, def_val);
    }

    /// Convert a number to its hex representation string.
    template <typename T> inline std::string to_hex_string(const T& value)
    {
        const char* hex_chars = "0123456789ABCDEF";
        std::string result;
        for (int i = sizeof(value) * 8 - 4; i >= 0; i -= 4)
            result += hex_chars[((value >> i) & 0xF)];
        return result;
    }

    /// Convert array of chars/bytes to hex string.
    inline std::string to_hex_string(char* buff, size_t size)
    {
        const char* hex_chars = "0123456789ABCDEF";
        std::string result;
        for (size_t i = 0; i < size; i++) {
            result += hex_chars[(buff[i] >> 4) & 0xF];
            result += hex_chars[buff[i] & 0xF];
        }
        return result;
    }

    /// Convert hex string to a value. Optinally err_code will contain error, if it occurs.
    template <typename T> inline T hex_string_to_value(const std::string_view str, int* err_code=nullptr)
    {
        std::stringstream ss;
        ss << std::hex << str;
        unsigned long x = 0;
        int errorCode = (!(ss >> x)) ? 1 : 0;
        if (err_code)
            *err_code = errorCode;
        return static_cast<T>(x);
    }

    /// Format string with variadic number of arguments, simirally to printf
    template<typename... Args> std::string format(const char* fmt, Args... args)
    {
        size_t size = snprintf(nullptr, 0, fmt, args...);
        std::string buff;
        buff.reserve(size + 1);
        buff.resize(size);
        snprintf(&buff[0], size + 1, fmt, args...);
        return buff;
    }

    // Format string with va_list args, simirally to printf
    inline std::string formatv(const std::string fmt, va_list args)
    {
        std::string str;
        int size = 500;

        while (true) {
            str.resize(size);

            #ifdef _MSC_VER
                int n = _vsnprintf_s((char*)str.c_str(), size, size - 1, fmt.c_str(), args);
            #else
                int n = vsnprintf((char*)str.c_str(), size, fmt.c_str(), args);
            #endif

            if (n > -1 && n < size)
                return str;

            size = (n > -1) ? n + 1 : 2 * size;
        }
    }

    /// Splits a string by delimiter. It is possible to specify, if empty items are skiped and what is the maximum number of items.
    inline std::vector<std::string> split(const std::string str, const std::string_view delim, bool skip_empty=false, size_t max_items=0)
    {
        size_t start = 0;
        size_t end = 0;
        std::vector<std::string> items;

        if (max_items == 1) {
            items.push_back(str);
            return items;
        }

        while (end != std::string::npos && end != str.size())
        {
            end = str.find(delim, start);

            if (end == std::string::npos)
                end = str.size();

            std::string item = str.substr(start, end - start);

            if (!skip_empty || !item.empty())
                items.push_back(item);

            start = (end > str.size() - delim.size()) ? std::string::npos : end + delim.size();

            if (max_items != 0 && items.size() + 1 == max_items && end != std::string::npos) {
                item = str.substr(start, std::string::npos);
                items.push_back(item);
                return items;
            }
        }

        return items;
    }

    /// Splits a string by one of the delimiter characters.
    /// It is possible to specify, if empty items are skiped and what is the maximum number of items.
    inline std::vector<std::string> split_delims(const std::string str, const std::string_view delims, bool skip_empty=false, size_t max_items=0)
    {
        size_t start = 0;
        size_t end = 0;
        std::vector<std::string> items;

        if (max_items == 1) {
            items.push_back(str);
            return items;
        }

        while (end != std::string::npos && end != str.size())
        {
            end = str.find_first_of(delims, start);

            if (end == std::string::npos)
                end = str.size();

            std::string item = str.substr(start, end - start);

            if (!skip_empty || !item.empty())
                items.push_back(item);

            start = (end > str.size() - 1) ? std::string::npos : end + 1;

            if (max_items != 0 && items.size() + 1 == max_items && end != std::string::npos) {
                item = str.substr(start, std::string::npos);
                items.push_back(item);
                return items;
            }
        }

        return items;
    }

}


#endif /* end of include guard: STRUTILS_H */


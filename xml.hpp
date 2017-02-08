#pragma once
#include <string.h>
#include <algorithm>
#include <functional>
#include <cctype>
#include "reflection.hpp"
#include "traits.hpp"
#include "itoa.hpp"

namespace iguana {
	namespace xml {
		//to xml
		template<typename Stream, typename T>
		std::enable_if_t<!std::is_floating_point<T>::value && (std::is_integral<T>::value || std::is_unsigned<T>::value || std::is_signed<T>::value)>
			render_xml_value(Stream& ss, T value)
		{
			char temp[20];
			auto p = itoa_fwd(value, temp);
			ss.write(temp, p - temp);
		}

		template<typename Stream, typename T>
		std::enable_if_t<std::is_floating_point<T>::value> render_xml_value(Stream& ss, T value)
		{
			char temp[20];
			sprintf(temp, "%f", value);
			ss.write(temp);
		}

		template<typename Stream>
		void render_xml_value(Stream& ss, const std::string &s)
		{
			ss.write(s.c_str(), s.size());
		}

		template<typename Stream>
		void render_xml_value(Stream& ss, const char* s)
		{
			ss.write(s, strlen(s));
		}

		template<typename Stream, typename T>
		std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream& ss, T t) {
			ss.put('<');
			render_xml_value(ss, t);
			ss.put('>');
		}

		template<typename Stream>
		void render_key(Stream& ss, const std::string &s) {
			render_xml_value(ss, s);
		}

		template<typename Stream>
		void render_key(Stream& ss, const char* s) {
			render_xml_value(ss, s);
		}

		template<typename Stream>
		void render_tail(Stream& ss, const char* s)
		{
			ss.put('<');
			ss.put('/');
			ss.write(s, strlen(s));
			ss.put('>');
		}

		template<typename Stream>
		void render_head(Stream& ss, const char* s)
		{
			ss.put('<');
			ss.write(s, strlen(s));
			ss.put('>');
		}

		template<typename Stream, typename T, typename = std::enable_if_t<is_reflection<T>::value>>
		void to_xml_impl(Stream& s, T &&t) {
			for_each(std::forward<T>(t), [&s](const auto& v, size_t I, bool is_last) { //magic for_each struct std::forward<T>(t)
				render_head(s, get_name<T>(I));
				render_xml_value(s, v);
				render_tail(s, get_name<T>(I));
			}, [&s](const auto& o, size_t I, bool is_last)
			{
				render_head(s, get_name<T>(I));
				to_xml_impl(s, o);
				render_tail(s, get_name<T>(I));
			});
		}

		template<typename Stream, typename T, typename = std::enable_if_t<is_reflection<T>::value>>
		void to_xml(Stream& s, T &&t) {
			render_head(s, "xml");

			to_xml_impl(s, std::forward<T>(t));

			render_tail(s, "xml"/*get_name<T>()*/);
		}

		//template<typename T, typename = std::enable_if_t<is_reflection<T>::value>>
		//void do_read(reader_t &rd, T &&t) {

		//}

		//template<typename T, typename = std::enable_if_t<is_reflection<T>::value>>
		//void from_xml(T &&t, const char *buf, size_t len = -1) {
		//	xml_reader_t rd(buf, len);
		//	do_read(rd, t);
		//}
	}
}




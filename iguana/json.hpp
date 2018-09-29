//
// Created by Qiyu on 17-6-5.
//

#ifndef SERIALIZE_JSON_HPP
#define SERIALIZE_JSON_HPP
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <iomanip>
#include <map>
#include <vector>
#include <array>
#include <type_traits>
#include <functional>
#include <string_view>
//#include "reflection.hpp" //test c++17 version
#include "detail/itoa.hpp"
#include "detail/traits.hpp"
#include "detail/string_stream.hpp"
#include "reflection_new.hpp"

namespace iguana { namespace json
{
        template<typename InputIt, typename T, typename F>
        T join(InputIt first, InputIt last, const T &delim, const F &f) {
            if (first == last)
                return T();

            T t = f(*first++);
            while (first != last) {
                t += delim;
                t += f(*first++);
            }
            return t;
        }

        template<typename Stream, typename InputIt, typename T, typename F>
        void join(Stream& ss, InputIt first, InputIt last, const T &delim, const F &f) {
            if (first == last)
                return;

            f(*first++);
            while (first != last) {
                ss.put(delim);
                f(*first++);
            }
        }

        template<typename Stream>
        void render_json_value(Stream& ss, std::nullptr_t) { ss.write("null"); }

        template<typename Stream>
        void render_json_value(Stream& ss, bool b) { ss.write(b ? "true" : "false"); };

        template<typename Stream, typename T>
        std::enable_if_t<!std::is_floating_point<T>::value&&(std::is_integral<T>::value || std::is_unsigned<T>::value || std::is_signed<T>::value)>
        render_json_value(Stream& ss, T value)
        {
            char temp[20];
            auto p = itoa_fwd(value, temp);
            ss.write(temp, p-temp);
        }

        template<typename Stream>
        void render_json_value(Stream& ss, int64_t value)
        {
            char temp[65];
            auto p = xtoa(value, temp, 10, 1);
            ss.write(temp, p - temp);
        }

        template<typename Stream>
        void render_json_value(Stream& ss, uint64_t value)
        {
            char temp[65];
            auto p = xtoa(value, temp, 10, 0);
            ss.write(temp, p - temp);
        }

        template<typename Stream, typename T>
        std::enable_if_t<std::is_floating_point<T>::value> render_json_value(Stream& ss, T value)
        {
            char temp[20];
            sprintf(temp, "%f", value);
            ss.write(temp);
        }

        template<typename Stream>
        void render_json_value(Stream& ss, const std::string &s)
        {
            ss.write_str(s.c_str(), s.size());
        }

        template<typename Stream>
        void render_json_value(Stream& ss, const char* s, size_t size)
        {
            ss.write_str(s, size);
        }

        template<typename Stream, typename T>
        std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream& ss, T t) {
            ss.put('"');
            render_json_value(ss, t);
            ss.put('"');
        }

        template<typename Stream>
        void render_key(Stream& ss, const std::string &s) {
            render_json_value(ss, s);
        }

        template<typename Stream, typename T>
        constexpr auto to_json(Stream& ss, T &&t) -> std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>;

        template<typename Stream, typename T>
        auto render_json_value(Stream& ss, T &&t) -> std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>
        {
            to_json(ss, std::forward<T>(t));
        }

        template<typename Stream, typename T>
        std::enable_if_t <std::is_enum<T>::value> render_json_value(Stream& ss, T val)
        {
            render_json_value(ss, (std::underlying_type_t<T>&)val);
        }

        template <typename Stream, typename T, size_t N>
        void render_json_value(Stream& ss, const T(&v)[N])
        {
            render_array(ss, v);
        }

        template<typename Stream, typename T>
        void render_array(Stream& ss, const T& v)
        {
            ss.put('[');
            join(ss, std::begin(v), std::end(v), ',',
                 [&ss](const auto &jsv) {
                     render_json_value(ss, jsv);
                 });
            ss.put(']');
        }

        template <typename Stream, typename T, size_t N>
        void render_json_value(Stream& ss, const std::array<T, N>& v)
        {
            render_array(ss, v);
        }

        template<typename Stream, typename T>
        std::enable_if_t <is_associat_container<T>::value>
        render_json_value(Stream& ss, const T&o) {
            ss.put('{');
            join(ss, o.cbegin(), o.cend(), ',',
                 [&ss](const auto &jsv) {
                     render_key(ss, jsv.first);
                     ss.put(':');
                     render_json_value(ss, jsv.second);
                 });
            ss.put('}');
        }

        template<typename Stream, typename T>
        std::enable_if_t <is_sequence_container<T>::value> render_json_value(Stream& ss, const T &v) {
            ss.put('[');
            join(ss, v.cbegin(), v.cend(), ',',
                 [&ss](const auto &jsv) {
                     render_json_value(ss, jsv);
                 });
            ss.put(']');
        }

        /*constexpr auto write_json_key = [](auto& s, auto i, auto& t){
            s.put('"');
            auto name = get_name<decltype(t), decltype(i)::value>(); //will be replaced by string_view later
            s.write(name.data(), name.length());
            s.put('"');
        };*/

		template<typename Stream, typename T>
		constexpr auto to_json(Stream& s, T&& v)->std::enable_if_t<is_sequence_container<std::decay_t<T>>::value>
		{
			using U = typename std::decay_t<T>::value_type;
			s.put('[');
			const size_t size = v.size();
			for (size_t i = 0; i < size; i++)
			{
				if constexpr(is_reflected_v<U>) {
					to_json(s, v[i]);
				}
				else {
					render_json_value(s, v[i]);
				}
				
				if(i!= size-1)
					s.put(',');
			}
			s.put(']');
		}

		template<typename Stream, typename T>
		constexpr auto to_json(Stream& s, T&& t)->std::enable_if_t<is_tuple<std::decay_t<T>>::value> {
			using U = typename std::decay_t<T>;
			s.put('[');
			const size_t size = std::tuple_size_v<U>;
			for_each(std::forward<T>(t), [&s, size](auto& v, auto i) {
				render_json_value(s, v);

				if (i != size - 1)
					s.put(',');
			});
			s.put(']');
		}

        // forward
        template<typename Stream, typename T>
        constexpr auto to_json(Stream& s, T &&t)->std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>;

        template <typename Stream, typename T>
        struct write_visitor_t
        {
            write_visitor_t(Stream& s, T const& t) : s_(s), t_(t) {}

            template <typename PMD, typename Arr>
            void visit_mdata(std::string_view name, PMD pmd, size_t index, Arr names)
            {
                s_.write(name.data(), name.length());
                s_.put(':');

                using element_type = std::remove_reference_t<decltype(t_.*pmd)>;
                if constexpr (is_reflected_v<element_type>)
                {
                    to_json(s_, t_.*pmd);
                }
                else
                {
                    render_json_value(s_, t_.*pmd);
                }

                if (index < names.size() - 1)
                    s_.put(',');
            }

        private:
            Stream& s_;
            T const& t_;
        };

        template<typename Stream, typename T>
        constexpr auto to_json(Stream& s, T &&t) -> std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>
        {
            s.put('{');
            reflect_visit(reflexpr(t) {}, write_visitor_t{ s, t });
            s.put('}');
        }

        ///***********************************  from json*********************************///
        //reader from ajson
        namespace detail
        {
            struct string_ref {
                char const *str;
                size_t len;
				bool operator == (string_ref const& rhs) const
				{
					if (len == rhs.len)
					{
						return std::memcmp(str, rhs.str, len) == 0;
					}
					return false;
				}

				bool operator != (std::string_view rhs) const
				{
					if (len == rhs.length())
					{
						return std::memcmp(str, rhs.data(), len) != 0;
					}
					return true;
				}
            };
        }

        struct token {
            detail::string_ref str;
            enum {
                t_string,
                t_int,
                t_uint,
                t_number,
                t_ctrl,
                t_end,
            } type;
            union {
                int64_t i64;
                uint64_t u64;
                double d64;
            } value;
            bool neg = false;
        };

        static bool g_has_error = false;
        class reader_t {
        public:
            reader_t(const char *ptr = nullptr, size_t len = -1) : ptr_((char *)ptr), len_(len) {
                if (ptr == nullptr) {
                    end_mark_ = true;
                }
                else if (len == 0) {
                    end_mark_ = true;
                }
                else if (ptr[0] == 0) {
                    end_mark_ = true;
                }
                next();
            }

            static inline char *itoa_native(size_t val, char *buffer, size_t len) {
                buffer[len] = 0;
                size_t pos = len - 1;
                if (val == 0) {
                    buffer[pos--] = '0';
                }
                while (val) {
                    buffer[pos--] = (char)((val % 10) + '0');
                    val = val / 10;
                }
                ++pos;
                return &buffer[0] + pos;
            }

            inline void error(const char *message)  {
                g_has_error = true;
//                char buffer[20];
//                std::string msg = "error at line :";
//                msg += itoa_native(cur_line_, buffer, 19);
//                msg += " col :";
//                msg += itoa_native(cur_col_, buffer, 19);
//                msg += " msg:";
//                msg += message;
//                throw std::invalid_argument(msg);
            }

            inline token const &peek() const {
                return cur_tok_;
            }

            void next()
            {
                auto c = skip();
                bool do_next = false;
                cur_tok_.neg = false;
                switch (c)
                {
                    case 0:
                        cur_tok_.type = token::t_end;
                        cur_tok_.str.str = ptr_ + cur_offset_;
                        cur_tok_.str.len = 1;
                        break;
                    case '{':
                    case '}':
                    case '[':
                    case ']':
                    case ':':
                    case ',':
                    {
                        cur_tok_.type = token::t_ctrl;
                        cur_tok_.str.str = ptr_ + cur_offset_;
                        cur_tok_.str.len = 1;
                        take();
                        break;
                    }
                    case '/':
                    {
                        take();
                        c = read();
                        if (c == '/')
                        {
                            take();
                            c = read();
                            while (c != '\n' && c != 0)
                            {
                                take();
                                c = read();
                            }
                            do_next = true;
                            break;
                        }
                        else if (c == '*')
                        {
                            take();
                            c = read();
                            do
                            {
                                while (c != '*')
                                {
                                    if (c == 0)
                                    {
                                        return;
                                    }
                                    take();
                                    c = read();
                                }
                                take();
                                c = read();
                                if (c == '/')
                                {
                                    take();
                                    do_next = true;
                                    break;
                                }
                            } while (true);
                        }
                        //error parser comment
                        error("not a comment!");
                    }
                    case '"':
                    {
                        cur_tok_.type = token::t_string;
                        parser_quote_string();
                        break;
                    }
                    default:
                    {
                        if (c >= '0' && c <= '9')
                        {
                            cur_tok_.type = token::t_uint;
                            cur_tok_.value.u64 = c - '0';
                            parser_number();
                        }
                        else if (c == '-')
                        {
                            cur_tok_.type = token::t_int;
                            cur_tok_.value.i64 = 0;
                            cur_tok_.neg = true;
                            parser_number();
                        }
                        else
                        {
                            cur_tok_.type = token::t_string;
                            parser_string();
                        }
                    }
                }
                if (do_next == false)
                    return;
                next();
            }

            inline bool expect(char c) {
                return cur_tok_.str.str[0] == c;
            }

        private:
            inline void decimal_reset() { decimal = 0.1; }

            inline char read() const {
                if (end_mark_)
                    return 0;
                return ptr_[cur_offset_];
            }

            inline void take() {
                if (end_mark_ == false) {
                    ++cur_offset_;
                    char v = ptr_[cur_offset_];
                    if (v != '\r')
                        ++cur_col_;
                    if (len_ > 0 && cur_offset_ >= len_) {
                        end_mark_ = true;
                    }
                    else if (v == 0) {
                        end_mark_ = true;
                    }
                    if (v == '\n') {
                        cur_col_ = 0;
                        ++cur_line_;
                    }
                }
            }

            char skip() {
                auto c = read();
                while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                    take();
                    c = read();
                }
                return c;
            }

            inline void fill_escape_char(size_t count, char c) {
				if (count == 0)
					return;
				ptr_[cur_offset_ - count] = c;
            }

            char table[103] = {
                    16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 16, 17, 17, 16,
                    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                    16, 16, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                    16, 16, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 16, 16, 16, 16,
                    16, 16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16,
                    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                    16, 16, 10, 11, 12, 13, 14, 15 };

            inline char char_to_hex(char v) {
                if (v < 'f') {
                    v = table[v];
                }
                else {
                    v = 16;
                }
                if (v > 15)
                    error("utf8 code error!");
                return v;
            }

            inline uint64_t read_utf() {
                char v = char_to_hex(read());
                uint64_t utf = v;
                utf <<= 4;
                take();
                v = char_to_hex(read());
                utf += v;
                utf <<= 4;
                take();
                v = char_to_hex(read());
                utf += v;
                utf <<= 4;
                take();
                v = char_to_hex(read());
                utf += v;
                take();
                return utf;
            }

            inline void esacpe_utf8(size_t &esc_count) {
                uint64_t utf1 = read_utf();
                esc_count += 6;

                if (utf1 < 0x80) {
                    fill_escape_char(esc_count, (char)utf1);
                }
                else if (utf1 < 0x800) {
                    fill_escape_char(esc_count, (char)(0xC0 | ((utf1 >> 6) & 0xFF)));
                    fill_escape_char(esc_count - 1, (char)(0x80 | ((utf1 & 0x3F))));
                }
                else if (utf1 < 0x80000) {
                    fill_escape_char(esc_count, (char)(0xE0 | ((utf1 >> 12) & 0xFF)));
                    fill_escape_char(esc_count - 1, (char)(0x80 | ((utf1 >> 6) & 0x3F)));
                    fill_escape_char(esc_count - 2, (char)(0x80 | ((utf1 & 0x3F))));
                }
                else {
                    if (utf1 < 0x110000) {
                        error("utf8 code error!");
                    }
                    fill_escape_char(esc_count, (char)(0xF0 | ((utf1 >> 18) & 0xFF)));
                    fill_escape_char(esc_count - 1, (char)(0x80 | ((utf1 >> 12) & 0x3F)));
                    fill_escape_char(esc_count - 2, (char)(0x80 | ((utf1 >> 6) & 0x3F)));
                    fill_escape_char(esc_count - 3, (char)(0x80 | ((utf1 & 0x3F))));
                }
            }

			void parser_quote_string()
			{
				take();
				cur_tok_.str.str = ptr_ + cur_offset_;
				auto c = read();
				size_t esc_count = 0;
				do
				{
					switch (c)
					{
					case 0:
					case '\n':
					{
						error("not a valid quote string!");
						return;
					}
					case '\\':
					{
						take();
						c = read();
						switch (c)
						{
						case '/':
						{
							c = '/';
							break;
						}
						case 'b':
						{
							c = '\b';
							break;
						}
						case 'f':
						{
							c = '\f';
							break;
						}
						case 'n':
						{
							c = '\n';
							break;
						}
						case 'r':
						{
							c = '\r';
							break;
						}
						case 't':
						{
							c = '\t';
							break;
						}
						case '"':
						{
							break;
						}
						case 'u':
						{
							take();
							esacpe_utf8(esc_count);
							c = read();
							continue;
						}
						default:
						{
							error("unknown escape char!");
							return;
						}
						}
						++esc_count;
						break;
					}
					case '"':
					{
						cur_tok_.str.len = ptr_ + cur_offset_ - esc_count - cur_tok_.str.str;
						take();
						return;
					}
					}
					fill_escape_char(esc_count, c);
					take();
					c = read();
				} while (true);
			}

			void parser_string()
			{
				cur_tok_.str.str = ptr_ + cur_offset_;
				take();
				auto c = read();
				size_t esc_count = 0;
				do
				{
					switch (c)
					{
					case 0:
					{
						error("not a valid string!");
                        return;
					}
					case '\\':
					{
						take();
						c = read();
						switch (c)
						{
						case 'b':
						{
							c = '\b';
							break;
						}
						case 'f':
						{
							c = '\f';
							break;
						}
						case 'n':
						{
							c = '\n';
							break;
						}
						case 'r':
						{
							c = '\r';
							break;
						}
						case 't':
						{
							c = '\t';
							break;
						}
						case 'u':
						{
							take();
							esacpe_utf8(esc_count);
							continue;
						}
						default:
						{
							error("unknown escape char!");
						}
						}
						++esc_count;
						break;
					}
					case ' ':
					case '\t':
					case '\r':
					case '\n':
					case ',':
					case '[':
					case ']':
					case ':':
					case '{':
					case '}':
					{
						cur_tok_.str.len = ptr_ + cur_offset_ - esc_count - cur_tok_.str.str;
						return;
					}
					}
					fill_escape_char(esc_count, c);
					take();
					c = read();
				} while (true);
			}

            void parser_number() {
                cur_tok_.str.str = ptr_ + cur_offset_;
                take();
                auto c = read();
                do {
                    switch (c) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9': {
                            if (cur_tok_.type == token::t_int) {
                                cur_tok_.value.i64 *= 10;
                                cur_tok_.value.i64 += c - '0';
                            }
                            else if (cur_tok_.type == token::t_uint) {
                                cur_tok_.value.u64 *= 10;
                                cur_tok_.value.u64 += c - '0';
                            }
                            else if (cur_tok_.type == token::t_number) {
                                cur_tok_.value.d64 += decimal * (c - '0');
                                decimal *= 0.1;
                            }
                            break;
                        }
                        case '.': {
                            if (cur_tok_.type == token::t_int) {
                                cur_tok_.type = token::t_number;
                                cur_tok_.value.d64 = (double)cur_tok_.value.i64;
                                decimal_reset();
                            }
                            else if (cur_tok_.type == token::t_uint) {
                                cur_tok_.type = token::t_number;
                                cur_tok_.value.d64 = (double)cur_tok_.value.u64;
                                decimal_reset();
                            }
                            else if (cur_tok_.type == token::t_number) {
                                error("not a valid number!");
                            }
                            break;
                        }
                        default: {
                            cur_tok_.str.len = ptr_ + cur_offset_ - cur_tok_.str.str;
                            return;
                        }
                    }
                    take();
                    c = read();
                } while (1);
            }

            token cur_tok_;
            size_t cur_col_ = 0;
            size_t cur_line_ = 0;
            size_t len_ = 0;
            size_t cur_offset_ = 0;
            bool end_mark_ = false;
            char *ptr_;
            double decimal = 0.1;
        };

        void skip(reader_t &rd);

        inline void skip_array(reader_t &rd) {
            auto tok = &rd.peek();
            while (tok->str.str[0] != ']') {
                skip(rd);
                tok = &rd.peek();
                if (tok->str.str[0] == ',') {
                    rd.next();
                    tok = &rd.peek();
                    continue;
                }
            }
            rd.next();
        }

        inline void skip_key(reader_t &rd) {
            auto &tok = rd.peek();
            switch (tok.type) {
                case token::t_string:
                case token::t_int:
                case token::t_uint:
                case token::t_number: {
                    rd.next();
                    return;
                }
                default:
                    break;
            }
            rd.error("invalid json document!");
        }

        inline void skip_object(reader_t &rd) {
            auto tok = &rd.peek();
            while (tok->str.str[0] != '}') {
                skip_key(rd);
                tok = &rd.peek();
                if (tok->str.str[0] == ':') {
                    rd.next();
                    skip(rd);
                    tok = &rd.peek();
                }
                else {
                    rd.error("invalid json document!");
                }
                if (tok->str.str[0] == ',') {
                    rd.next();
                    tok = &rd.peek();
                    continue;
                }
            }
            rd.next();
        }

        inline void skip(reader_t &rd) {
            auto &tok = rd.peek();
            switch (tok.type) {
                case token::t_string:
                case token::t_int:
                case token::t_uint:
                case token::t_number: {
                    rd.next();
                    return;
                }
                case token::t_ctrl: {
                    if (tok.str.str[0] == '[') {
                        rd.next();
                        skip_array(rd);
                        return;
                    }
                    else if (tok.str.str[0] == '{') {
                        rd.next();
                        skip_object(rd);
                        return;
                    }
                }
                case token::t_end: {
                    return;
                }
            }
            rd.error("invalid json document!");
        }

        template<typename T>
        void check_result(T val, char const *str){
            if(val == 0 && strcmp(str,"0") != 0){
                g_has_error = true;
            }
        }

        //read json to value
        template<typename T>
        inline std::enable_if_t<is_signed_intergral_like<T>::value> read_json(reader_t &rd, T &val) {
            auto &tok = rd.peek();
            switch (tok.type) {
                case token::t_string: {
                    int64_t temp = std::strtoll(tok.str.str, nullptr, 10);
                    val = static_cast<T>(temp);
                    check_result(val, tok.str.str);
                    break;
                }
                case token::t_int: {
                    val = static_cast<T>(tok.value.i64);
                    if (tok.neg)
                        val = -val;
                    break;
                }
                case token::t_uint: {
                    val = static_cast<T>(tok.value.u64);
                    break;
                }
                case token::t_number: {
                    val = static_cast<T>(tok.value.d64);
                    if (tok.neg)
                        val = -val;
                    break;
                }
                default: {
                    rd.error("not a valid signed integral like number.");
                }
            }
            rd.next();
        }

        template<typename T>
        inline std::enable_if_t<is_unsigned_intergral_like<T>::value> read_json(reader_t &rd, T &val) {
            auto &tok = rd.peek();
            switch (tok.type) {
                case token::t_string: {
                    uint64_t temp = std::strtoull(tok.str.str, nullptr, 10);
                    val = static_cast<T>(temp);
                    check_result(val, tok.str.str);
                    break;
                }
                case token::t_int: {
                    if (tok.value.i64 < 0) {
                        rd.error("assign a negative signed integral to unsigned integral number.");
                    }
                    val = static_cast<T>(tok.value.i64);
                    break;
                }
                case token::t_uint: {
                    val = static_cast<T>(tok.value.u64);
                    break;
                }
                case token::t_number: {
                    if (tok.value.d64 < 0) {
                        rd.error("assign a negative float point to unsigned integral number.");
                    }
                    val = static_cast<T>(tok.value.d64);
                    break;
                }
                default: {
                    rd.error("not a valid unsigned integral like number.");
                }
            }
            rd.next();
        }

        template<typename T>
        inline std::enable_if_t<std::is_enum<T>::value> read_json(reader_t &rd, T &val) {
            typedef typename std::underlying_type<T>::type RAW_TYPE;
            read_json(rd, (RAW_TYPE&)val);
        }

        template<typename T>
        inline std::enable_if_t<std::is_floating_point<T>::value> read_json(reader_t &rd, T &val)
        {
            auto& tok = rd.peek();
            switch (tok.type)
            {
                case token::t_string:
                {
                    double temp = std::strtold(tok.str.str, nullptr);
                    val = static_cast<T>(temp);
                    check_result(val, tok.str.str);
                    break;
                }
                case token::t_int:
                {
                    val = static_cast<T>(tok.value.i64);
                    if (tok.neg)
                        val = -val;
                    break;
                }
                case token::t_uint:
                {
                    val = static_cast<T>(tok.value.u64);
                    break;
                }
                case token::t_number:
                {
                    val = static_cast<T>(tok.value.d64);
                    if (tok.neg)
                        val = -val;
                    break;
                }
                default:
                {
                    rd.error("not a valid float point number.");
                }
            }
            rd.next();
        }

#define MIN_NUMBER_VALUE 1e-8
        inline void read_json(reader_t &rd, bool &val)
        {
            auto& tok = rd.peek();
            switch (tok.type)
            {
                case token::t_string:
                {
                    char const * ptr = tok.str.str;
                    if (tok.str.len == 4)
                    {
                        val = (ptr[0] == 't' || ptr[0] == 'T') &&
                              (ptr[1] == 'r' || ptr[1] == 'R') &&
                              (ptr[2] == 'u' || ptr[2] == 'U') &&
                              (ptr[3] == 'e' || ptr[3] == 'E');
                    }
                    else
                    {
                        val = false;
                    }
                    break;
                }
                case token::t_int:
                {
                    val = (tok.value.i64 != 0);
                    break;
                }
                case token::t_uint:
                {
                    val = (tok.value.u64 != 0);
                    break;
                }
                case token::t_number:
                {
                val = fabs(tok.value.d64) > MIN_NUMBER_VALUE;
                    break;
                }
                default:
                {
                    rd.error("not a valid bool.");
                }
            }
            rd.next();
        }

        inline void read_json(reader_t &rd, std::string &val) {
            auto &tok = rd.peek();
            if (tok.type == token::t_string) {
                val.assign(tok.str.str, tok.str.len);
            }
            else {
                rd.error("not a valid string.");
            }
            rd.next();
        }

        template <typename T, size_t N>
        inline void read_json(reader_t &rd, T(&val)[N])
        {
            read_array(rd, val);
        }

        template<typename T>
        inline void read_array(reader_t &rd, T& val)
        {
            if (rd.expect('[') == false) {
                rd.error("array must start with [.");
            }
            rd.next();
            auto tok = &rd.peek();
            int index = 0;
            while (tok->str.str[0] != ']') {
                read_json(rd, val[index++]);
                tok = &rd.peek();
                if (tok->str.str[0] == ',') {
                    rd.next();
                    tok = &rd.peek();
                    continue;
                }
                else if (tok->str.str[0] == ']') {
                    break;
                }
                else {
                    rd.error("no valid array!");
                }
            }
            rd.next();
        }

        template <typename T, size_t N>
        inline void read_json(reader_t &rd, std::array<T, N>& val)
        {
            read_array(rd, val);
        }

        template<typename T>
        std::enable_if_t<is_emplace_back_able<T>::value> emplace_back(T &val) {
            val.emplace_back();
        }

        template<typename T>
        std::enable_if_t<is_template_instant_of<std::queue, T>::value> emplace_back(T &val) {
            val.emplace();
        }

        template<typename T>
        inline std::enable_if_t<is_sequence_container<T>::value> read_json(reader_t &rd, T &val) {
            if (rd.expect('[') == false) {
                rd.error("array must start with [.");
            }
            rd.next();
            auto tok = &rd.peek();
            while (tok->str.str[0] != ']') {
                emplace_back(val);
                read_json(rd, val.back());
                tok = &rd.peek();
                if (tok->str.str[0] == ',') {
                    rd.next();
                    tok = &rd.peek();
                    continue;
                }
                else if (tok->str.str[0] == ']') {
                    break;
                }
                else {
                    rd.error("no valid array!");
                }
            }
            rd.next();
        }

        template<typename T>
        inline std::enable_if_t<is_associat_container<T>::value> read_json(reader_t &rd, T &val) {
            if (rd.expect('{') == false)
            {
                rd.error("object must start with {!");
            }
            rd.next();
            auto tok = &rd.peek();
            while (tok->str.str[0] != '}')
            {
                typename T::key_type key;
                read_json(rd, key);
                if (rd.expect(':') == false)
                {
                    rd.error("invalid object!");
                }
                rd.next();
                typename T::mapped_type value;
                read_json(rd, value);
                val[key] = value;
                tok = &rd.peek();
                if (tok->str.str[0] == ',')
                {
                    rd.next();
                    tok = &rd.peek();
                    continue;
                }
                else if (tok->str.str[0] == '}')
                {
                    break;
                }
                else
                {
                    rd.error("no valid object!");
                }
            }
            rd.next();
        }

        template<typename T, typename = std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>>
        inline void read_json(reader_t &rd, T &val) {
            do_read(rd, val);
            rd.next();
        }

        template <typename T>
        struct read_visitor_t
        {
            read_visitor_t(reader_t& rd, T& t)
                : rd_(rd)
                , t_(t)
            {
                static_assert(!std::is_const_v<T>);
            }

            template <typename PMD, typename Arr>
            void visit_mdata(std::string_view name, PMD pmd, size_t index, Arr names)
            {
                using element_type = std::remove_reference_t<decltype(t_.*pmd)>;
                if constexpr (is_reflected_v<element_type>)
                {
                    rd_.next();
                    rd_.next();
                    rd_.next();
                    do_read(rd_, t_.*pmd);
                    rd_.next();
                }
                else
                {
                    rd_.next();
                    if (rd_.peek().str != name.data()) {
                        g_has_error = true;
                        return;
                    }

                    rd_.next();
                    rd_.next();
                    read_json(rd_, t_.*pmd);
                }
            }

        private:
            reader_t&   rd_;
            T&          t_;
        };

        template <class Tuple, class F, std::size_t...Is>
        void tuple_switch(std::size_t i, Tuple&& t, F&& f, std::index_sequence<Is...>) {
            ((i == Is && ((std::forward<F>(f)(std::get<Is>(t))), false)), ...);
        }

        template<typename T>
		inline constexpr std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>> do_read(reader_t &rd, T &&t)
        {
            reflect_visit(reflexpr(t) {}, read_visitor_t{ rd, t });
        }

		template<typename U, typename T>
		inline void assign(reader_t& rd, T& t) {
			if constexpr (!is_reflected_v<U>)
			{
				read_json(rd, t);
			}
			else
			{
				do_read(rd, t);
				rd.next();
			}

			if (g_has_error)
				return;

			rd.next();
		}

		template<typename T>
		inline constexpr std::enable_if_t<is_tuple<std::decay_t<T>>::value, bool>
			from_json(T&& t, const char *buf, size_t len = -1) {
			using U = std::decay_t<T>;
			g_has_error = false;
			reader_t rd(buf, len);
			rd.next();
			for_each(std::forward<T>(t), [&rd](auto &v, auto i)
			{
				assign<decltype(v)>(rd, v);
			});
			return !g_has_error;
		}

		template<typename T>
		inline constexpr std::enable_if_t<is_sequence_container<std::decay_t<T>>::value, bool> 
			from_json(T&& v, const char *buf, size_t len = -1) {
			v.clear();
			using U = typename std::decay_t<T>::value_type;
			U t{};
			reader_t rd(buf, len);
			rd.next();
			while (rd.peek().type != token::t_end)
			{
				if (g_has_error)
					return false;

				assign<U>(rd, t);
				v.push_back(std::move(t));
			}
			
			return true;
		}

        template<typename T>
        inline constexpr auto from_json(T &&t, const char *buf, size_t len = -1) 
            -> std::enable_if_t< is_reflected_v<std::remove_reference_t<T>>, bool>
        {
            g_has_error = false;
            reader_t rd(buf, len);
            do_read(rd, t);
            return !g_has_error;
        }

        //this interface support disorderly parse, however slower than from_json interface
        template<typename T>
        inline constexpr auto from_json0(T &&t, const char *buf, size_t len = -1) 
            -> std::enable_if_t< is_reflected_v<std::remove_reference_t<T>>, bool>
        {
            g_has_error = false;
            reader_t rd(buf, len);
            do_read0(rd, t);
            return !g_has_error;
        }

        template<typename T>
        constexpr auto do_read0(reader_t &rd, T &&t)
            -> std::enable_if_t<is_reflected_v<std::remove_reference_t<T>>>
        {
            while(rd.peek().type != token::t_end) {
                rd.next();
				auto& tk = rd.peek();

                std::string_view s(tk.str.str, tk.str.len);
                auto index = iguana::get_index<T>(s);
                if(index==iguana::get_size<T>()){
					if (tk.type == token::t_end)
						break;

					rd.next();
					rd.next();
					rd.next();
                    continue;
                }

                tuple_switch(index, iguana::get_mdata<T>(), [&t, &rd](auto &v) {
                    if constexpr (!is_reflected_v<std::remove_reference_t<decltype(t.*v)>>)
                    {
                        rd.next();
                        rd.next();
                        read_json(rd, t.*v);
                    }
                    else
                    {
                        rd.next();
                        rd.next();
                        do_read0(rd, t.*v);
                        rd.next();
                    }
                }, std::make_index_sequence<iguana::get_size<T>()>{});
            }
        }
    } }
#endif //SERIALIZE_JSON_HPP


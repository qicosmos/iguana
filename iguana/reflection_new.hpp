#pragma once

#include <type_traits>
#include <array>
#include <boost/preprocessor.hpp>

namespace iguana
{
    template <typename T>
    struct reflect_info 
    {
        static char const* name() noexcept
        {
            return typeid(T).name();
        }
    };
}

/* common macros */
#define IGUANA_APPLY_MACRO(macro) macro
#define IGUANA_ACCESS_MEMBER(c, m) &c::m
#define IGUANA_MEMBER_NAME_I(member) std::string_view{ #member }
#define IGUANA_MEMBER_NAME(member) IGUANA_MEMBER_NAME_I(member)

namespace iguana
{
    template <typename T, typename F>
    struct to_member_function_pointer;

    template <typename T, typename Ret, typename ... Args>
    struct to_member_function_pointer<T, Ret(Args...)>
    {
        using type = Ret(T::*)(Args...);
    };

    template <typename T, typename Ret, typename ... Args>
    struct to_member_function_pointer<T, Ret(Args...) const>
    {
        using type = Ret(T::*)(Args...) const;
    };

    template <typename T, typename = void>
    struct is_reflected : std::false_type {};

    template <typename T>
    struct is_reflected<T, std::void_t<
        decltype(reflect_info<T>::name())
    >> : std::true_type {};

    template <typename T>
    inline constexpr bool is_reflected_v = is_reflected<T>::value;

    template <bool Test, typename T = void>
    using disable_if = std::enable_if<!Test, T>;
    template <bool Test, typename T = void>
    using disable_if_t = typename disable_if<Test, T>::type;
}

/* non-static member function */
// function member without overloading
#define IGUANA_MFUNC_MEMBER_II_1 IGUANA_ACCESS_MEMBER
// function member with overloading
#define IGUANA_MFUNC_MEMBER_II_2(c, m, t) \
    static_cast<iguana::to_member_function_pointer<c, t>::type>(IGUANA_ACCESS_MEMBER(c, m))
// reflect all members
#define IGUANA_MFUNC_MEMBER_II(c, ...) \
    IGUANA_APPLY_MACRO(BOOST_PP_OVERLOAD(IGUANA_MFUNC_MEMBER_II_, __VA_ARGS__)(c, __VA_ARGS__))
// for each macro to create names
#define IGUANA_MEMBER_NAME_0 IGUANA_MEMBER_NAME
#define IGUANA_MEMBER_NAME_1(tuple) IGUANA_MEMBER_NAME(BOOST_PP_TUPLE_ELEM(0, tuple))
#define IGUANA_MFUNC_MEMBER_NAME(r, data, i, t) \
    BOOST_PP_COMMA_IF(i) \
    IGUANA_APPLY_MACRO(BOOST_PP_CAT(IGUANA_MEMBER_NAME_, BOOST_PP_IS_BEGIN_PARENS(t))(t))
// for each macro to create pointer to member function
#define IGUANA_MFUNC_MEMBER_I_0 IGUANA_MFUNC_MEMBER_II
#define IGUANA_MFUNC_MEMBER_I_1(c, t) IGUANA_MFUNC_MEMBER_II(c, BOOST_PP_TUPLE_ENUM(t))
#define IGUANA_MFUNC_MEMBER_I(c, t) \
    BOOST_PP_CAT(IGUANA_MFUNC_MEMBER_I_, BOOST_PP_IS_BEGIN_PARENS(t))(c, t)
#define IGUANA_MFUNC_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) IGUANA_MFUNC_MEMBER_I(data, t)
// entry for all member function pointers
#define IGUANA_MFUNC_PROCESS(CLASS, N, ...) \
static constexpr auto mfunc_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(IGUANA_MFUNC_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mfunc() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(IGUANA_MFUNC_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); } \
static constexpr size_t mfunc_count = N;

/* static member function */
// for each macro to create names
#define IGUANA_SFUNC_MEMBER_NAME IGUANA_MFUNC_MEMBER_NAME
// for each macro to create function pointer
#define IGUANA_SFUNC_MEMBER_II_1 IGUANA_ACCESS_MEMBER
#define IGUANA_SFUNC_MEMBER_II_2(c, m, t) static_cast<std::add_pointer_t<t>>(IGUANA_ACCESS_MEMBER(c, m))
#define IGUANA_SFUNC_MEMBER_II(c, ...) \
    IGUANA_APPLY_MACRO(BOOST_PP_OVERLOAD(IGUANA_SFUNC_MEMBER_II_, __VA_ARGS__)(c, __VA_ARGS__))
#define IGUANA_SFUNC_MEMBER_I_0 IGUANA_SFUNC_MEMBER_II
#define IGUANA_SFUNC_MEMBER_I_1(c, t) IGUANA_SFUNC_MEMBER_II(c, BOOST_PP_TUPLE_ENUM(t))
#define IGUANA_SFUNC_MEMBER_I(c, t) \
    BOOST_PP_CAT(IGUANA_SFUNC_MEMBER_I_, BOOST_PP_IS_BEGIN_PARENS(t))(c, t)
#define IGUANA_SFUNC_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) IGUANA_SFUNC_MEMBER_I(data, t)
// entry for all static member function
#define IGUANA_SFUNC_PROCESS(CLASS, N, ...) \
static constexpr auto sfunc_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(IGUANA_SFUNC_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mfunc() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(IGUANA_SFUNC_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));}\
static constexpr size_t sfunc_count = N;

/* static and no-static data member*/
#define IGUANA_DATA_MEMBER_NAME(r, data, i, t) BOOST_PP_COMMA_IF(i) IGUANA_MEMBER_NAME(t)
#define IGUANA_DATA_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) &data::t
// non-static data member
#define IGUANA_MDATA_PROCESS(CLASS, N, ...) \
static constexpr auto mdata_names() noexcept -> std::array<std::string_view, N> { \
    return { BOOST_PP_SEQ_FOR_EACH_I(IGUANA_DATA_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mdata() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(IGUANA_DATA_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));} \
static constexpr size_t mdata_count = N;
// static data member
#define IGUANA_SDATA_PROCESS(CLASS, N, ...) \
static constexpr auto sdata_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(IGUANA_DATA_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto sdata() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(IGUANA_DATA_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));} \
static constexpr size_t sdata_count = N;

/* constructor */
#define IGUANA_TUPLE_TO_STD_TUPLE(t) std::tuple<BOOST_PP_TUPLE_ENUM(t)>
#define IGUANA_CTOR_LIST_ITEM(r, data, i, t) BOOST_PP_COMMA_IF(i) IGUANA_TUPLE_TO_STD_TUPLE(t)
#define IGUANA_CTOR_PROCESS(CLASS, N, ...) \
using ctor_list = std::tuple<BOOST_PP_SEQ_FOR_EACH_I(IGUANA_CTOR_LIST_ITEM, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>; \
static constexpr size_t ctor_count = N;

/* dispatch */
#define IGUANA_MDATA(...) ((IGUANA_MDATA_PROCESS, (__VA_ARGS__)))
#define IGUANA_SDATA(...) ((IGUANA_SDATA_PROCESS, (__VA_ARGS__)))
#define IGUANA_MFUNC(...) ((IGUANA_MFUNC_PROCESS, (__VA_ARGS__)))
#define IGUANA_SFUNC(...) ((IGUANA_SFUNC_PROCESS, (__VA_ARGS__)))
#define IGUANA_CTOR(...)  ((IGUANA_CTOR_PROCESS,  (__VA_ARGS__)))
#define IGUANA_APPLY_PROCESS_II(c, p, n, ...) p(c, n, __VA_ARGS__)
#define IGUANA_APPLY_PROCESS_I(c, p, params) \
    IGUANA_APPLY_PROCESS_II(c, p, BOOST_PP_TUPLE_SIZE(params), BOOST_PP_TUPLE_ENUM(params))
#define IGUANA_APPLY_PROCESS(r, data, tuple) \
    IGUANA_APPLY_PROCESS_I(data, BOOST_PP_TUPLE_ELEM(0, tuple), BOOST_PP_TUPLE_ELEM(1, tuple))

/* MAIN entry */
#define IGUANA_REFLECT_1(c)                                               \
template<> struct iguana::reflect_info<c> {                               \
    static constexpr std::string_view name() noexcept { return #c; }    \
};
#define IGUANA_REFLECT_2(c, seq)                                          \
template<> struct iguana::reflect_info<c> {                               \
    static constexpr std::string_view name() noexcept { return #c; }    \
    BOOST_PP_SEQ_FOR_EACH(IGUANA_APPLY_PROCESS, c, seq)                   \
};
#define IGUANA_REFLECT(...) IGUANA_APPLY_MACRO(BOOST_PP_OVERLOAD(IGUANA_REFLECT_, __VA_ARGS__)(__VA_ARGS__))

/* useful mpl */
#define IGUANA_TMP_HAS(CAT)                                               \
template <typename T, typename = void>                                  \
struct BOOST_PP_CAT(has_reflect_, CAT) : std::false_type{};             \
template <typename T>                                                   \
struct BOOST_PP_CAT(has_reflect_, CAT)<T, std::void_t<                  \
    decltype(T::BOOST_PP_CAT(CAT, _names)()),                           \
    decltype(T::BOOST_PP_CAT(CAT, BOOST_PP_EMPTY())())                  \
>> : std::true_type{};                                                  \
template <typename T, typename = void>                                  \
struct BOOST_PP_CAT(has_visit_, CAT) : std::false_type{};               \
template <typename T>                                                   \
struct BOOST_PP_CAT(has_visit_, CAT)<T, std::void_t<                    \
    decltype(std::declval<T>().BOOST_PP_CAT(visit_, CAT)(               \
        std::declval<std::string_view>(),                               \
        std::declval<int>(),                                            \
        std::declval<size_t>(),                                         \
        std::declval<int>()                                             \
    ))>> : std::true_type{};

#define IGUANA_TMP_VISIT(CAT)                                             \
template <typename T, typename V>                                       \
inline static auto BOOST_PP_CAT(visit_, CAT)(reflect_info<T>, V const& visitor) noexcept \
-> std::enable_if_t<std::conjunction<                                   \
    BOOST_PP_CAT(has_reflect_, CAT)<reflect_info<T>>,                   \
    BOOST_PP_CAT(has_visit_, CAT)<V>>::value> {                         \
    using reflect_into_t = reflect_info<T>;                             \
    constexpr size_t size = reflect_into_t::BOOST_PP_CAT(CAT, _count);  \
    visit_loop(                                                         \
        reflect_into_t::BOOST_PP_CAT(CAT, _names)(),                    \
        reflect_into_t::BOOST_PP_CAT(CAT, BOOST_PP_EMPTY())(),          \
        [&visitor](auto ... args) {                                     \
            visitor.BOOST_PP_CAT(visit_, CAT)(std::forward<decltype(args)>(args)...); }, \
        std::make_index_sequence<size>{});                              \
}                                                                       \
template <typename T, typename V>                                       \
inline static auto BOOST_PP_CAT(visit_, CAT)(reflect_info<T>, V const& visitor) noexcept \
-> disable_if_t<std::conjunction<                                       \
    BOOST_PP_CAT(has_reflect_, CAT)<reflect_info<T>>,                   \
    BOOST_PP_CAT(has_visit_, CAT)<V>>::value>{}

/* some tricks to remove DRY code */
#define IGUANA_VISIT_SEQ (mdata)(sdata)(mfunc)(sfunc)
#define IGUANA_VISIT_PROC(r, data, elem) BOOST_PP_CAT(visit_, elem)(ri, visitor);
#define IGUANA_TMP_HAS_PROC(r, data, elem) IGUANA_TMP_HAS(elem)
#define IGUANA_TMP_VISIT_PROC(r, data, elem) IGUANA_TMP_VISIT(elem)

/* interface to visit the reflect_info */
namespace iguana
{
    BOOST_PP_SEQ_FOR_EACH(IGUANA_TMP_HAS_PROC, _, IGUANA_VISIT_SEQ)

    template <typename T, typename = void>
    struct has_ctor_list : std::false_type {};
    template <typename T>
    struct has_ctor_list<T, std::void_t<
        typename T::ctor_list
        >> : std::true_type {};

    template <typename T, typename = void>
    struct has_visit_ctor : std::false_type {};
    template <typename T>
    struct has_visit_ctor<T, std::void_t<
        decltype(&T::visit_ctor)
        >> : std::true_type {};

    namespace reflect_detail
    {
        template <typename Arr, typename Tuple, typename F, size_t ... Is>
        inline static auto visit_loop(Arr const& names, Tuple const& members, F const& func, std::index_sequence<Is...>)
        {
            swallow_t{
                (func(names[Is], std::get<Is>(members), Is, names), true) ...
            };
        }

        BOOST_PP_SEQ_FOR_EACH(IGUANA_TMP_VISIT_PROC, _, IGUANA_VISIT_SEQ)

        template <typename T, typename V>
        inline static void visit(reflect_info<T> ri, V const& visitor) noexcept
        {
            BOOST_PP_SEQ_FOR_EACH(IGUANA_VISIT_PROC, _, IGUANA_VISIT_SEQ)
        }
    }

    template <typename T, typename V>
    inline static void reflect_visit(V const& visitor) noexcept
    {
        reflect_detail::visit(reflect_info<std::remove_const_t<T>>{}, visitor);
    }

    template <typename T, typename V>
    inline static void reflect_visit(reflect_info<T> ri, V const& visitor) noexcept
    {
        reflect_detail::visit(ri, visitor);
    }
}

// reflect interface
namespace iguana
{
    template <typename T>
    inline static constexpr auto reflect()
    {
        static_assert(is_reflected_v<T>);
        return reflect_info<T>{};
    }
}

// simulate the new reflexpr keyword
#define reflexpr(x) iguana::reflect_info<std::remove_const_t<x>>

#ifndef TCB_UNICODE_HPP_INCLUDED
#define TCB_UNICODE_HPP_INCLUDED

#include <array>
#include <cstdint>
#include <iterator>
#include <string>

#if __cpp_constexpr >= 201304
#define TCB_CONSTEXPR14 constexpr
#else
#define TCB_CONSTEXPR14
#endif

namespace tcb {
namespace unicode {

namespace detail {

template <typename C, typename InputIt, typename Sentinel>
using is_compatible_container =
    std::conditional_t<std::is_constructible<C, InputIt, Sentinel>::value,
                       std::true_type,
                       std::false_type>;

template <typename C, typename InputIt, typename Sentinel>
constexpr bool is_compatible_container_v = is_compatible_container<C, InputIt, Sentinel>::value;

#ifdef __GNUC__
#   define TCB_LIKELY(x)   __builtin_expect((x),1)
#   define TCB_UNLIKELY(x) __builtin_expect((x),0)
#else
#   define TCB_LIKELY(x)   (x)
#   define TCB_UNLIKELY(x) (x)
#endif

using code_point = char32_t;
static constexpr code_point illegal = 0xFFFFFFFFu;
static constexpr code_point incomplete = 0xFFFFFFFEu;

inline TCB_CONSTEXPR14 bool is_valid_codepoint(code_point v)
{
    if (v > 0x10FFFF)
        return false;
    if (0xD800 <= v && v <= 0xDFFF) // surrogates
        return false;
    return true;
}

template <typename CharType, int size = sizeof(CharType)>
struct utf_traits;

template <typename CharType>
struct encoded_chars {
public:
    constexpr encoded_chars() = default;

    constexpr encoded_chars(CharType _1)
            : chars_{{_1}}, size_{1} {}

    constexpr encoded_chars(CharType _1, CharType _2)
            : chars_{{_1, _2}}, size_{2} {}

    constexpr encoded_chars(CharType _1, CharType _2, CharType _3)
            : chars_{{_1, _2, _3}}, size_{3} {}

    constexpr encoded_chars(CharType _1, CharType _2, CharType _3, CharType _4)
            : chars_{{_1, _2, _3, _4}}, size_{4} {}

    constexpr int size() const noexcept { return size_; }

    constexpr const CharType& operator[](int i) const noexcept { return chars_[i]; }

    friend bool operator==(const encoded_chars& lhs, const encoded_chars& rhs)
    {
        return std::equal(std::begin(lhs.chars_),
                          std::begin(lhs.chars_) + lhs.size_,
                          std::begin(rhs.chars_),
                          std::begin(rhs.chars_) + rhs.size_);
    }

private:
    std::array<CharType, 4 / sizeof(CharType)> chars_{{}};
    int size_ = 0;
};

template <typename CharType>
struct utf_traits<CharType, 1> {

    typedef CharType char_type;

    static TCB_CONSTEXPR14 int trail_length(char_type ci)
    {
        unsigned char c = ci;
        if (c < 128)
            return 0;
        if (TCB_UNLIKELY(c < 194))
            return -1;
        if (c < 224)
            return 1;
        if (c < 240)
            return 2;
        if (TCB_LIKELY(c <= 244))
            return 3;
        return -1;
    }

    static constexpr int max_width = 4;

    static TCB_CONSTEXPR14 int width(code_point value)
    {
        if (value <= 0x7F) {
            return 1;
        }
        else if (value <= 0x7FF) {
            return 2;
        }
        else if (TCB_LIKELY(value <= 0xFFFF)) {
            return 3;
        }
        else {
            return 4;
        }
    }

    static TCB_CONSTEXPR14 bool is_trail(char_type ci)
    {
        unsigned char c = ci;
        return (c & 0xC0) == 0x80;
    }

    static constexpr bool is_lead(char_type ci)
    {
        return !is_trail(ci);
    }

    template <typename Iterator, typename Sentinel>
    static TCB_CONSTEXPR14 code_point decode(Iterator& p, Sentinel e)
    {
        if (TCB_UNLIKELY(p == e))
            return incomplete;

        unsigned char lead = *p++;

        // First byte is fully validated here
        int trail_size = trail_length(lead);

        if (TCB_UNLIKELY(trail_size < 0))
            return illegal;

        //
        // Ok as only ASCII may be of size = 0
        // also optimize for ASCII text
        //
        if (trail_size == 0)
            return lead;

        code_point c = lead & ((1 << (6 - trail_size)) - 1);

        // Read the rest
        unsigned char tmp{};
        switch (trail_size) {
        case 3:
            if (TCB_UNLIKELY(p == e))
                return incomplete;
            tmp = *p++;
            if (!is_trail(tmp))
                return illegal;
            c = (c << 6) | (tmp & 0x3F);
        case 2:
            if (TCB_UNLIKELY(p == e))
                return incomplete;
            tmp = *p++;
            if (!is_trail(tmp))
                return illegal;
            c = (c << 6) | (tmp & 0x3F);
        case 1:
            if (TCB_UNLIKELY(p == e))
                return incomplete;
            tmp = *p++;
            if (!is_trail(tmp))
                return illegal;
            c = (c << 6) | (tmp & 0x3F);
        }

        // Check code point validity: no surrogates and
        // valid range
        if (TCB_UNLIKELY(!is_valid_codepoint(c)))
            return illegal;

        // make sure it is the most compact representation
        if (TCB_UNLIKELY(width(c) != trail_size + 1))
            return illegal;

        return c;

    }

    template <typename Iterator>
    static TCB_CONSTEXPR14 code_point decode_valid(Iterator& p)
    {
        unsigned char lead = *p++;
        if (lead < 192)
            return lead;

        int trail_size = 0;

        if (lead < 224)
            trail_size = 1;
        else if (TCB_LIKELY(lead < 240)) // non-BMP rare
            trail_size = 2;
        else
            trail_size = 3;

        code_point c = lead & ((1 << (6 - trail_size)) - 1);

        switch (trail_size) {
        case 3:
            c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
        case 2:
            c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
        case 1:
            c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
        }

        return c;
    }

    template <typename Iterator>
    static TCB_CONSTEXPR14 Iterator encode(code_point value, Iterator out)
    {
        if (value <= 0x7F) {
            *out++ = static_cast<char_type>(value);
        }
        else if (value <= 0x7FF) {
            *out++ = static_cast<char_type>((value >> 6) | 0xC0);
            *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
        }
        else if (TCB_LIKELY(value <= 0xFFFF)) {
            *out++ = static_cast<char_type>((value >> 12) | 0xE0);
            *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
            *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
        }
        else {
            *out++ = static_cast<char_type>((value >> 18) | 0xF0);
            *out++ = static_cast<char_type>(((value >> 12) & 0x3F) | 0x80);
            *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
            *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
        }
        return out;
    }

    static TCB_CONSTEXPR14 encoded_chars<CharType> encode(code_point value)
    {
        if (value <= 0x7F) {
            return {static_cast<char_type>(value)};
        }
        else if (value <= 0x7FF) {
            return {static_cast<char_type>((value >> 6) | 0xC0),
                    static_cast<char_type>((value & 0x3F) | 0x80)};
        }
        else if (TCB_LIKELY(value <= 0xFFFF)) {
            return {static_cast<char_type>((value >> 12) | 0xE0),
                    static_cast<char_type>(((value >> 6) & 0x3F) | 0x80),
                    static_cast<char_type>((value & 0x3F) | 0x80)};
        }
        else {
            return {static_cast<char_type>((value >> 18) | 0xF0),
                    static_cast<char_type>(((value >> 12) & 0x3F) | 0x80),
                    static_cast<char_type>(((value >> 6) & 0x3F) | 0x80),
                    static_cast<char_type>((value & 0x3F) | 0x80)};
        }
    }

}; // utf8

template <typename CharType>
struct utf_traits<CharType, 2> {
    typedef CharType char_type;

    // See RFC 2781
    static constexpr bool is_first_surrogate(uint16_t x)
    {
        return 0xD800 <= x && x <= 0xDBFF;
    }

    static constexpr bool is_second_surrogate(uint16_t x)
    {
        return 0xDC00 <= x && x <= 0xDFFF;
    }

    static constexpr code_point combine_surrogate(uint16_t w1, uint16_t w2)
    {
        return ((code_point(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
    }

    static TCB_CONSTEXPR14 int trail_length(char_type c)
    {
        if (is_first_surrogate(c))
            return 1;
        if (is_second_surrogate(c))
            return -1;
        return 0;
    }

    ///
    /// Returns true if c is trail code unit, always false for UTF-32
    ///
    static constexpr bool is_trail(char_type c)
    {
        return is_second_surrogate(c);
    }

    ///
    /// Returns true if c is lead code unit, always true of UTF-32
    ///
    static constexpr bool is_lead(char_type c)
    {
        return !is_second_surrogate(c);
    }

    template <typename It, typename S>
    static TCB_CONSTEXPR14 code_point decode(It& current, S last)
    {
        if (TCB_UNLIKELY(current == last))
            return incomplete;
        uint16_t w1 = *current++;
        if (TCB_LIKELY(w1 < 0xD800 || 0xDFFF < w1)) {
            return w1;
        }
        if (w1 > 0xDBFF)
            return illegal;
        if (current == last)
            return incomplete;
        uint16_t w2 = *current++;
        if (w2 < 0xDC00 || 0xDFFF < w2)
            return illegal;
        return combine_surrogate(w1, w2);
    }

    template <typename It>
    static TCB_CONSTEXPR14 code_point decode_valid(It& current)
    {
        uint16_t w1 = *current++;
        if (TCB_LIKELY(w1 < 0xD800 || 0xDFFF < w1)) {
            return w1;
        }
        uint16_t w2 = *current++;
        return combine_surrogate(w1, w2);
    }

    static const int max_width = 2;

    static constexpr int width(code_point u)
    {
        return u >= 0x10000 ? 2 : 1;
    }

    template <typename It>
    static TCB_CONSTEXPR14 It encode(code_point u, It out)
    {
        if (TCB_LIKELY(u <= 0xFFFF)) {
            *out++ = static_cast<char_type>(u);
        }
        else {
            u -= 0x10000;
            *out++ = static_cast<char_type>(0xD800 | (u >> 10));
            *out++ = static_cast<char_type>(0xDC00 | (u & 0x3FF));
        }
        return out;
    }

    static TCB_CONSTEXPR14 encoded_chars<CharType> encode(code_point u)
    {
        if (TCB_LIKELY(u <= 0xFFFF)) {
            return {static_cast<char_type>(u)};
        }
        else {
            u -= 0x10000;
            return {static_cast<char_type>(0xD800 | (u >> 10)),
                    static_cast<char_type>(0xDC00 | (u & 0x3FF))};
        }
    }
}; // utf16;


template <typename CharType>
struct utf_traits<CharType, 4> {
    typedef CharType char_type;

    static constexpr int trail_length(char_type c)
    {
        return is_valid_codepoint(c) ? 0 : -1;
    }

    static constexpr bool is_trail(char_type /*c*/)
    {
        return false;
    }

    static constexpr bool is_lead(char_type /*c*/)
    {
        return true;
    }

    template <typename It>
    static constexpr code_point decode_valid(It& current)
    {
        return *current++;
    }

    template <typename It, typename S>
    static TCB_CONSTEXPR14 code_point decode(It& current, S last)
    {
        if (TCB_UNLIKELY(current == last))
            return incomplete;
        code_point c = *current++;
        if (TCB_UNLIKELY(!is_valid_codepoint(c)))
            return illegal;
        return c;
    }

    static constexpr int max_width = 1;

    static constexpr int width(code_point /*u*/)
    {
        return 1;
    }

    template <typename It>
    static TCB_CONSTEXPR14 It encode(code_point u, It out)
    {
        *out++ = static_cast<char_type>(u);
        return out;
    }

    static constexpr encoded_chars<char_type> encode(code_point u)
    {
        return {static_cast<char_type>(u)};
    }

}; // utf32


template <typename InputIt, typename Sentinel, typename InCharT, typename OutCharT>
class unicode_view {
private:
    struct iterator {
        // Required typedefs
        using value_type = OutCharT;
        using difference_type = typename std::iterator_traits<InputIt>::difference_type; // ?
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = // ForwardIterator, unless input is only InputIterator
            std::conditional_t<std::is_same<typename std::iterator_traits<InputIt>::iterator_category,
                                            std::input_iterator_tag>::value,
                               std::input_iterator_tag,
                               std::forward_iterator_tag>;

        constexpr iterator() = default;

        TCB_CONSTEXPR14 iterator(InputIt first, Sentinel last)
                : first_(first), last_(last)
        {
            if (first_ != last_) {
                char32_t c = detail::utf_traits<InCharT>::decode(first_, last_);
                next_chars_ = detail::utf_traits<OutCharT>::encode(c);
            }
        }

        constexpr reference operator*() const
        {
            return next_chars_[idx_];
        }

        constexpr pointer operator->() const
        {
            return &next_chars_[idx_];
        }

        TCB_CONSTEXPR14 iterator& operator++()
        {
            if (++idx_ == next_chars_.size() && first_ != last_) {
                char32_t c = utf_traits<InCharT>::decode(first_, last_);
                next_chars_ = utf_traits<OutCharT>::encode(c);
                idx_ = 0;
            }
            return *this;
        }

        TCB_CONSTEXPR14 iterator operator++(int)
        {
            iterator temp{*this};
            this->operator++();
            return temp;
        }

        bool operator==(const iterator& other) const
        {
            return (done() && other.done()) ||
                    (first_ == other.first_ &&
                    last_ == other.last_ &&
                    idx_ == other.idx_ &&
                    next_chars_ == other.next_chars_);
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }



    private:

        bool done() const
        {
            return first_ == last_ && idx_ == next_chars_.size();
        }

        encoded_chars<OutCharT> next_chars_;
        InputIt first_{};
        Sentinel last_{};
        std::uint8_t idx_ = 0;
    };

public:

    constexpr unicode_view() = default;

    constexpr unicode_view(InputIt first, Sentinel last)
            : first_(first),
              last_(last) {}

    constexpr iterator begin() const { return iterator{first_, last_}; }

    constexpr iterator cbegin() const { return begin(); }

	constexpr iterator end() const { return iterator{last_, last_}; }

    constexpr iterator cend() const { return end(); }

    template <typename Container,
              typename = std::enable_if_t<is_compatible_container_v<Container, InputIt, Sentinel>>>
    constexpr operator Container() const
    {
        return Container(begin(), end());
    }

private:
    InputIt first_{};
    Sentinel last_{};
};

template <typename Iter>
using iter_value_t = typename std::iterator_traits<Iter>::value_type;

} // end namespace detail

template <typename InputIt, typename Sentinel,
          typename InCharT = detail::iter_value_t<InputIt>>
using utf8_view = detail::unicode_view<InputIt, Sentinel, InCharT, char>;

template <typename InputIt, typename Sentinel,
        typename InCharT = detail::iter_value_t<InputIt>>
using utf16_view = detail::unicode_view<InputIt, Sentinel, InCharT, char16_t>;

template <typename InputIt, typename Sentinel,
        typename InCharT = detail::iter_value_t<InputIt>>
using utf32_view = detail::unicode_view<InputIt, Sentinel, InCharT, char32_t>;

// View functions

template <typename InputIt, typename Sentinel>
constexpr
utf8_view<InputIt, Sentinel> as_utf8(InputIt first, Sentinel last)
{
    return {first, last};
}

template <typename String>
constexpr
auto as_utf8(const String& str)
{
    return as_utf8(std::cbegin(str), std::cend(str));
}

template <typename InputIt, typename Sentinel>
constexpr
utf16_view<InputIt, Sentinel> as_utf16(InputIt first, Sentinel last)
{
    return {first, last};
}

template <typename String>
constexpr
auto as_utf16(const String& str)
{
    return as_utf16(std::cbegin(str), std::cend(str));
}

template <typename InputIt, typename Sentinel>
constexpr
utf32_view<InputIt, Sentinel> as_utf32(InputIt first, Sentinel last)
{
    return {first, last};
}

template <typename String>
constexpr
auto as_utf32(const String& str)
{
    return as_utf32(std::cbegin(str), std::cend(str));
}

// Conversion functions

template <typename OutCharT,
          typename InIter, typename Sentinel,
          typename OutIter,
          typename InCharT = detail::iter_value_t<InIter>>
TCB_CONSTEXPR14
OutIter convert(InIter first, Sentinel last, OutIter out)
{
    while (first != last) {
        const char32_t c = detail::utf_traits<InCharT>::decode(first, last);
        detail::utf_traits<OutCharT>::encode(c, out);
    }
    return out;
}

template <typename OutCharT,
          typename InputIt, typename Sentinel,
          typename InCharT = detail::iter_value_t<InputIt>>
std::basic_string<OutCharT>
to_utf_string(InputIt first, Sentinel last)
{
    using string_type = std::basic_string<OutCharT>;

    string_type output;

    // Try to minimise the number of reallocations
    if /*constexpr*/ (std::is_same<typename std::iterator_traits<InputIt>::iterator_category,
                                   std::random_access_iterator_tag>::value) {
        output.reserve(static_cast<typename string_type::size_type>(std::distance(first, last)));
    }

    convert<OutCharT>(first, last, std::back_inserter(output));

    return output;
}

template <typename InputIt, typename Sentinel>
std::string to_u8string(InputIt first, Sentinel last)
{
    return to_utf_string<char>(first, last);
}

template <typename String>
std::string to_u8string(const String& str)
{
    return to_u8string(std::cbegin(str), std::cend(str));
}

template <typename InputIt, typename Sentinel>
std::u16string to_u16string(InputIt first, Sentinel last)
{
    return to_utf_string<char16_t>(first, last);
}

template <typename String>
std::u16string to_u16string(const String& str)
{
    return to_u16string(std::cbegin(str), std::cend(str));
}

template <typename InputIt, typename Sentinel>
std::u32string to_u32string(InputIt first, Sentinel last)
{
    return to_utf_string<char32_t>(first, last);
}

template <typename String>
std::u32string to_u32string(const String& str)
{
    return to_u32string(std::cbegin(str), std::cend(str));
}

} // end namespace unicode
} // end namespace tcb

#undef TCB_LIKELY
#undef TCB_UNLIKELY
#undef TCB_CONSTEXPR14

#endif

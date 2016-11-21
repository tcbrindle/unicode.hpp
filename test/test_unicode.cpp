
#include "catch.hpp"

#include <tcb/unicode.hpp>

#include <iostream>
#include <sstream>

using namespace tcb::unicode;

//#define TEST_STRING "$€0123456789你好abcdefghijklmnopqrstuvwxyz\U0001F60E"
#define TEST_STRING "ab"


const std::string u8 = u8"" TEST_STRING;
const std::u16string u16 = u"" TEST_STRING;
const std::u32string u32 = U"" TEST_STRING;
const std::wstring w = L"" TEST_STRING;

template <typename R1, typename R2>
bool equal(const R1& r1, const R2& r2)
{
    return std::equal(std::cbegin(r1), std::cend(r1),
                      std::cbegin(r2), std::cend(r2));
}

TEST_CASE("Basic test case")
{
    REQUIRE(equal(as_utf8(u8), u8));
    REQUIRE(equal(as_utf16(u8), u16));
    REQUIRE(equal(as_utf32(u8), u32));

    REQUIRE(equal(as_utf8(u16), u8));
    REQUIRE(equal(as_utf16(u16), u16));
    REQUIRE(equal(as_utf32(u16), u32));

    REQUIRE(equal(as_utf8(u32), u8));
    REQUIRE(equal(as_utf16(u32), u16));
    REQUIRE(equal(as_utf32(u32), u32));

    REQUIRE(equal(as_utf8(w), u8));
    REQUIRE(equal(as_utf16(w), u16));
    REQUIRE(equal(as_utf32(w), u32));
}

TEST_CASE("Input iterators can be converted")
{
    std::basic_stringstream<char> s8{u8};
    std::basic_stringstream<char16_t> s16{u16};
    std::basic_stringstream<char32_t> s32{u32};
    std::basic_stringstream<wchar_t> sw{w};

    using iter8 = std::istreambuf_iterator<char>;
    using iter16 = std::istreambuf_iterator<char16_t>;
    using iter32 = std::istreambuf_iterator<char32_t>;
    using iterw = std::istreambuf_iterator<wchar_t>;


    SECTION("UTF-8 view") {
        REQUIRE(equal(as_utf8(iter8{s8}, iter8{}), u8));
        REQUIRE(equal(as_utf8(iter16{s16}, iter16{}), u8));
        REQUIRE(equal(as_utf8(iter32{s32}, iter32{}), u8));
        REQUIRE(equal(as_utf8(iterw{sw}, iterw{}), u8));
    }

    SECTION("UTF-16 view") {
        REQUIRE(equal(as_utf16(iter8{s8}, iter8{}), u16));
        REQUIRE(equal(as_utf16(iter16{s16}, iter16{}), u16));
        REQUIRE(equal(as_utf16(iter32{s32}, iter32{}), u16));
        REQUIRE(equal(as_utf16(iterw{sw}, iterw{}), u16));
    }
    SECTION("UTF-32 view") {
        REQUIRE(equal(as_utf32(iter8{s8}, iter8{}), u32));
        REQUIRE(equal(as_utf32(iter16{s16}, iter16{}), u32));
        REQUIRE(equal(as_utf32(iter32{s32}, iter32{}), u32));
        REQUIRE(equal(as_utf32(iterw{sw}, iterw{}), u32));
    }



    SECTION("UTF-8 conversion") {
        REQUIRE(to_u8string(iter8{s8}, iter8{}) == u8);
        REQUIRE(to_u8string(iter16{s16}, iter16{}) == u8);
        REQUIRE(to_u8string(iter32{s32}, iter32{}) == u8);
        REQUIRE(to_u8string(iterw{sw}, iterw{}) == u8);
    }

    SECTION("UTF-16 conversion") {
        REQUIRE(to_u16string(iter8{s8}, iter8{}) == u16);
        REQUIRE(to_u16string(iter16{s16}, iter16{}) == u16);
        REQUIRE(to_u16string(iter32{s32}, iter32{}) == u16);
        REQUIRE(to_u16string(iterw{sw}, iterw{}) == u16);
    }

    SECTION("UTF-32 conversion") {
        REQUIRE(to_u32string(iter8{s8}, iter8{}) == u32);
        REQUIRE(to_u32string(iter16{s16}, iter16{}) == u32);
        REQUIRE(to_u32string(iter32{s32}, iter32{}) == u32);
        REQUIRE(to_u32string(iterw{sw}, iterw{}) == u32);
    }
}

TEST_CASE("Test conversion functions")
{
    REQUIRE(to_u8string(u8) == u8);
    REQUIRE(to_u8string(u16) == u8);
    REQUIRE(to_u8string(u32) == u8);
    REQUIRE(to_u8string(w) == u8);

    REQUIRE(to_u16string(u8) == u16);
    REQUIRE(to_u16string(u16) == u16);
    REQUIRE(to_u16string(u32) == u16);
    REQUIRE(to_u16string(w) == u16);

    REQUIRE(to_u32string(u8) == u32);
    REQUIRE(to_u32string(u16) == u32);
    REQUIRE(to_u32string(u32) == u32);
    REQUIRE(to_u32string(w) == u32);
}
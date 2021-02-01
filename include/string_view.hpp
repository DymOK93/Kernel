﻿#pragma once
#include "string_base.hpp"
#include "string_algorithms.hpp"
#include "type_traits.hpp"

#include <ntddk.h>

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
#include <string_view>
#endif

namespace ktl {
class unicode_string_view {
 public:
  using value_type = wchar_t;
  using pointer = wchar_t*;
  using const_pointer = const wchar_t*;
  using reference = wchar_t&;
  using const_reference = const wchar_t&;
  using const_iterator = const_pointer;
#ifndef KTL_NO_CXX_STANDARD_LIBRARY
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
#endif
  using size_type = unsigned short;  //Как и в UNICODE_STRING
  using difference_type = int;

  static constexpr size_type npos = static_cast<size_type>(-1);

 public:
  constexpr unicode_string_view() noexcept = default;
  constexpr unicode_string_view(const unicode_string_view& other) = default;
  constexpr unicode_string_view& operator=(const unicode_string_view& other) =
      default;

  template <size_t N>
  constexpr unicode_string_view(const wchar_t (&str)[N])
      : m_str{make_unicode_string(str), static_cast<size_type>(N)} {}
  constexpr unicode_string_view(const wchar_t* str)
      : m_str{make_unicode_string(str, static_cast<size_type>(ktl::length(str)))} {}
  constexpr unicode_string_view(const wchar_t* str, size_type length)
      : m_str{make_unicode_string(str, length)} {}
  constexpr unicode_string_view(UNICODE_STRING str) : m_str{str} {}
  constexpr unicode_string_view(PCUNICODE_STRING str_ptr) : m_str{*str_ptr} {}

 public:
  constexpr const_iterator begin() const noexcept { return cbegin(); }
  constexpr const_iterator cbegin() const noexcept { return data(); }
  constexpr const_iterator rbegin() const noexcept { return rbegin(); }
  constexpr const_iterator crbegin() const noexcept { return cend(); }
  constexpr const_iterator end() const noexcept { return cend(); }
  constexpr const_iterator cend() const noexcept { return data() + length(); }
  constexpr const_iterator rend() const noexcept { return crend(); }
  constexpr const_iterator crend() const noexcept { return cbegin(); }

 public:
  constexpr const_reference& operator[](size_type idx) const {
    return m_str.Buffer[idx];
  }
  constexpr const_reference& front() const { return m_str.Buffer[0]; }
  constexpr const_reference& back() const { return m_str.Buffer[length() - 1]; }
  constexpr const_pointer data() const { return m_str.Buffer; }

 public:
  constexpr size_type size() const {
    return length_to_characters(m_str.Length);
  }
  constexpr size_type length() const {
    return length_to_characters(m_str.Length);
  }
  constexpr bool empty() const { return length() == 0; }

 public:
  constexpr void remove_prefix(size_type shift) { m_str.Buffer += shift; }
  constexpr void remove_suffix(size_type shift) { m_str.Length -= shift; }
  constexpr void swap(unicode_string_view& other) {
    ktl::swap(m_str, other.m_str);
  }

 public:
  /*constexpr size_type copy(wchar_t* dst,          // ktl::copy пока недоступна
                              size_type count,
                              size_type pos = 0) {
    auto last{std::copy(begin() + pos,
                        begin() + calc_segment_length(pos, count), dst)};
    return static_cast<size_type>(last - dst);
  }
  constexpr size_type copy(PUNICODE_STRING dst,
                              size_type count,
                              size_type pos = 0) {
    return copy(dst->Buffer, count, pos);
  }*/
  constexpr unicode_string_view substr(size_type pos, size_type count = npos) {
    return unicode_string_view(data() + pos, calc_segment_length(pos, count));
  }
  constexpr int compare(unicode_string_view other) {
    return ktl::compare(data(), size(), other.data(), other.size());
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        unicode_string_view other) {
    return substr(pos, count).compare(other);
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        unicode_string_view other,
                        size_type other_pos,
                        size_type other_count) {
    return substr(pos, count).compare(other.substr(other_pos, other_count));
  }
  constexpr int compare(const wchar_t* other) {
    return ktl::compare(data(), size(), other, ktl::length(other));
  }
  constexpr int compare(size_type pos, size_type count, const wchar_t* other) {
    return substr(pos, count).compare(other);
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        const wchar_t* other,
                        size_type other_pos,
                        size_type other_count) {
    return substr(pos, count)
        .compare(unicode_string_view(other + other_pos, other_count));
  }
  constexpr bool starts_with(wchar_t ch) { return !empty() && front() == ch; }
  constexpr bool starts_with(const wchar_t* str) {
    size_type str_length{static_cast<size_type>(ktl::length(str))};
    return compare(0, str_length, str, 0, str_length) == 0;
  }
  constexpr bool starts_with(unicode_string_view str) {
    return compare(0, str.length(), str) == 0;
  }
  constexpr bool ends_with(wchar_t ch) { return !empty() && back() == ch; }
  constexpr bool ends_with(const wchar_t* str) {
    size_type str_length{static_cast<size_type>(ktl::length(str))};
    if (str_length > length()) {
      return false;
    }
    auto lhs{substr(length() - str_length, str_length)};
    return lhs.compare(0, ktl::length(lhs), str, 0, str_length);
  }
  constexpr bool ends_with(unicode_string_view str) {
    size_type str_length{str.size()};
    if (str_length > length()) {
      return false;
    }
    auto lhs{substr(length() - str_length, str_length)};
    return lhs.compare(str);
  }
  constexpr size_type find(wchar_t ch, size_type pos = 0) {
    return static_cast<size_type>(find_character(substr(pos), ch));
  }

  constexpr size_type find(const wchar_t* str, size_type pos = 0) {
    return static_cast<size_type>(find_substr(substr(pos), str));
  }

  constexpr size_type find(const wchar_t* str,
                           size_t count,
                           size_type pos = 0) {
    auto lhs{substr(pos)};
    return static_cast<size_type>(
        str::details::find_substr_impl(substr(pos), 0, lhs.size(), str, count));
  }
  constexpr size_type find(unicode_string_view str, size_type pos = 0) {
    return static_cast<size_type>(find_substr(substr(pos), str));
  }

 public:
  PUNICODE_STRING raw_str() { return addressof(m_str); }
  PCUNICODE_STRING raw_str() const { return addressof(m_str); }

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
  explicit constexpr operator std::wstring_view() const {
    return std::wstring_view(m_str.Buffer, m_str.Length);
  }
#endif

 private:
  constexpr size_type calc_segment_length(size_type pos,
                                          size_type count) {  // In characters
    return length() <= static_cast<size_type>(count + pos)
               ? length()
               : static_cast<size_type>(count + pos);
  }
  static constexpr UNICODE_STRING make_unicode_string(
      const value_type* str,
      size_type length) noexcept {  // length in characters
    UNICODE_STRING unicode_str{};
    unicode_str.Buffer = dirty::remove_const_from_value(str);
    unicode_str.Length = length_to_bytes(
        length);  // UNICODE_STRING requires length in bytes (NOT in characters)
    unicode_str.MaximumLength = length_to_bytes(length);
    return unicode_str;
  }

  static constexpr size_type length_to_bytes(
      size_type length_in_characters) noexcept {
    return length_in_characters * sizeof(value_type);
  }

  static constexpr size_type length_to_characters(
      size_type bytes_count) noexcept {
    return bytes_count / sizeof(value_type);
  }

 private:
  UNICODE_STRING m_str{};
};

constexpr bool operator==(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) == 0;
}
constexpr bool operator!=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs == rhs);
}
constexpr bool operator<(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) < 0;
}
constexpr bool operator>(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) > 0;
}
constexpr bool operator<=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs > rhs);
}
constexpr bool operator>=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs < rhs);
}

constexpr void swap(unicode_string_view& lhs,
                    unicode_string_view& rhs) noexcept {
  return lhs.swap(rhs);
}
}  // namespace ktl
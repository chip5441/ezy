#ifndef EZY_BITS_EMPTY_SIZE_H_INCLUDED
#define EZY_BITS_EMPTY_SIZE_H_INCLUDED

#include <type_traits>
#include <experimental/type_traits>

namespace ezy
{
  namespace detail
  {
    template <typename T>
    using empty_mem_fn_t = decltype(std::declval<T>().empty());

    template <typename T>
    using empty_free_fn_t = decltype(empty(std::declval<T>()));

    template <typename Range>
    using size_mem_fn_t = decltype(std::declval<Range>().size());
  }

  template <typename...>
  constexpr auto always_false = false;

  template <size_t I> struct priority_tag : priority_tag<I - 1> {};
  template <> struct priority_tag<0>{};

  namespace detail
  {
    template <typename T>
    constexpr auto impl_empty(const T& t, priority_tag<0>) -> bool
    {
      using std::begin;
      using std::end;
      return !(begin(t) != end(t));
    }

    template <typename T>
    constexpr auto impl_empty(const T& t, priority_tag<1>) -> decltype(t.size())
    {
      return t.size() != 0;
    }

    template <typename T>
    constexpr auto impl_empty(const T& t, priority_tag<2>) -> decltype(t.empty())
    {
      return t.empty();
    }

    template <typename T>
    constexpr auto impl_empty(const T& t, priority_tag<3>) -> decltype(empty(t))
    {
      return empty(t);
    }
  }

  struct empty_fn
  {
    template <typename T>
    [[nodiscard]] constexpr auto operator()(const T& t) const
    {
      return detail::impl_empty(t, priority_tag<3>{});
    }
  };

  static constexpr empty_fn empty = {};

  namespace detail
  {
    template <typename T>
    constexpr auto impl_size(const T& t, priority_tag<0>)
    {
      using std::begin;
      using std::end;
      return std::distance(begin(t), end(t));
    }

    template <typename T>
    constexpr auto impl_size(const T& t, priority_tag<1>) -> decltype(size(t))
    {
      return size(t);
    }

    template <typename T>
    constexpr auto impl_size(const T& t, priority_tag<2>) -> decltype(t.size())
    {
      return t.size();
    }

  }

  struct size_fn
  {
    template <typename T>
    [[nodiscard]] constexpr auto operator()(const T& t) const
    {
      return detail::impl_size(t, priority_tag<2>{});
    }
  };

  static constexpr size_fn size = {};
}

#endif

#ifndef STASH_RANGE_H_INCLUDED
#define STASH_RANGE_H_INCLUDED

#include "experimental/tuple_algorithm.h"
#include "experimental/keeper.h"
#include "invoke.h"

#include <type_traits>
#include <utility>
#include <iterator>

#include <cstddef>
#include <tuple>

namespace ezy::detail
{
  template <typename T>
  struct const_iterator_type
  {
    using type = decltype(std::begin(std::declval<const T>()));
  };

  template <typename T>
  using const_iterator_type_t = typename const_iterator_type<T>::type;

  template <typename T>
  struct iterator_type
  {
    using type = decltype(std::begin(std::declval<T>()));
  };

  template <typename T>
  using iterator_type_t = typename iterator_type<T>::type;

  template <typename T>
  struct value_type
  {
    using type = ezy::remove_cvref_t<decltype(*std::begin(std::declval<T>()))>;
  };

  template <typename T>
  using value_type_t = typename value_type<T>::type;

  template <typename T>
  struct const_value_type
  {
    using type = std::remove_reference_t<decltype(*std::begin(std::declval<const T>()))>;
  };

  template <typename T>
  using const_value_type_t = typename const_value_type<T>::type;
}

namespace ezy::detail
{
  template <typename F>
  using IsFunction = typename std::enable_if<std::is_function<F>::value, F>;
}

namespace ezy::detail
{
  template <typename orig_type>
  struct basic_iterator_adaptor
  {
    public:
      using difference_type = typename orig_type::difference_type;
      using value_type = typename orig_type::value_type;
      using pointer = typename orig_type::pointer;
      using reference = typename orig_type::reference;
      using iterator_category = typename orig_type::iterator_category;

      constexpr explicit basic_iterator_adaptor(orig_type original)
          : orig(original)
      {}

      inline constexpr basic_iterator_adaptor& operator++()
      {
          ++orig;
          return *this;
      }

      inline constexpr decltype(auto) operator*()
      {
        return *orig;
      }

      inline constexpr bool operator==(const basic_iterator_adaptor& rhs) const
      { return orig == rhs.orig; }

      inline constexpr bool operator!=(const basic_iterator_adaptor& rhs) const
      { return orig != rhs.orig; }

      inline constexpr auto operator+=(difference_type diff)
      { return orig += diff; }

      inline constexpr auto operator-(const orig_type& rhs)
      { return orig - rhs; }

      inline constexpr auto operator-(const basic_iterator_adaptor& rhs)
      { return orig - rhs.orig; }

      orig_type orig;
  };

  // tag for mark end iterator - experimental
  struct end_marker_t
  {};

  /**
   * iterator tracker is a collection of iterators to Ranges.
   *
   * It's convenient for algorithms which operates multiple range at the same time.
   */
  template <typename... Iters>
  struct iterator_tracker
  {
    public:
      constexpr static auto size = sizeof...(Iters);

      template <typename... Iterators>
      iterator_tracker(Iterators&&... p_iters)
        : iters{std::forward<Iterators>(p_iters)...}
      {}

      template <typename... Ranges>
      static iterator_tracker begin_from_ranges(const Ranges&... ranges)
      {
        using std::begin;
        return iterator_tracker(begin(ranges)...);
      }

      template <typename... Ranges>
      static iterator_tracker end_from_ranges(const Ranges&... ranges)
      {
        using std::end;
        return iterator_tracker(end(ranges)...);
      }

      template <unsigned N>
      constexpr decltype(auto) get() noexcept
      {
        return std::get<N>(iters);
      }

      template <unsigned N>
      constexpr decltype(auto) get() const noexcept
      {
        return std::get<N>(iters);
      }

      template <unsigned N, typename IterT>
      void set_to(IterT&& it)
      {
        get<N>() = std::forward<IterT>(it);
      }

      template <unsigned N>
      decltype(auto) next()
      {
        return ++std::get<N>(iters);
      }

      void next_all()
      {
        ezy::experimental::static_for_each(iters, [](auto& it){ ++it; });
      }

    private:
      std::tuple<Iters...> iters;
  };

  template <typename... Ranges>
  using iterator_tracker_for = iterator_tracker<iterator_type_t<Ranges>...>;

  /**
   * range_tracker is a more heavyweight version of iterator tracker.
   *
   * Alongside with an iterator it stores a reference to the range itself. So it can access the range
   * properties, but it may provide unecessary lazyness.
   *
   * Idea: this should contain {it, end} iterator pairs instead.
   *
   * Since a range tracker itself is able to notice when the iterator reaches the end, it should not be used
   * in an end iterator. But swiching to iterator+sentinel begin/end implementation would cause
   * interoperability problems eg. with standard algorithms, and container constructors whose takes iterator
   * pairs.
   */

  template <typename... Ranges>
  struct range_tracker
  {
    public:
      constexpr static auto size = sizeof...(Ranges);

      range_tracker(const Ranges&... ranges)
        : ranges(ranges...)
        , current(std::begin(ranges)...)
      {}

      range_tracker(const Ranges&... ranges, end_marker_t&&)
        : ranges(ranges...)
        , current(std::end(ranges)...)
      {}

      template <unsigned N>
      auto get()
      {
        using it_type = decltype(std::get<N>(current));
        using end_it_type = decltype(std::get<N>(ranges).get().end());
        return std::pair<it_type, const end_it_type>(std::get<N>(current), std::get<N>(ranges).get().end());
      }

      template <unsigned N>
      auto get() const
      {
        using it_type = decltype(std::get<N>(current));
        using end_it_type = decltype(std::get<N>(ranges).get().end());
        return std::pair<it_type, const end_it_type>(std::get<N>(current), std::get<N>(ranges).get().end());
      }

      template <unsigned N, typename ItT>
      void set_to(ItT it)
      {
        std::get<N>(current) = it;
      }

      template <unsigned N>
      void set_to_end()
      {
        set_to<N>(std::get<N>(ranges).get().end());
      }

      template <unsigned N>
      auto next()
      {
        return ++std::get<N>(current);
      }

      void next_all()
      {
        ezy::experimental::static_for_each(current, [](auto& it){ ++it; });
      }

      template <unsigned N>
      bool has_next() const
      {
        return std::get<N>(current) != std::get<N>(ranges).get().end();
      }

    private:
    public:
      template <typename RangeType>
      using const_it_type = ezy::detail::iterator_type_t<RangeType>; //typename RangeType::const_iterator;

      std::tuple<const std::reference_wrapper<const Ranges>...> ranges;
      std::tuple<const_it_type<Ranges>...> current;
  };


  template <typename orig_type,
           typename converter_type
           // , typename = IsFunction<converter_type>
           >
  struct iterator_adaptor : basic_iterator_adaptor<orig_type>
  {
    public:
      using base = basic_iterator_adaptor<orig_type>;
      using value_type = decltype(*std::declval<orig_type>());
      using result_type = decltype(std::declval<converter_type>()(std::declval<value_type>()));

      // constructor
      using base::basic_iterator_adaptor;

      /*
      iterator_adaptor(orig_type original, converter_type&& c)
        : base(original)
        , converter(std::move(c))
      {}
      */

      constexpr iterator_adaptor(orig_type original, const converter_type& c)
        : base(original)
        , converter(c)
      {}

      inline constexpr iterator_adaptor operator+(int increment) const
      { return iterator_adaptor(base::orig + increment); }

      inline constexpr iterator_adaptor& operator++()
      {
        ++base::orig;
        return *this;
      }

      constexpr result_type operator*()
      {
        return converter(*(base::orig));
      }

    private:
      converter_type converter;
  };

  /**
   * iterator_filter
   */
  template <typename orig_type,
            typename predicate_type
           >
  struct iterator_filter : basic_iterator_adaptor<orig_type>
  {
    public:
      using base = basic_iterator_adaptor<orig_type>;
      using value_type = decltype(*std::declval<orig_type>());
      using difference_type = typename base::difference_type;
      using iterator_category = std::input_iterator_tag; // forward_iterator_tag?

      // constructor
      using base::basic_iterator_adaptor;


      /*
      iterator_filter(orig_type original, predicate_type&& p, const orig_type& end)
        : base(original)
        , converter(std::move(c))
      {}
      */

      iterator_filter(orig_type original, const predicate_type& p, const orig_type& end)
        : base(original)
        , predicate(p)
        , end_iterator(end)
      {
        if (base::orig != end_iterator && !predicate(*base::orig))
          operator++();
      }

      inline iterator_filter& operator++()
      {
        ++base::orig;
        for (; base::orig != end_iterator; ++base::orig)
          if (predicate(*base::orig))
            return *this;

        return *this;
      }

      value_type operator*()
      {
        return *(base::orig);
      }

    private:
      predicate_type predicate;
      const orig_type end_iterator;
  };

  template <typename first_range_type, typename second_range_type>
  struct iterator_concatenator
  {
    public:
      using orig_type = typename first_range_type::const_iterator;
      using value_type = decltype(*std::declval<orig_type>());
      using pointer = std::add_pointer_t<value_type>;
      using reference = std::add_lvalue_reference_t<value_type>;
      using difference_type = typename orig_type::difference_type;
      using iterator_category = std::input_iterator_tag; // forward_iterator_tag?

      // constructor

      iterator_concatenator(const first_range_type& fr, const second_range_type& sr)
        : tracker(fr, sr)
      {}

      iterator_concatenator(const first_range_type& fr, const second_range_type& sr, end_marker_t&&)
        : tracker(fr, sr, end_marker_t{})
      {}

      inline iterator_concatenator& operator++()
      {
        if (auto [it, end] = tracking_info<0>(); it != end)
        {
          ++it;
          return *this;
        }

        if (auto [it, end] = tracking_info<1>(); it != end)
        {
          ++it;
          return *this;
        }

        return *this;
      }

      auto operator*()
      {
        if (auto [it, end] = tracking_info<0>(); it != end)
        {
          return *it;
        }

        auto [it, end] = tracking_info<1>();
        return *it;
      }

      bool operator==(const iterator_concatenator& rhs) const
      {
        if (auto [it, end] = tracking_info<0>(); it != end)
        {
          if (auto [r_it, r_end] = rhs.tracking_info<0>(); r_it != r_end)
            return (r_it == it);
          else
            return false;
        }

        if (auto [it, end] = tracking_info<1>(); it != end)
        {
          if (auto [r_it, r_end] = rhs.tracking_info<1>(); r_it != r_end)
            return (r_it == it);
          else
            return false;
        }

        // this ended
        auto [it, end] = rhs.tracking_info<1>();
        return it == end;
      }

      bool operator!=(const iterator_concatenator& rhs) const
      {
        return !(*this == rhs);
      }

      template <unsigned N>
      auto tracking_info()
      {
        return tracker.template get<N>();
      }

      template <unsigned N>
      auto tracking_info() const
      {
        return tracker.template get<N>();
      }

    private:
      range_tracker<first_range_type, second_range_type> tracker;
  };

  template <typename range_type>
  struct iterator_flattener
  {
    public:
      using orig_iterator = typename range_type::const_iterator;
      using inner_iterator = typename orig_iterator::value_type::const_iterator;

      using value_type = typename inner_iterator::value_type;
      using difference_type = typename inner_iterator::difference_type;
      using pointer = typename inner_iterator::pointer;
      using reference = typename inner_iterator::reference;
      using iterator_category = std::forward_iterator_tag;

      iterator_flattener(const range_type& range)
        : tracker(range)
      {
        if (auto [it, end] = tracker.template get<0>(); it != end) // not empty
          inner = outer()->begin();
      }

      iterator_flattener(const range_type& range, end_marker_t&&)
        : tracker(range, end_marker_t{})
      {
        if (auto [it, end] = tracker.template get<0>(); it != end) // not empty
          inner = outer()->end();
      }

      iterator_flattener& operator++()
      {
        ++inner;
        auto [outer_it, outer_end] = tracker.template get<0>();
        while (outer_it != outer_end && inner == outer_it->end()) // end of current subrange
        {
          ++outer_it;
          if (outer_it != outer_end) // not finished
          {
            inner = outer()->begin();
          }
        }

        return *this;
      }

      const value_type& operator*() const
      {
        return *inner;
      }

      friend bool operator==(const iterator_flattener& lhs, const iterator_flattener& rhs)
      {
        const auto& lhs_tracker = lhs.tracker.template get<0>();
        const auto& rhs_tracker = rhs.tracker.template get<0>();
        if (lhs_tracker.first != rhs_tracker.first)
          return false;

        if (lhs_tracker.first != lhs_tracker.second
            && rhs_tracker.first != rhs_tracker.second
            && lhs.inner != rhs.inner)
          return false;

        return true;
      }

      bool operator!=(const iterator_flattener& rhs) const
      {
        return !(*this == rhs);
      }

      orig_iterator& outer()
      {
        return tracker.template get<0>().first;
      }

    private:
        range_tracker<range_type> tracker;
        inner_iterator inner;
  };

  template <typename Zipper, typename... Ranges>
  struct iterator_zipper
  {
    public:
      using difference_type = size_t; // TODO what should it be?
      using value_type = std::remove_reference_t<decltype(
          ezy::invoke(std::declval<Zipper>(), (*std::begin(std::declval<Ranges>()))...)
          )>;
      using pointer = std::add_pointer_t<value_type>;
      using reference = std::add_lvalue_reference_t<value_type>;
      using iterator_category = std::input_iterator_tag; // forward_iterator_tag?

      using tracker_type = iterator_tracker_for<Ranges...>;

      static constexpr auto cardinality = sizeof...(Ranges);

      iterator_zipper(Zipper zipper, const Ranges&... rs)
        : storage(zipper, tracker_type::begin_from_ranges(rs...))
      {}

      iterator_zipper(Zipper zipper, const Ranges&... rs, end_marker_t&&)
        : storage(zipper, tracker_type::end_from_ranges(rs...))
      {}

    private:
      static constexpr size_t zipper_index{0};
      static constexpr size_t tracker_index{1};

      constexpr decltype(auto) tracker()
      {
        return std::get<tracker_index>(storage);
      }

      constexpr decltype(auto) tracker() const
      {
        return std::get<tracker_index>(storage);
      }

      constexpr decltype(auto) zipper()
      {
        return std::get<zipper_index>(storage);
      }

      template <size_t... Is>
      auto deref_helper(std::index_sequence<Is...>)
      {
        return zipper()(
            (*(tracker().template get<Is>()))...
            );
      }

      template <size_t... Is>
      static bool has_next_helper(const tracker_type& tracker, const tracker_type& rhs, std::index_sequence<Is...>)
      {
        return ((tracker.template get<Is>() != rhs.template get<Is>()) && ...);
      }

    public:
      auto operator*()
      {
        return deref_helper(std::make_index_sequence<cardinality>());
      }

      iterator_zipper& operator++()
      {
        tracker().next_all();
        return *this;
      }

      bool operator!=(const iterator_zipper& rhs) const
      {
        return has_next_helper(tracker(), rhs.tracker(), std::make_index_sequence<cardinality>());
      }

      bool operator==(const iterator_zipper& rhs) const
      {
        return !(*this != rhs);
      }

    private:
      std::tuple<Zipper, tracker_type> storage;
  };

/*
  template <typename orig_type>
  struct iterator_group_adaptor : basic_iterator_adaptor<orig_type>
  {
      using base = basic_iterator_adaptor<orig_type>;
      using orig_value_type = decltype(*std::declval<orig_type>());
      using value_type = range_view_slice<orig_value_type>;
      using difference_type = typename base::difference_type;
      using iterator_category = std::input_iterator_tag; // forward_iterator_tag?

      iterator_group_adaptor(const orig_type& original, const orig_type::difference_type gs, const orig_type& end)
        : base(original)
        , group_size(gs)
        , current_position(0)
        , end_iterator(end)
    {
    }

    inline iterator_group_adaptor& operator++()
    {
      const auto& target_position = (current_position + group_size)
      for (; (current_position < target_position); ++base::orig)
        if (base::orig == end_iterator)
          return *this;

      return *this;
    }

    value_type operator*()
    {
      return range_view_slice<orig_value_type>(base::orig);
    }


    private:
      const orig_type::difference_type group_size;
      orig_type::difference_type current_position;
      const orig_type end_iterator;
  }
  */

  template <typename RangeType>
  struct take_iterator
  {
    public:
      using difference_type = typename RangeType::const_iterator::difference_type;
      using value_type = typename RangeType::const_iterator::value_type;
      using pointer = typename RangeType::const_iterator::pointer;
      using reference = typename RangeType::const_iterator::reference;
      using iterator_category = std::forward_iterator_tag; // ??

      static typename RangeType::size_type
      minimum_n(typename RangeType::size_type n, const RangeType& range)
      {
        return std::min<decltype(n)>(n, std::distance(std::begin(range), std::end(range)));
      }

      explicit take_iterator(const RangeType& range, typename RangeType::size_type n)
        : tracker(range)
        , n(minimum_n(n, range))
      {}

      explicit take_iterator(const RangeType& range, end_marker_t)
        : tracker(range)
        , n(0)
      {}

      inline take_iterator& operator++()
      {
        tracker.template next<0>();
        --n;
        return *this;
      }

      decltype(auto) operator*()
      {
        return *(tracker.template get<0>().first);
      }

      bool operator!=(const take_iterator&) const
      {
        return n != 0;
      }

      bool operator==(const take_iterator& rhs) const
      {
        return !(*this != rhs);
      }

    private:
      range_tracker<RangeType> tracker;
      typename RangeType::size_type n;
  };

  template <typename RangeType, typename Predicate>
  struct take_while_iterator
  {
    public:
      using difference_type = typename RangeType::const_iterator::difference_type;
      using value_type = typename RangeType::const_iterator::value_type;
      using pointer = typename RangeType::const_iterator::pointer;
      using reference = typename RangeType::const_iterator::reference;
      using iterator_category = std::forward_iterator_tag; // ??

      explicit take_while_iterator(const RangeType& range, Predicate p)
        : tracker(range)
        , predicate(std::move(p))
      {
        if (auto [it, end] = tracker.template get<0>(); it != end && !predicate(*it))
          tracker.template set_to<0>(end);
      }

      explicit take_while_iterator(const RangeType& range, Predicate p, end_marker_t)
        : tracker(range)
        , predicate(std::move(p))
      {
        const auto [it, end] = tracker.template get<0>();
        tracker.template set_to<0>(end);
      }

      inline take_while_iterator& operator++()
      {
        tracker.template next<0>();
        const auto [it, end] = tracker.template get<0>();
        if (it == end)
          return *this;

        if (!predicate(*it))
          tracker.template set_to<0>(end);

        return *this;
      }

      decltype(auto) operator*()
      {
        return *(tracker.template get<0>().first);
      }

      bool operator!=(const take_while_iterator&) const
      {
        // TODO fix it
        const auto [it, end] = tracker.template get<0>();
        return it != end;
      }

      bool operator==(const take_while_iterator& rhs) const
      {
        return !(*this != rhs);
      }

    private:
      range_tracker<RangeType> tracker;
      Predicate predicate;
  };

  // TODO
  template <typename... Ranges>
  struct range_reference
  {
    std::tuple<std::add_const_t<Ranges>...> ranges;
  };

  /**
   * basic_range_view
   */
  template <typename Range>
  struct basic_range_view
  {
    using const_iterator = basic_iterator_adaptor<typename Range::const_iterator>;

    constexpr basic_range_view(const Range& orig)
      : orig_range(orig)
    {}

    const Range& orig_range;
  };

  /**
   * range_view
   */
  template <typename KeeperCategory, typename Range, typename Transformation>
  struct range_view
  {
    using _orig_const_iterator = decltype(std::cbegin(std::declval<Range>()));
    using const_iterator = iterator_adaptor<_orig_const_iterator, Transformation>;

    constexpr const_iterator begin() const
    { return const_iterator(orig_range.get().begin(), transformation); }

    constexpr const_iterator end() const
    { return const_iterator(orig_range.get().end(), transformation); }

    ezy::experimental::basic_keeper<KeeperCategory, Range> orig_range;
    Transformation transformation;
  };

  /**
   * range_view_filter
   */
  template <typename KeeperCategory, typename Range, typename FilterPredicate>
  struct range_view_filter
  {
    using const_iterator = iterator_filter<typename Range::const_iterator, FilterPredicate>;

    const_iterator begin() const
    { return const_iterator(orig_range.get().begin(), predicate, orig_range.get().end()); }

    const_iterator end() const
    { return const_iterator(orig_range.get().end(), predicate, orig_range.get().end()); }

    ezy::experimental::basic_keeper<KeeperCategory, Range> orig_range;
    FilterPredicate predicate;
  };

  /**
   * range_view_slice
   */
  template <typename KeeperCategory, typename Range>
  struct range_view_slice
  {
    public:
      using const_iterator = typename Range::const_iterator;
      using difference_type = typename const_iterator::difference_type;

      using Keeper = ezy::experimental::basic_keeper<KeeperCategory, Range>;

      range_view_slice(Keeper&& orig, difference_type f, difference_type u)
        : orig_range(std::move(orig))
        , from(f)
        , until(u)
      {
        if (from > until)
          throw std::logic_error("logic error"); // programming error
      }

      const_iterator begin() const
      { return const_iterator(std::next(orig_range.get().begin(), bounded(from))); }

      const_iterator end() const
      { return const_iterator(std::next(orig_range.get().begin(), bounded(until))); }

    private:
      difference_type get_range_size() const
      {
        return std::distance(orig_range.get().begin(), orig_range.get().end());
      }

      difference_type bounded(difference_type difference) const
      {
        return std::min(get_range_size(), difference);
      }

      Keeper orig_range;
      const difference_type from;
      const difference_type until;
  };

  template <typename Range>
  struct group_range_view : basic_range_view<Range>
  {
    using base = basic_range_view<Range>;
    using const_iterator = typename Range::const_iterator;
    using difference_type = typename const_iterator::difference_type;

    group_range_view(const Range& orig, difference_type size)
      : base(orig)
      , group_size(size)
    {
    }

    const difference_type group_size;
  };

  // TODO generalize to N ranges (N > 0)
  template <typename KeeperCategory1, typename RangeType1, typename KeeperCategory2, typename RangeType2>
  struct concatenated_range_view
  {
    public:
      using const_iterator = iterator_concatenator<RangeType1, RangeType2>;
      using difference_type = typename const_iterator::difference_type;

      using Keeper1 = ezy::experimental::basic_keeper<KeeperCategory1, RangeType1>;
      using Keeper2 = ezy::experimental::basic_keeper<KeeperCategory2, RangeType2>;

      /*
      concatenated_range_view(Keeper1&& k1, const RangeType2& r2)
        : range1(std::move(k1))
        , range2(r2)
      {}
      */

      const_iterator begin() const
      { return const_iterator(range1.get(), range2.get()); }

      const_iterator end() const
      { return const_iterator(range1.get(), range2.get(), end_marker_t{}); }

    public:
    //private:
      Keeper1 range1;
      Keeper2 range2;
  };

  template <typename Zipper, typename... Keepers>
  struct zip_range_view
  {
    public:
      using KeepersTuple = std::tuple<Keepers...>;

      using const_iterator = iterator_zipper<Zipper, ezy::experimental::detail::keeper_value_type_t<Keepers>...>;
      using difference_type = typename const_iterator::difference_type;
      using value_type = typename const_iterator::value_type;

      template <typename UZipper> // universal
      zip_range_view(UZipper&& zipper, Keepers&&... keepers)
        : zipper(std::forward<UZipper>(zipper))
        , keepers{std::move(keepers)...}
      {}

      template <size_t... Is>
      static const_iterator get_begin_helper(Zipper zipper, const KeepersTuple& keepers, std::index_sequence<Is...>)
      {
        return const_iterator(zipper, std::get<Is>(keepers).get()...);
      }
      static const_iterator get_begin(Zipper zipper, const KeepersTuple& keepers)
      {
        return get_begin_helper(zipper, keepers, std::make_index_sequence<std::tuple_size_v<KeepersTuple>>());
      }

      template <size_t... Is>
      static const_iterator get_end_helper(Zipper zipper, const KeepersTuple& keepers, std::index_sequence<Is...>)
      {
        return const_iterator(zipper, std::get<Is>(keepers).get()..., end_marker_t{});
      }
      static const_iterator get_end(Zipper zipper, const KeepersTuple& keepers)
      {
        return get_end_helper(zipper, keepers, std::make_index_sequence<std::tuple_size_v<KeepersTuple>>());
      }

      const_iterator begin() const
      { return get_begin(zipper, keepers); }

      auto end() const
      { return get_end(zipper, keepers); }

    public:
    //private:
      Zipper zipper;
      KeepersTuple keepers;
  };

  template <typename KeeperCategory, typename RangeType>
  struct flattened_range_view
  {
    public:
      using const_iterator = iterator_flattener<RangeType>;

      using value_type = typename const_iterator::value_type;
      using pointer = typename const_iterator::pointer;
      using reference = typename const_iterator::reference;
      using difference_type = typename const_iterator::difference_type;

      const_iterator begin() const
      {
        return const_iterator(range.get());
      }

      const_iterator end() const
      {
        return const_iterator(range.get(), end_marker_t{});
      }

    //private:
      ezy::experimental::basic_keeper<KeeperCategory, RangeType> range;
  };

  // some common view adaptors
  template <size_t N>
  struct pick_nth_t
  {
    template <typename ValueType>
    auto operator()(const ValueType& value)
    { return std::get<N>(value); }
  };

  /*
  inline constexpr pick_nth_t<0> pick_first;
  inline constexpr pick_nth_t<1> pick_second;
  */

  using pick_first = pick_nth_t<0>;
  using pick_second = pick_nth_t<1>;

  template <typename KeeperCategory, typename RangeType>
  struct take_n_range_view
  {
    public:
      using const_iterator = take_iterator<RangeType>;
      using size_type = typename RangeType::size_type;

      using Keeper = ezy::experimental::basic_keeper<KeeperCategory, RangeType>;

      const_iterator begin() const
      {
        return const_iterator(range.get(), n);
      }

      const_iterator end() const
      {
        return const_iterator(range.get(), end_marker_t{});
      }

      Keeper range;
      size_type n;
  };

  template <typename KeeperCategory, typename RangeType, typename Predicate>
  struct take_while_range_view
  {
    public:
      using const_iterator = take_while_iterator<RangeType, Predicate>;
      using size_type = typename RangeType::size_type;

      using Keeper = ezy::experimental::basic_keeper<KeeperCategory, RangeType>;

      const_iterator begin() const
      {
        return const_iterator(range.get(), pred);
      }

      const_iterator end() const
      {
        return const_iterator(range.get(), pred, end_marker_t{});
      }

      Keeper range;
      Predicate pred;
  };

}

#endif

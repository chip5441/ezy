#ifndef EZY_FEATURES_ARITHMETIC_H_INCLUDED
#define EZY_FEATURES_ARITHMETIC_H_INCLUDED
#include "../strong_type.h"

#include <experimental/type_traits>

namespace ezy::features
{

  namespace detail
  {
    template <typename T>
    using operator_ne_t = decltype(std::declval<T>() != std::declval<T>());
  }

  template <typename T>
  struct addable : crtp<T, addable>
  {
    T operator+(T const& other) const { return T(this->that().get() + other.get()); }
    T& operator+=(T const& other)
    {
      this->that().get() += other.get();
      return this->that();
    }
  };

  template <typename T>
  struct subtractable : crtp<T, subtractable>
  {
    T operator-(T const& other) const { return T(this->that().get() - other.get()); }
    T& operator-=(T const& other)
    {
      this->that().get() -= other.get();
      return this->that();
    }
  };

  template <typename T>
  struct additive : addable<T>, subtractable<T> {};

  template <typename T>
  struct equal_comparable : crtp<T, equal_comparable>
  {
    bool operator==(const T& rhs) const
    {
      return this->that().get() == rhs.get();
    }

    bool operator!=(const T& rhs) const
    {
      if constexpr (std::experimental::is_detected_v<ezy::features::detail::operator_ne_t, typename T::type>)
      {
        return this->that().get() != rhs.get();
      }
      else
      {
        return !(this->that() == rhs);
      }
    }
  };

  template <typename T>
  struct greater : crtp<T, greater>
  {
    bool operator>(const T& rhs) const
    {
      return this->that().get() > rhs.get();
    }
  };

  template <typename T>
  struct greater_equal : crtp<T, greater_equal>
  {
    bool operator>=(const T& rhs) const
    {
      return this->that().get() >= rhs.get();
    }
  };

  template <typename T>
  struct less : crtp<T, less>
  {
    bool operator<(const T& rhs) const
    {
      return this->that().get() < rhs.get();
    }
  };

  template <typename T>
  struct less_equal : crtp<T, less_equal>
  {
    bool operator<=(const T& rhs) const
    {
      return this->that().get() <= rhs.get();
    }
  };

  // nicer solution?
  template <typename N>
  struct multipliable_by
  {
    template <typename T>
    struct internal : crtp<T, internal>
    {
      using numtype = N;
      T operator*(numtype other) const { return T(this->that().get() * other); }
      T& operator*=(numtype other)
      {
        this->that().get() *= other;
        return this->that();
      }
    };
  };

  template <typename N>
  struct divisible_by
  {
    template <typename T>
    struct internal : crtp<T, internal>
    {
      using numtype = N;
      T operator/(numtype other) const { return T(this->that().get() / other); }
      T& operator/=(numtype other)
      {
        this->that().get() /= other;
        return this->that();
      }
    };
  };

  template <typename T>
  using divisible_by_int = divisible_by<int>::internal<T>;

  template <typename N>
  struct multiplicative_by
  {
    template <typename T>
    struct internal : multipliable_by<N>::template internal<T>, divisible_by<N>::template internal<T>
    {
      //using numtype = N;
    };
  };

  template <typename T>
  using multiplicative_by_int = multiplicative_by<int>::internal<T>;

}

#endif

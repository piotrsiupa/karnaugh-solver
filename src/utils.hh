#pragma once

#include <iterator>
#include <utility>


// It's like `std::remove_if` but the predicate operates on both values and indexes instead of just values.
template<class ForwardIt, class UnaryPredicate>
ForwardIt remove_if_index(const ForwardIt first, const ForwardIt last, const UnaryPredicate predicate)
{
    ForwardIt dest = first;
    for (ForwardIt i = first; i != last; ++i)
        if (!predicate(std::as_const(*i), std::distance(first, i)))
            *dest++ = std::move(*i);
    return dest;
}

// libycxx core: std::default_searcher ([func.search.default]), declared by <functional> and built
// on the search algorithm. The Boyer-Moore searchers need hashing containers and are not
// provided yet.
#pragma once

#include <ycxx/core/algo_nonmod.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class ForwardIterator1, class BinaryPredicate = equal_to<>>
class default_searcher {
  ForwardIterator1 pat_first_;
  ForwardIterator1 pat_last_;
  BinaryPredicate pred_;

public:
  constexpr default_searcher(ForwardIterator1 pat_first, ForwardIterator1 pat_last,
                             BinaryPredicate pred = BinaryPredicate())
      : pat_first_(pat_first), pat_last_(pat_last), pred_(pred) {}

  template <class ForwardIterator2>
  constexpr pair<ForwardIterator2, ForwardIterator2> operator()(ForwardIterator2 first, ForwardIterator2 last) const {
    BinaryPredicate pred = pred_; // std::search takes its predicate by value
    return ::ycxx::detail::search_impl(first, last, pat_first_, pat_last_, ::ycxx::detail::ref_pred(pred));
  }
};

} // namespace std

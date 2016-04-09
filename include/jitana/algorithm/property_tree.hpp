/*
 * Copyright (c) 2016, Yutaka Tsutano
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef JITANA_PROPERTY_TREE_HPP
#define JITANA_PROPERTY_TREE_HPP

#include <boost/property_tree/ptree.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/range/iterator_range.hpp>

namespace jitana {
    auto child_elements(const boost::property_tree::ptree& pt,
                        const std::string& element_name)
    {
        auto pred = [&](const auto& x) { return x.first == element_name; };
        auto fit_begin = boost::make_filter_iterator(pred, begin(pt), end(pt));
        auto fit_end = boost::make_filter_iterator(pred, end(pt), end(pt));
        return boost::make_iterator_range(fit_begin, fit_end);
    }
}

#endif

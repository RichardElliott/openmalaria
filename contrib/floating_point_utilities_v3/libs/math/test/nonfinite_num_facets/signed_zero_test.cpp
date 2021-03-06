// Copyright (c) 2006 Johan Rade

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#   pragma warning(disable : 4702)
#endif

#include <iomanip>
#include <locale>
#include <sstream>
#include <boost/test/auto_unit_test.hpp>
#include "almost_equal.hpp"
#include "S_.hpp"
#include "../../../../boost/math/nonfinite_num_facets.hpp"

namespace {

// the anonymous namespace resolves ambiguities on platforms
// with fpclassify etc functions at global scope

using namespace boost::math;
using boost::math::signbit;
using boost::math::changesign;
using boost::math::isnan;

//------------------------------------------------------------------------------

template<class CharType, class ValType> void signed_zero_test_impl();

BOOST_AUTO_TEST_CASE(signed_zero_test)
{
    signed_zero_test_impl<char, float>();
    signed_zero_test_impl<char, double>();
    signed_zero_test_impl<char, long double>();
    signed_zero_test_impl<wchar_t, float>();
    signed_zero_test_impl<wchar_t, double>();
    signed_zero_test_impl<wchar_t, long double>();
}

template<class CharType, class ValType> void signed_zero_test_impl()
{
    std::locale old_locale;
    std::locale tmp_locale(
        old_locale, new nonfinite_num_put<CharType>(signed_zero));
    std::locale new_locale(tmp_locale, new nonfinite_num_get<CharType>);

    std::basic_stringstream<CharType> ss;
    ss.imbue(new_locale);

    ValType a1 = static_cast<ValType>(0);
    ValType a2 = (changesign)(static_cast<ValType>(0));

    ss << a1 << ' ' << a2;

    std::basic_string<CharType> s = S_("0 -0");
    BOOST_CHECK(ss.str() == s);

    ValType b1, b2;
    ss >> b1 >> b2;

    BOOST_CHECK(b1 == a1);
    BOOST_CHECK(b2 == a2);
    BOOST_CHECK(!(signbit)(b1));
    BOOST_CHECK((signbit)(b2));
    BOOST_CHECK(ss.rdstate() == std::ios_base::eofbit);
}

//------------------------------------------------------------------------------

}   // anonymous namespace

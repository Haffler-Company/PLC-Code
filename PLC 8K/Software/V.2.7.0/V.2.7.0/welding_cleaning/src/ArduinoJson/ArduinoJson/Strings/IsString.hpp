// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2022, Benoit BLANCHON
// MIT License

#pragma once

#include "../Polyfills/type_traits.hpp"

namespace ARDUINOJSON_NAMESPACE
{

    template <typename T, typename Enable = void>
    struct IsString : false_type
    {
    };

    template <typename TChar>
    struct IsString<const TChar *> : IsString<TChar *>
    {
    };

} // namespace ARDUINOJSON_NAMESPACE

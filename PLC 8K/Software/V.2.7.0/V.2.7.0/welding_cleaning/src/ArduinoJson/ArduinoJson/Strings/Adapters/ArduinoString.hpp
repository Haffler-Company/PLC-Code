// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2022, Benoit BLANCHON
// MIT License

#pragma once

#include <Arduino.h>

#include "../../Strings/Adapters/RamString.hpp"
#include "../../Strings/IsString.hpp"

namespace ARDUINOJSON_NAMESPACE
{

  inline SizedRamString adaptString(const ::String &s)
  {
    return SizedRamString(s.c_str(), s.length());
  }

  template <>
  struct IsString<::String> : true_type
  {
  };

  template <>
  struct IsString<::StringSumHelper> : true_type
  {
  };

} // namespace ARDUINOJSON_NAMESPACE

// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2022, Benoit BLANCHON
// MIT License

#pragma once

#include "../../Strings/Adapters/RamString.hpp"
#include "../../Strings/IsString.hpp"
#include "../../Strings/String.hpp"

namespace ARDUINOJSON_NAMESPACE
{

  inline SizedRamString adaptString(const String &s)
  {
    return SizedRamString(s.c_str(), s.size());
  }

  template <>
  struct IsString<String> : true_type
  {
  };

} // namespace ARDUINOJSON_NAMESPACE

//
// Copyright 2014 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include <type_traits>

#include "details/OptionsParser.h"
#include "IntValue.h"

namespace comms
{

namespace field
{

/// @addtogroup comms
/// @{

/// @brief Defines "Enum Value Field".
/// @details The class uses ComplexIntValue as its underlying type while providing
///          additional API function to check range of the value.
/// @tparam TField Base (interface) class for this field.
/// @tparam TEnum Enum type.
/// @tparam TLen Length of serialised data in bytes. The default value is
///         sizeof(std::underlying_type<TEnum>::type).
/// @tparam TLimit Maximal and invalid value. All other values below the TLimit
///         are considered to be valid.
/// @headerfile comms/field/EnumValue.h
template <typename TFieldBase, typename TEnum, typename... TOptions>
class EnumValue : public TFieldBase
{
    static_assert(std::is_enum<TEnum>::value, "TEnum must be enum type");
    typedef TFieldBase Base;
public:

    typedef details::OptionsParser<TOptions...> ParsedOptions;

    /// @brief Type of the stored value
    typedef TEnum ValueType;

    /// @brief Underlying type
    typedef typename std::underlying_type<ValueType>::type UnderlyingType;

    /// @brief Definition of underlying ComplexIntValue field type
    typedef
        IntValue<
            Base,
            UnderlyingType,
            TOptions...
        > IntValueField;


    /// @brief Default constructor.
    /// @brief Initial value is equal to LimitValue
    EnumValue() = default;

    /// @brief Constructor
    explicit EnumValue(ValueType value)
      : intValue_(static_cast<UnderlyingType>(value))
    {
    }

    /// @brief Copy constructor is default
    EnumValue(const EnumValue&) = default;

    /// @brief Destructor is default
    ~EnumValue() = default;

    /// @brief Copy assignment is default
    EnumValue& operator=(const EnumValue&) = default;

    constexpr const ValueType getValue() const
    {
        return static_cast<ValueType>(intValue_.getValue());
    }

    /// @copydoc ComplexIntValue::setValue()
    void setValue(ValueType value)
    {
        intValue_.setValue(static_cast<UnderlyingType>(value));
    }

    /// @copydoc ComplexIntValue::length()
    constexpr std::size_t length() const
    {
        return intValue_.length();
    }

    static constexpr std::size_t minLength()
    {
        return IntValueField::minLength();
    }

    static constexpr std::size_t maxLength()
    {
        return IntValueField::maxLength();
    }

    template <typename TIter>
    ErrorStatus read(TIter& iter, std::size_t size)
    {
        return intValue_.read(iter, size);
    }

    template <typename TIter>
    ErrorStatus write(TIter& iter, std::size_t size) const
    {
        return intValue_.write(iter, size);
    }

    bool valid() const
    {
        return intValue_.valid();
    }

private:

    IntValueField intValue_;
};

// Implementation

/// @brief Equality comparison operator.
/// @related EnumValue
template <typename... TArgs>
bool operator==(
    const EnumValue<TArgs...>& field1,
    const EnumValue<TArgs...>& field2)
{
    return field1.getValue() == field2.getValue();
}

/// @brief Non-equality comparison operator.
/// @related EnumValue
template <typename... TArgs>
bool operator!=(
    const EnumValue<TArgs...>& field1,
    const EnumValue<TArgs...>& field2)
{
    return field1.getValue() != field2.getValue();
}

/// @brief Equivalence comparison operator.
/// @related EnumValue
template <typename... TArgs>
bool operator<(
    const EnumValue<TArgs...>& field1,
    const EnumValue<TArgs...>& field2)
{
    return field1.getValue() < field2.getValue();
}

namespace details
{

template <typename T>
struct IsEnumValue
{
    static const bool Value = false;
};

template <typename... TArgs>
struct IsEnumValue<comms::field::EnumValue<TArgs...> >
{
    static const bool Value = true;
};

}  // namespace details

template <typename T>
constexpr bool isEnumValue()
{
    return details::IsEnumValue<T>::Value;
}


/// @}

}  // namespace field

}  // namespace comms






//
// Copyright 2015 - 2017 (C). Alex Robenko. All rights reserved.
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

#include <cstdint>
#include <tuple>

#include "comms/options.h"


namespace comms
{

namespace details
{

template <typename... TOptions>
class MessageInterfaceOptionsParser;

template <>
class MessageInterfaceOptionsParser<>
{
public:
    static const bool HasMsgIdType = false;
    static const bool HasEndian = false;
    static const bool HasReadIterator = false;
    static const bool HasWriteIterator = false;
    static const bool HasMsgIdInfo = false;
    static const bool HasHandler = false;
    static const bool HasValid = false;
    static const bool HasLength = false;
    static const bool HasRefresh = false;
    static const bool HasNoVirtualDestructor = false;
};

template <typename T, typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::MsgIdType<T>,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    using MsgIdType = T;
    static const bool HasMsgIdType = true;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::IdInfoInterface,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasMsgIdInfo = true;
};

template <typename TEndian, typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::Endian<TEndian>,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasEndian = true;
    using Endian = TEndian;
};

template <typename TIter, typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::ReadIterator<TIter>,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasReadIterator = true;
    using ReadIterator = TIter;
};

template <typename TIter, typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::WriteIterator<TIter>,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasWriteIterator = true;
    using WriteIterator = TIter;
};

template <typename T, typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::Handler<T>,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasHandler = true;
    using Handler = T;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::ValidCheckInterface,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasValid = true;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::LengthInfoInterface,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasLength = true;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::RefreshInterface,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasRefresh = true;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::NoVirtualDestructor,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
public:
    static const bool HasNoVirtualDestructor = true;
};

template <typename... TOptions>
class MessageInterfaceOptionsParser<
    comms::option::EmptyOption,
    TOptions...> : public MessageInterfaceOptionsParser<TOptions...>
{
};

template <typename... TBundledOptions, typename... TOptions>
class MessageInterfaceOptionsParser<
    std::tuple<TBundledOptions...>,
    TOptions...> : public MessageInterfaceOptionsParser<TBundledOptions..., TOptions...>
{
};



}  // namespace details

}  // namespace comms



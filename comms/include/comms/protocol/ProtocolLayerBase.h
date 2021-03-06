//
// Copyright 2014 - 2017 (C). Alex Robenko. All rights reserved.
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

#include <tuple>
#include <utility>
#include <algorithm>

#include "comms/ErrorStatus.h"
#include "comms/util/Tuple.h"
#include "comms/Assert.h"

namespace comms
{

namespace protocol
{

namespace details
{

template <class T, class R = void>
struct ProtocolLayerEnableIfHasAllMessages { using Type = R; };

template <class T, class Enable = void>
struct ProtocolLayerAllMessagesHelper
{
    using Type = void;
};

template <class T>
struct ProtocolLayerAllMessagesHelper<T, typename ProtocolLayerEnableIfHasAllMessages<typename T::AllMessages>::Type>
{
    using Type = typename T::AllMessages;
};

template <class T>
using ProtocolLayerAllMessagesType = typename ProtocolLayerAllMessagesHelper<T>::Type;

template <class T, class R = void>
struct ProtocolLayerEnableIfHasInterfaceOptions { using Type = R; };

template <class T, class Enable = void>
struct ProtocolLayerHasInterfaceOptions
{
    static const bool Value = false;
};

template <class T>
struct ProtocolLayerHasInterfaceOptions<T, typename ProtocolLayerEnableIfHasInterfaceOptions<typename T::InterfaceOptions>::Type>
{
    static const bool Value = true;
};

template <class T, class R = void>
struct ProtocolLayerEnableIfHasImplOptions { using Type = R; };

template <class T, class Enable = void>
struct ProtocolLayerHasImplOptions
{
    static const bool Value = false;
};

template <class T>
struct ProtocolLayerHasImplOptions<T, typename ProtocolLayerEnableIfHasImplOptions<typename T::ImplOptions>::Type>
{
    static const bool Value = true;
};

template <typename T, bool THasImpl>
struct ProtocolLayerHasFieldsImplHelper;

template <typename T>
struct ProtocolLayerHasFieldsImplHelper<T, true>
{
    static const bool Value = T::ImplOptions::HasFieldsImpl;
};

template <typename T>
struct ProtocolLayerHasFieldsImplHelper<T, false>
{
    static const bool Value = false;
};

template <typename T>
struct ProtocolLayerHasFieldsImpl
{
    static const bool Value =
        ProtocolLayerHasFieldsImplHelper<T, ProtocolLayerHasImplOptions<T>::Value>::Value;
};

template <typename T, bool THasImpl>
struct ProtocolLayerHasDoGetIdHelper;

template <typename T>
struct ProtocolLayerHasDoGetIdHelper<T, true>
{
    static const bool Value = T::ImplOptions::HasStaticMsgId;
};

template <typename T>
struct ProtocolLayerHasDoGetIdHelper<T, false>
{
    static const bool Value = false;
};

template <typename T>
struct ProtocolLayerHasDoGetId
{
    static const bool Value =
        ProtocolLayerHasDoGetIdHelper<T, ProtocolLayerHasImplOptions<T>::Value>::Value;
};

template <class T, class R = void>
struct ProtocolLayerEnableIfHasMsgPtr { using Type = R; };

template <class T, class Enable = void>
struct ProtocolLayerMsgPtr
{
    using Type = void;
};

template <class T>
struct ProtocolLayerMsgPtr<T, typename ProtocolLayerEnableIfHasMsgPtr<typename T::MsgPtr>::Type>
{
    using Type = typename T::MsgPtr;
};


}  // namespace details

/// @brief Base class for all the middle (non @ref MsgDataLayer) protocol transport layers.
/// @details Provides all the default and/or common functionality for the
///     middle transport layer. The inheriting actual layer class may
///     use and/or override the provided functionality by redefining member
///     function with the same signature. The @ref NextLayer is stored as a private
///     data meber.
/// @tparam TField Every middle layer will have a field containing extra
///     information for this layer. This template parameter is a type of such
///     field.
/// @tparam TNextLayer Next layer this one wraps and forwards the calls to.
/// @tparam TDerived Actual protocol layer class that extends this one.
/// @headerfile comms/protocol/ProtocolLayerBase.h
template <typename TField, typename TNextLayer, typename TDerived>
class ProtocolLayerBase
{
public:
    /// @brief Type of the field used for this layer.
    using Field = TField;

    /// @brief Type of the next transport layer
    using NextLayer = TNextLayer;

    /// @brief Type of all the fields of all the transport layers
    ///     wrapped in std::tuple.
    /// @details The @ref Field type is prepended to the @ref AllFields type
    ///     of the @ref NextLayer and reported as @ref AllFields of this one.
    using AllFields = typename std::decay<
        decltype(
            std::tuple_cat(
                std::declval<std::tuple<Field> >(),
                std::declval<typename TNextLayer::AllFields>())
            )
        >::type;

    /// @brief All supported messages.
    /// @details Same as NextLayer::AllMessages or void if such doesn't exist.
    using AllMessages = details::ProtocolLayerAllMessagesType<NextLayer>;

    /// @brief Type of pointer to the message.
    /// @details Same as NextLayer::MsgPtr or void if such doesn't exist.
    using MsgPtr = typename details::ProtocolLayerMsgPtr<NextLayer>::Type;

    /// @brief Static constant indicating amount of transport layers used.
    static const std::size_t NumOfLayers = 1 + NextLayer::NumOfLayers;

    /// @brief Copy constructor
    ProtocolLayerBase(const ProtocolLayerBase&) = default;

    /// @brief Move constructor
    ProtocolLayerBase(ProtocolLayerBase&&) = default;

    /// @brief Constructor.
    /// @details Forwards all the parameters to the constructor of the embedded
    ///     @ref NextLayer object.
    /// @param args Arguments to be passed to the constructor of the next layer
    template <typename... TArgs>
    explicit ProtocolLayerBase(TArgs&&... args)
      : nextLayer_(std::forward<TArgs>(args)...)
    {
    }

    /// @brief Desctructor
    ~ProtocolLayerBase() noexcept = default;

    /// @brief Copy assignment
    ProtocolLayerBase& operator=(const ProtocolLayerBase&) = default;

    /// @brief Get access to the next layer object.
    NextLayer& nextLayer()
    {
        return nextLayer_;
    }

    /// @brief Get "const" access to the next layer object.
    const NextLayer& nextLayer() const
    {
        return nextLayer_;
    }

    /// @brief Deserialise message from the input data sequence.
    /// @details The function will invoke @b doRead() member function
    ///     provided by the derived class, which must have the following signature
    ///     and logic:
    ///     @code
    ///         template<typename TMsgPtr, typename TIter, typename TNextLayerReader>
    ///         comms::ErrorStatus doRead(
    ///             Field& field, // field object used to read required data
    ///             TMsgPtr& msgPtr, // smart pointer to message object
    ///             TIter& iter, // iterator used for reading
    ///             std::size_t size, // size of the remaining data
    ///             std::size_t* missingSize, // output of missing bytest count
    ///             TNextLayerReader&& nextLayerReader // next layer reader object
    ///             )
    ///         {
    ///             // internal logic prior next layer read, such as reading the field value
    ///             auto es = field.read(msgPtr, iter, size, missingSize);
    ///             ...
    ///             // request next layer to perform read operation
    ///             es = nextLayerReader.read(msgPtr, iter, size - field.length(), missingSize);
    ///             ... // internal logic after next layer read if applicable
    ///             return es;
    ///         };
    ///     @endcode
    ///     The signature of the @b nextLayerReader.read() function is
    ///     the same as the signature of this @b read() member function.
    /// @tparam TMsgPtr Type of smart pointer that holds message object.
    /// @tparam TIter Type of iterator used for reading.
    /// @param[in, out] msgPtr Reference to smart pointer that will hold
    ///                 allocated message object
    /// @param[in, out] iter Input iterator used for reading.
    /// @param[in] size Size of the data in the sequence
    /// @param[out] missingSize If not nullptr and return value is
    ///             comms::ErrorStatus::NotEnoughData it will contain
    ///             minimal missing data length required for the successful
    ///             read attempt.
    /// @return Status of the operation.
    /// @pre Iterator must be valid and can be dereferenced and incremented at
    ///      least "size" times;
    /// @post The iterator will be advanced by the number of bytes was actually
    ///       read. In case of an error, distance between original position and
    ///       advanced will pinpoint the location of the error.
    /// @post Returns comms::ErrorStatus::Success if and only if msgPtr points
    ///       to a valid object.
    /// @post missingSize output value is updated if and only if function
    ///       returns comms::ErrorStatus::NotEnoughData.
    template <typename TMsgPtr, typename TIter>
    comms::ErrorStatus read(
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize = nullptr)
    {
        Field field;
        auto& derivedObj = static_cast<TDerived&>(*this);
        return derivedObj.doRead(field, msgPtr, iter, size, missingSize, createNextLayerReader());
    }

    /// @brief Deserialise message from the input data sequence while caching
    ///     the read transport information fields.
    /// @details Very similar to @ref read() member function, but adds "allFields"
    ///     parameter to store read transport information fields.
    ///     The function will also invoke the same @b doRead() member function
    ///     provided by the derived class, as described with @ref read().
    /// @tparam TIdx Index of the message ID field in TAllFields tuple.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TMsgPtr Type of smart pointer that holds message object.
    /// @tparam TIter Type of iterator used for reading.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in] msgPtr Reference to the smart pointer holding message object.
    /// @param[in, out] iter Iterator used for reading.
    /// @param[in] size Number of bytes available for reading.
    /// @param[out] missingSize If not nullptr and return value is
    ///             comms::ErrorStatus::NotEnoughData it will contain
    ///             minimal missing data length required for the successful
    ///             read attempt.
    /// @return Status of the operation.
    template <std::size_t TIdx, typename TAllFields, typename TMsgPtr, typename TIter>
    comms::ErrorStatus readFieldsCached(
        TAllFields& allFields,
        TMsgPtr& msgPtr,
        TIter& iter,
        std::size_t size,
        std::size_t* missingSize = nullptr)
    {
        auto& field = getField<TIdx>(allFields);
        auto& derivedObj = static_cast<TDerived&>(*this);
        return derivedObj.doRead(field, msgPtr, iter, size, missingSize, createNextLayerCachedFieldsReader<TIdx>(allFields));
    }

    /// @brief Serialise message into output data sequence.
    /// @details The function will invoke @b doWrite() member function
    ///     provided by the derived class, which must have the following signature
    ///     and logic:
    ///     @code
    ///         template<typename TMsg, typename TIter, typename TNextLayerWriter>
    ///         comms::ErrorStatus doWrite(
    ///             Field& field, // field object used to update and write required data
    ///             const TMsg& msg, // reference to ready to be sent message object
    ///             TIter& iter, // iterator used for writing
    ///             std::size_t size, // Max number of bytes that can be written.
    ///             TNextLayerWriter&& nextLayerWriter // next layer writer object
    ///             )
    ///         {
    ///             // internal logic prior next layer write, such as
    ///             // updating field's value and writing it.
    ///             field.value() = ...;
    ///             auto es = field.write(iter, size);
    ///             ...
    ///             // request next layer to perform write operation
    ///             es = nextLayerWriter.write(msg, iter, size - field.length());
    ///             ... // internal logic after next layer write if applicable
    ///             return es;
    ///         };
    ///     @endcode
    ///     The signature of the @b nextLayerWriter.write() function is
    ///     the same as the signature of this @b write() member function.
    /// @tparam TMsg Type of the message being written.
    /// @tparam TIter Type of iterator used for writing.
    /// @param[in] msg Reference to message object
    /// @param[in, out] iter Output iterator used for writing.
    /// @param[in] size Max number of bytes that can be written.
    /// @return Status of the write operation.
    /// @pre Iterator must be valid and can be dereferenced and incremented at
    ///      least "size" times;
    /// @post The iterator will be advanced by the number of bytes was actually
    ///       written. In case of an error, distance between original position
    ///       and advanced will pinpoint the location of the error.
    /// @return Status of the write operation.
    template <typename TMsg, typename TIter>
    comms::ErrorStatus write(
        const TMsg& msg,
        TIter& iter,
        std::size_t size) const
    {
        Field field;
        auto& derivedObj = static_cast<const TDerived&>(*this);
        return derivedObj.doWrite(field, msg, iter, size, createNextLayerWriter());
    }

    /// @brief Serialise message into output data sequence while caching the written transport
    ///     information fields.
    /// @details Very similar to @ref write() member function, but adds "allFields"
    ///     parameter to store raw data of the message.
    ///     The function will also invoke the same @b doWrite() member function
    ///     provided by the derived class, as described with @ref write().
    /// @tparam TIdx Index of the data field in TAllFields, expected to be last
    ///     element in the tuple.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TMsg Type of the message being written.
    /// @tparam TIter Type of iterator used for writing.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in] msg Reference to the message object that is being written,
    /// @param[in, out] iter Iterator used for writing.
    /// @param[in] size Max number of bytes that can be written.
    /// @return Status of the write operation.
    template <std::size_t TIdx, typename TAllFields, typename TMsg, typename TIter>
    comms::ErrorStatus writeFieldsCached(
        TAllFields& allFields,
        const TMsg& msg,
        TIter& iter,
        std::size_t size) const
    {
        auto& field = getField<TIdx>(allFields);
        auto& derivedObj = static_cast<const TDerived&>(*this);
        return derivedObj.doWrite(field, msg, iter, size, createNextLayerCachedFieldsWriter<TIdx>(allFields));
    }

    /// @brief Get remaining length of wrapping transport information.
    /// @details The message data always get wrapped with transport information
    ///     to be successfully delivered to and unpacked on the other side.
    ///     This function return remaining length of the transport information.
    /// @return length of the field + length reported by the next layer.
    constexpr std::size_t length() const
    {
        return Field::minLength() + nextLayer_.length();
    }

    /// @brief Get remaining length of wrapping transport information + length
    ///     of the provided message.
    /// @details This function usually gets called when there is a need to
    ///     identify the size of the buffer required to write provided message
    ///     wrapped in the transport information. This function is very similar
    ///     to length(), but adds also length of the message.
    /// @tparam TMsg Type of message object.
    /// @param[in] msg Message object
    /// @return length of the field + length reported by the next layer.
    template <typename TMsg>
    constexpr std::size_t length(const TMsg& msg) const
    {
        return Field::minLength() + nextLayer_.length(msg);
    }

    /// @brief Update recently written (using write()) message contents data.
    /// @details Sometimes, when NON random access iterator is used for writing
    ///     (for example std::back_insert_iterator), some transport data cannot
    ///     be properly written. In this case, @ref write() function will return
    ///     comms::ErrorStatus::UpdateRequired. When such status is returned
    ///     it is necessary to call update() with random access iterator on
    ///     the written buffer to update written dummy information with
    ///     proper values.@n
    ///     The function will invoke @b doUpdate() member function
    ///     provided (or inherited) by the derived class, which must have the following signature
    ///     and logic:
    ///     @code
    ///         template<typename TIter, typename TNextLayerUpdater>
    ///         comms::ErrorStatus doUpdate(
    ///             Field& field, // field object to update and re-write if necessary
    ///             TIter& iter, // iterator used for updateing
    ///             std::size_t size, // Number of remaning bytes in the output buffer.
    ///             TNextLayerUpdater&& nextLayerUpdater // next layer updater object
    ///             )
    ///         {
    ///             // internal logic prior next layer update, such as
    ///             // updating field's value and re-writing it.
    ///             field.value() = ...;
    ///             auto es = field.write(iter, size);
    ///             ...
    ///             // request next layer to perform update operation
    ///             es = nextLayerUpdater.update(iter, size - field.length());
    ///             ... // internal logic after next layer write if applicable
    ///             return es;
    ///         };
    ///     @endcode
    ///     The signature of the @b nextLayerUpdater.update() function is
    ///     the same as the signature of this @b update() member function.
    /// @param[in, out] iter Any random access iterator.
    /// @param[in] size Number of bytes that have been written using write().
    /// @return Status of the update operation.
    template <typename TIter>
    comms::ErrorStatus update(TIter& iter, std::size_t size) const
    {
        Field field;
        auto& derivedObj = static_cast<const TDerived&>(*this);
        return derivedObj.doUpdate(field, iter, size, createNextLayerUpdater());
    }

    /// @brief Update recently written (using writeFieldsCached()) message data as
    ///     well as cached transport information fields.
    /// @details Very similar to @ref update() member function, but adds "allFields"
    ///     parameter to store raw data of the message.@n
    ///     The function will also invoke the same @b doUpdate() member function
    ///     provided by the derived class, as described with @ref write().
    /// @tparam TIdx Index of the data field in TAllFields.
    /// @tparam TAllFields std::tuple of all the transport fields, must be
    ///     @ref AllFields type defined in the last layer class that defines
    ///     protocol stack.
    /// @tparam TIter Type of the random access iterator.
    /// @param[out] allFields Reference to the std::tuple object that wraps all
    ///     transport fields (@ref AllFields type of the last protocol layer class).
    /// @param[in, out] iter Random access iterator to the written data.
    /// @param[in] size Number of bytes that have been written using writeFieldsCached().
    /// @return Status of the update operation.
    template <std::size_t TIdx, typename TAllFields, typename TIter>
    ErrorStatus updateFieldsCached(
        TAllFields& allFields,
        TIter& iter,
        std::size_t size) const
    {
        auto& field = getField<TIdx>(allFields);
        auto& derivedObj = static_cast<const TDerived&>(*this);
        return derivedObj.doUpdate(field, iter, size, createNextLayerCachedFieldsUpdater<TIdx>(allFields));
    }

    /// @brief Default implementation of the "update" functaionality.
    /// @details It will be invoked by @ref update() or @ref updateFieldsCached()
    ///     member function, unless the derived class provides its own @ref doUpdate()
    ///     member function to override the default behavior.@n
    ///     This function in this layer does nothing, just advances the iterator
    ///     by the length of the @ref Field.
    /// @tparam TIter Type of iterator used for updating.
    /// @tparam TNextLayerWriter next layer updater object type.
    /// @param[out] field Field that needs to be updated.
    /// @param[in, out] iter Any random access iterator.
    /// @param[in] size Number of bytes that have been written using @ref write().
    /// @param[in] nextLayerUpdater Next layer updater object.
    template <typename TIter, typename TNextLayerUpdater>
    comms::ErrorStatus doUpdate(
        Field& field,
        TIter& iter,
        std::size_t size,
        TNextLayerUpdater&& nextLayerUpdater) const
    {
        return updateInternal(field, iter, size, std::forward<TNextLayerUpdater>(nextLayerUpdater), LengthTag());
    }

    /// @brief Create message object given the ID.
    /// @details The default implementation is to forwards this call to the next
    ///     layer. One of the layers (usually comms::protocol::MsgIdLayer)
    ///     hides and overrides this implementation.
    /// @tparam TMsg Type of message ID.
    /// @param id ID of the message.
    /// @param idx Relative index of the message with the same ID.
    /// @return Smart pointer (variant of std::unique_ptr) to allocated message
    ///     object
    template <typename TId>
    MsgPtr createMsg(TId&& id, unsigned idx = 0)
    {
        return nextLayer().createMsg(std::forward<TId>(id), idx);
    }

protected:

    /// @cond SKIP_DOC
    struct FixedLengthTag {};
    struct VarLengthTag {};
    using LengthTag = typename std::conditional<
        (Field::minLength() == Field::maxLength()),
        FixedLengthTag,
        VarLengthTag
    >::type;

    class NextLayerReader
    {
    public:
        explicit NextLayerReader(NextLayer& nextLayer)
          : nextLayer_(nextLayer)
        {
        }

        template <typename TMsgPtr, typename TIter>
        ErrorStatus read(
            TMsgPtr& msg,
            TIter& iter,
            std::size_t size,
            std::size_t* missingSize)
        {
            return nextLayer_.read(msg, iter, size, missingSize);
        }
    private:
        NextLayer& nextLayer_;
    };

    template <std::size_t TIdx, typename TAllFields>
    class NextLayerCachedFieldsReader
    {
    public:
        NextLayerCachedFieldsReader(
            NextLayer& nextLayer,
            TAllFields& allFields)
          : nextLayer_(nextLayer),
            allFields_(allFields)
        {
        }

        template<typename TMsgPtr, typename TIter>
        ErrorStatus read(
            TMsgPtr& msg,
            TIter& iter,
            std::size_t size,
            std::size_t* missingSize)
        {
            return nextLayer_.template readFieldsCached<TIdx + 1>(allFields_, msg, iter, size, missingSize);
        }

    private:
        NextLayer& nextLayer_;
        TAllFields& allFields_;
    };

    class NextLayerWriter
    {
    public:

        explicit NextLayerWriter(const NextLayer& nextLayer)
          : nextLayer_(nextLayer)
        {
        }

        template <typename TMsg, typename TIter>
        ErrorStatus write(const TMsg& msg, TIter& iter, std::size_t size) const
        {
            return nextLayer_.write(msg, iter, size);
        }

    private:
        const NextLayer& nextLayer_;
    };

    template <std::size_t TIdx, typename TAllFields>
    class NextLayerCachedFieldsWriter
    {
    public:
        NextLayerCachedFieldsWriter(
            const NextLayer nextLayer,
            TAllFields& allFields)
          : nextLayer_(nextLayer),
            allFields_(allFields)
        {
        }

        template <typename TMsg, typename TIter>
        ErrorStatus write(const TMsg& msg, TIter& iter, std::size_t size) const
        {
            return nextLayer_.template writeFieldsCached<TIdx + 1>(allFields_, msg, iter, size);
        }

    private:
        const NextLayer& nextLayer_;
        TAllFields& allFields_;
    };

    class NextLayerUpdater
    {
    public:

        explicit NextLayerUpdater(const NextLayer& nextLayer)
          : nextLayer_(nextLayer)
        {
        }

        template <typename TIter>
        ErrorStatus update(TIter& iter, std::size_t size) const
        {
            return nextLayer_.update(iter, size);
        }

    private:
        const NextLayer& nextLayer_;
    };

    template <std::size_t TIdx, typename TAllFields>
    class NextLayerCachedFieldsUpdater
    {
    public:
        NextLayerCachedFieldsUpdater(
            const NextLayer nextLayer,
            TAllFields& allFields)
          : nextLayer_(nextLayer),
            allFields_(allFields)
        {
        }

        template <typename TIter>
        ErrorStatus update(TIter& iter, std::size_t size) const
        {
            return nextLayer_.template updateFieldsCached<TIdx + 1>(allFields_, iter, size);
        }

    private:
        const NextLayer& nextLayer_;
        TAllFields& allFields_;
    };

    void updateMissingSize(std::size_t size, std::size_t* missingSize) const
    {
        if (missingSize != nullptr) {
            GASSERT(size <= length());
            *missingSize = std::max(std::size_t(1U), length() - size);
        }
    }

    void updateMissingSize(
        const Field& field,
        std::size_t size,
        std::size_t* missingSize) const
    {
        if (missingSize != nullptr) {
            auto totalLen = field.length() + nextLayer_.length();
            GASSERT(size <= totalLen);
            *missingSize = std::max(std::size_t(1U), totalLen - size);
        }
    }

    template <std::size_t TIdx, typename TAllFields>
    static Field& getField(TAllFields& allFields)
    {
        static_assert(comms::util::IsTuple<TAllFields>::Value,
                                        "Expected TAllFields to be a tuple");
        static_assert(TIdx < std::tuple_size<TAllFields>::value,
                                        "Invalid tuple access index");

        auto& field = std::get<TIdx>(allFields);

        using FieldType = typename std::decay<decltype(field)>::type;
        static_assert(
            std::is_same<Field, FieldType>::value,
            "Field has wrong type");

        return field;
    }

    NextLayerReader createNextLayerReader()
    {
        return NextLayerReader(nextLayer_);
    }

    template <std::size_t TIdx, typename TAllFields>
    NextLayerCachedFieldsReader<TIdx, TAllFields>
    createNextLayerCachedFieldsReader(TAllFields& fields)
    {
        return NextLayerCachedFieldsReader<TIdx, TAllFields>(nextLayer_, fields);
    }

    NextLayerWriter createNextLayerWriter() const
    {
        return NextLayerWriter(nextLayer_);
    }

    template <std::size_t TIdx, typename TAllFields>
    NextLayerCachedFieldsWriter<TIdx, TAllFields>
    createNextLayerCachedFieldsWriter(TAllFields& fields) const
    {
        return NextLayerCachedFieldsWriter<TIdx, TAllFields>(nextLayer_, fields);
    }

    NextLayerUpdater createNextLayerUpdater() const
    {
        return NextLayerUpdater(nextLayer_);
    }

    template <std::size_t TIdx, typename TAllFields>
    NextLayerCachedFieldsUpdater<TIdx, TAllFields>
    createNextLayerCachedFieldsUpdater(TAllFields& fields) const
    {
        return NextLayerCachedFieldsUpdater<TIdx, TAllFields>(nextLayer_, fields);
    }

    /// @endcond
private:

    template <typename TIter, typename TNextLayerUpdater>
    comms::ErrorStatus updateInternal(
        Field& field,
        TIter& iter,
        std::size_t size,
        TNextLayerUpdater&& nextLayerUpdater,
        FixedLengthTag) const
    {
        auto len = field.length();
        GASSERT(len <= size);
        std::advance(iter, len);
        return nextLayerUpdater.update(iter, size - len);
    }

    template <typename TIter, typename TNextLayerUpdater>
    comms::ErrorStatus updateInternal(
        Field& field,
        TIter& iter,
        std::size_t size,
        TNextLayerUpdater&& nextLayerUpdater,
        VarLengthTag) const
    {
        auto es = field.read(iter, size);
        if (es == comms::ErrorStatus::Success) {
            es = nextLayerUpdater.update(iter, size - field.length());
        }
        return es;
    }

    static_assert (comms::util::IsTuple<AllFields>::Value, "Must be tuple");
    NextLayer nextLayer_;
};

}  // namespace protocol

}  // namespace comms

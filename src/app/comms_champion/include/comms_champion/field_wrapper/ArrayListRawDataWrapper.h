//
// Copyright 2015 (C). Alex Robenko. All rights reserved.
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


#pragma once

#include <cstdint>
#include <cassert>
#include <memory>
#include <limits>

#include <QtCore/QString>

#include "comms/comms.h"

#include "FieldWrapper.h"

namespace comms_champion
{

namespace field_wrapper
{

class ArrayListRawDataWrapper : public FieldWrapper
{
public:

    virtual ~ArrayListRawDataWrapper() {}

    QString getValue() const
    {
        return getValueImpl();
    }

    void setValue(const QString& val)
    {
        setValueImpl(val);
    }

    int maxSize() const
    {
        return maxSizeImpl();
    }

protected:
    virtual QString getValueImpl() const = 0;
    virtual void setValueImpl(const QString& val) = 0;
    virtual int maxSizeImpl() const = 0;
};

template <typename TField>
class ArrayListRawDataWrapperT : public FieldWrapperT<ArrayListRawDataWrapper, TField>
{
    using Base = FieldWrapperT<ArrayListRawDataWrapper, TField>;
    using Field = TField;

public:
    using SerialisedSeq = typename Base::SerialisedSeq;

    ArrayListRawDataWrapperT(Field& field)
      : Base(field)
    {
    }

    ArrayListRawDataWrapperT(const ArrayListRawDataWrapperT&) = default;
    ArrayListRawDataWrapperT(ArrayListRawDataWrapperT&&) = default;
    virtual ~ArrayListRawDataWrapperT() = default;

    ArrayListRawDataWrapperT& operator=(const ArrayListRawDataWrapperT&) = delete;

protected:

    virtual QString getValueImpl() const override
    {
        QString retStr;
        auto& dataField = Base::field();
        auto& data = dataField.value();
        for (auto byte : data) {
            retStr.append(QString("%1").arg(static_cast<uint>(byte), 2, 16, QChar('0')));
        }
        return retStr;
    }

    virtual void setValueImpl(const QString& val) override
    {
        SerialisedSeq data;
        QString byteStr;

        auto addByteToData =
            [&data, &byteStr]() mutable
            {
                bool ok = false;
                auto val = byteStr.toInt(&ok, 16);
                if (!ok) {
                    return;
                }

                data.push_back(static_cast<typename SerialisedSeq::value_type>(val));
                byteStr.clear();
            };

        for (auto ch : val) {
            if (((ch < '0') || ('9' < ch)) &&
                ((ch.toLower() < 'a') || ('f' < ch.toLower()))) {
                continue;
            }

            byteStr.append(ch);

            if (2 <= byteStr.size()) {
                addByteToData();
            }
        }

        if (!byteStr.isEmpty()) {
            byteStr.append('0');
            addByteToData();
        }

        Base::setSerialisedValueImpl(data);
    }

    virtual bool setSerialisedValueImpl(const SerialisedSeq& value) override
    {
        static_cast<void>(value);
        assert(!"Mustn't be called");
        return false;
    }

    virtual int maxSizeImpl() const override
    {
        return maxSizeInternal(SizeExistanceTag());
    }

private:
    struct SizeFieldExistsTag {};
    struct NoSizeFieldTag {};

    typedef typename Field::ParsedOptions FieldOptions;
    typedef typename std::conditional<
        FieldOptions::HasSequenceSizeFieldPrefix,
        SizeFieldExistsTag,
        NoSizeFieldTag
    >::type SizeExistanceTag;

    static int maxSizeInternal(SizeFieldExistsTag)
    {
        typedef typename FieldOptions::SequenceSizeFieldPrefix SizeField;
        if (sizeof(int) <= SizeField::maxLength()) {
            return std::numeric_limits<int>::max();
        }

        auto shift =
            SizeField::maxLength() * std::numeric_limits<std::uint8_t>::digits;

        return static_cast<int>((1U << shift) - 1);
    }

    int maxSizeInternal(NoSizeFieldTag) const
    {
        return
            static_cast<int>(
                std::min(
                    static_cast<std::size_t>(std::numeric_limits<int>::max()),
                    Base::field().value().max_size()));
    }
};

using ArrayListRawDataWrapperPtr = std::unique_ptr<ArrayListRawDataWrapper>;

template <typename TField>
ArrayListRawDataWrapperPtr
makeArrayListRawDataWrapper(TField& field)
{
    return
        ArrayListRawDataWrapperPtr(
            new ArrayListRawDataWrapperT<TField>(field));
}

}  // namespace field_wrapper

}  // namespace comms_champion



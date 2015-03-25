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

#include <vector>
#include "comms_champion/Protocol.h"
#include "ProtocolStack.h"

namespace demo
{

namespace plugin
{

class Protocol : public comms_champion::Protocol
{
public:
    Protocol() = default;
    virtual ~Protocol();

protected:
    virtual const std::string& nameImpl() const override;

    virtual MessagesList readImpl(
            const comms_champion::DataInfo& dataInfo) override;

    virtual DataInfosList writeImpl(const MessagesList& msgs) override;

    virtual MessagesList createAllMessagesImpl() override;

    virtual comms_champion::MessageInfoPtr createMessageImpl(const QString& idAsString) override;

    virtual void updateMessageInfoImpl(comms_champion::MessageInfo& msgInfo) override;

    virtual comms_champion::MessageInfoPtr cloneMessageImpl(
        const comms_champion::MessageInfo& msgInfo) override;

private:
    comms_champion::MessageInfoPtr createMessage(demo::message::MsgId id);
    ProtocolStack m_protStack;
    std::vector<std::uint8_t> m_data;
    std::vector<std::uint8_t> m_garbage;
};

}  // namespace plugin

}  // namespace demo



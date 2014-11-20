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

#include "MsgListWidget.h"

#include <cassert>

#include "comms_champion/Message.h"

namespace comms_champion
{

MsgListWidget::MsgListWidget(
    const QString& listName,
    QWidget* toolbar,
    QWidget* parent)
{
    m_ui.setupUi(this);
    m_ui.m_groupBoxLayout->insertWidget(0, toolbar);
    m_ui.m_groupBox->setTitle(listName);
}

void MsgListWidget::addMessage(Message* msg)
{
    assert(msg != nullptr);
    m_ui.m_listWidget->addItem(msg->name());
}

}  // namespace comms_champion



/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//TODO extrn?

#ifndef I2CHANDLER_H__
#define I2CHANDLER_H__

#include "../space_control/spacecontrolclient.h"

#include <string>
#include <map>

namespace xmppsc {


//! I2C handler; currently the testbed for command methods.
class I2CHandler : public xmppsc::SpaceControlHandler {
public:
    virtual ~I2CHandler();

    virtual void handleSpaceCommand(gloox::JID peer, SpaceCommand sc, SpaceCommandSink* sink);

    //! Add a method to the handler
    /*!
     * \param method a pointer to the added method; ownership is transferred to the handler.
     */
    void add_method(CommandMethod* method);

private:
    std::map<std::string, CommandMethod*> m_methods;
};


} // namespace xmppsc

#endif // I2CHANDLER_H__
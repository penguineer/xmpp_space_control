#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

#include <unistd.h>

#include <xmppsc/configuredclientfactory.h>
#include <xmppsc/spacecontrolclient.h>
#include <xmppsc/methodhandler.h>
#include <xmppsc/util.h>

#include "finalizingcommandmethod.h"
#include "i3cresponsemethod.h"

using namespace xmppsc;

void run_client(gloox::Client* client) {
    if (!client->connect(true)) {
        std::cerr << "could not connect!" << std::endl;
        return;
    }
}

void wait_for_state(const gloox::Client* client, const gloox::ConnectionState& state) {
    while (client->state() != state)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

class I3CExceptionHandler : public FinalizingCommandMethod {
public:
    I3CExceptionHandler(gloox::Client* _client) throw()
        : FinalizingCommandMethod(_client, "exception")  {}

    virtual ~I3CExceptionHandler() throw() {}

    virtual bool evaluate_result(gloox::JID peer, const xmppsc::SpaceCommand& sc, xmppsc::SpaceCommandSink* sink) {
        // print the exception

        std::cerr << "Exception: " << std::endl;

        for (xmppsc::SpaceCommand::space_command_params::const_iterator it = sc.params().cbegin();
                it != sc.params().cend(); it++)
            std::cerr << it->first << ": " << it->second << std::endl;

        return true;
    }
};


class I3CTimeoutHandler : public FinalizingCommandMethod {
public:
    I3CTimeoutHandler(gloox::Client* _client) throw()
        : FinalizingCommandMethod(_client, "i3c.timeout")  {}

    virtual ~I3CTimeoutHandler() throw() {}

    virtual bool evaluate_result(gloox::JID peer, const xmppsc::SpaceCommand& sc, xmppsc::SpaceCommandSink* sink) {
        // print timeout error message

        std::cerr << "TTL-Timeout on I2C sending, something went wrong with bus, message or processing on the device!" << std::endl;

        return true;
    }
};

class I3CDeniedHandler : public FinalizingCommandMethod {
public:
    I3CDeniedHandler(gloox::Client* _client) throw()
        : FinalizingCommandMethod(_client, "denied")  {}

    virtual ~I3CDeniedHandler() throw() {}

    virtual bool evaluate_result(gloox::JID peer, const xmppsc::SpaceCommand& sc, xmppsc::SpaceCommandSink* sink) {
        // print timeout error message

        std::cerr << "Access denied!" << std::endl;

        return true;
    }
};

int main(int argc, char **argv) {
    //evaluate CLI arguments
    // progname <peer> <device> <command> [data]
    if (argc < 4) {
        std::cout << "Usage:" << std::endl;
        std::cout << "\ti3c_cli <XMPP peer> <device> <command> [data]" << std::endl << std::endl;
        std::cout << "Device, command and data expect hex values!" << std::endl;
        return -1;
    }

    const std::string peer(argv[1]);
    const std::string i3c_device(argv[2]);
    const std::string i3c_command(argv[3]);
    const std::string i3c_data(argc > 4 ? argv[4] : "");



    // Client configuration
    gloox::Client* client=0;
    xmppsc::AccessFilter* af=0;
    try {
        xmppsc::ConfiguredClientFactory ccf("spacecontrol.config");
        client = ccf.newClient();
        af = ccf.newAccessFilter();
    } catch (xmppsc::ConfiguredClientFactoryException &ccfe) {
        std::cerr << "ConfiguredClientFactoryException: " << ccfe.what() << std::endl;
        return (-1);
    }

    if (client) {
	// Use the "eco" variant
        xmppsc::set_eco_tcp_client(client);

        xmppsc::MethodHandler* mh = new xmppsc::MethodHandler();
        mh->add_method(new I3CResponseMethod(client));
        mh->add_method(new I3CExceptionHandler(client));
        mh->add_method(new I3CTimeoutHandler(client));
        mh->add_method(new I3CDeniedHandler(client));


        xmppsc::SpaceControlClient* scc = new xmppsc::SpaceControlClient(client, mh,
                new xmppsc::TextSpaceCommandSerializer(), af);

        // connect
        std::thread client_thread(run_client, client);
        wait_for_state(client, gloox::ConnectionState::StateConnected);

        // Send the I3C command
        xmppsc::SpaceCommand::space_command_params parms;
        parms["device"] = i3c_device;
        parms["command"] = i3c_command;
        if (! i3c_data.empty())
            parms["data"] = i3c_data;
        const xmppsc::SpaceCommand sc("i3c.call", parms);

        std::stringstream threadId("");
        threadId << "cli_thread_" << client->resource();
        xmppsc::SpaceCommandSink* sink = scc->create_sink(gloox::JID(peer), threadId.str());

        sink->sendSpaceCommand(sc);

        // go on in client thread (response arrives here)
        client_thread.join();

        // cleanup
        delete sink;


        delete client;
    } else {
        std::cerr << "Could not create an XMPP client instance!" << std::endl;
    }

    if (af)
        delete af;

    return 0;
}

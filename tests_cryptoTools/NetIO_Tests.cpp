#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>

#include <cryptoTools/Common/TestCollection.h>

#include "NetIO_Tests.h"

using namespace osuCrypto;

namespace tests_cryptoTools
{
    void NetIO_AnonymousMode_Test() {
        IOService ioService;
        Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
        Session s2(ioService, "127.0.0.1", 1212, SessionMode::Server);
        Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
        Session c2(ioService, "127.0.0.1", 1212, SessionMode::Client);

        auto c1c1 = c1.addChannel();
        auto c1c2 = c1.addChannel();

        auto s1c1 = s1.addChannel();
        auto s2c1 = s2.addChannel();
        //auto s1c2 = s1.addChannel();
        //auto s2c2 = s2.addChannel();

        c1c2.waitForConnection();
        auto c2c1 = c2.addChannel();
        auto c2c2 = c2.addChannel();
        std::string m1 = "m1";
        std::string m2 = "m2";

        /*c1c1.send(m1);
        c2c1.send(m1);
        c1c2.send(m2);
        c2c2.send(m2);

        std::string t;

        s1c1.recv(t);
        if (m1 != t) throw UnitTestFail();

        s2c1.recv(t);
        if (m1 != t) throw UnitTestFail();

        s1c2.recv(t);
        if (m2 != t) throw UnitTestFail();

        s2c2.recv(t);
        if (m2 != t) throw UnitTestFail();

        if (c1c1.getName() != s1c1.getName()) throw UnitTestFail();
        if (c2c1.getName() != s2c1.getName()) throw UnitTestFail();
        if (c1c2.getName() != s1c2.getName()) throw UnitTestFail();
        if (c2c2.getName() != s2c2.getName())
            throw UnitTestFail();

        if (s1.getSessionID() != c1.getSessionID())
            throw UnitTestFail();
        if (s2.getSessionID() != c2.getSessionID()) throw UnitTestFail();*/
    }

}
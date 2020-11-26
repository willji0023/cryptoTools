#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>

#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/TestCollection.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/Finally.h>

#include "NetIO_Tests.h"

using namespace osuCrypto;

namespace tests_cryptoTools
{
    void NetIO_Connect1_Test() {
        std::string channelName{ "TestChannel" };
        std::string msg{ "This is the message" };

        IOService ioService;
        Channel chl1, chl2;
        auto thrd = std::thread([&]()
            {
                setThreadName("Test_Client");

                Session endpoint(ioService, "127.0.0.1", 1212, SessionMode::Client);
                chl1 = endpoint.addChannel(channelName, channelName);

                std::string recvMsg;
                chl1.recv(recvMsg);

                if (recvMsg != msg) throw UnitTestFail();

                chl1.asyncSend(std::move(recvMsg));
                chl1.close();
            });

        try {
            Session endpoint(ioService, "127.0.0.1", 1212, SessionMode::Server);
            chl2 = endpoint.addChannel(channelName, channelName);

            chl2.asyncSend(msg);

            std::string clientRecv;
            chl2.recv(clientRecv);

            if (clientRecv != msg)
                throw UnitTestFail();
        }
        catch (std::exception & e)
        {
            lout << e.what() << std::endl;
            thrd.join();
            throw;
        }

        thrd.join();
    }

    void NetIO_AnonymousMode_Test() {
        IOService ioService;

        std::string m1 = "m1";
        std::string m2 = "m2";

        Channel c1c, c2c;
        
        std::string c1Name, c2Name;
        u64 c1ID, c2ID;

        auto thrd = std::thread([&]() {
            Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
            Session c2(ioService, "127.0.0.1", 1212, SessionMode::Client);

            c1ID = c1.getSessionID();
            c2ID = c2.getSessionID();

            c1c = c1.addChannel();
            c1c.waitForConnection();

            c2c = c2.addChannel();
            c2c.waitForConnection();

            c1Name = c1c.getName();
            c2Name = c2c.getName();

            c1c.send(m1);
            c2c.send(m1);
            c1c.send(m2);
            c2c.send(m2);
        });

        try {
            Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
            Session s2(ioService, "127.0.0.1", 1212, SessionMode::Server);

            auto s1c = s1.addChannel();
            auto s2c = s2.addChannel();

            std::string t;

            s1c.recv(t);
            if (m1 != t) throw UnitTestFail();

            s2c.recv(t);
            if (m1 != t) throw UnitTestFail();

            s1c.recv(t);
            if (m2 != t) throw UnitTestFail();

            s2c.recv(t);
            if (m2 != t) throw UnitTestFail();

            if (c1Name != s1c.getName()) throw UnitTestFail();
            if (c2Name != s2c.getName()) throw UnitTestFail();
        }
        catch (std::exception & e)
        {
            lout << e.what() << std::endl;
            thrd.join();
            throw;
        }

        thrd.join();
    }

    void NetIO_ServerMode_Test() {
        u64 numConnect = 25;
        IOService ioService(0);
        std::vector<Channel> srvChls(numConnect), clientChls(numConnect);

        for (u64 i = 0; i < numConnect; ++i)
        {
            auto thrd = std::thread([&]() {
                Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
                srvChls[i] = s1.addChannel();
            });

            Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
            clientChls[i] = c1.addChannel();

            std::string m1("c1");

            clientChls[i].asyncSend(std::move(m1));

            thrd.join();
        }

        for (u64 i = 0; i < numConnect; ++i)
        {
            std::string m;
            srvChls[i].recv(m);
            if (m != "c1") throw UnitTestFail();
        }

        /////////////////////////////////////////////////////////////////////////////

        for (u64 i = 0; i < numConnect; ++i)
        {
            auto thrd = std::thread([&]() {
                Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
                srvChls[i] = s1.addChannel();
            });

            Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
            clientChls[i] = c1.addChannel();

            std::string m1("c1");

            clientChls[i].asyncSend(std::move(m1));

            thrd.join();
        }

        for (u64 i = 0; i < numConnect; ++i)
        {
            std::string m;
            srvChls[i].recv(m);
            if (m != "c1") throw UnitTestFail();
        }
    }

    void NetIO_Shutdown_Test() {
        IOService ios;
        std::array<Channel, 2> chls;

        auto go = [&](int idx)
        {
            auto mode = idx ? EpMode::Server : EpMode::Client;
            Endpoint ep(ios, "127.0.0.1", 1213, mode, "none");
            chls[idx] = ep.addChannel();

            std::vector<u8> buff(100);
            chls[idx].send(buff);
            chls[idx].recv(buff);
            chls[idx].send(buff);
            chls[idx].recv(buff);

            chls[idx].close();
            ep.stop();
        };

        {
            auto thrd = std::thread([&]()
                {
                    go(1);
                });
            go(0);
            thrd.join();
        }

        ios.stop();
    }

    void NetIO_OneMegabyteSend_Test() {
        setThreadName("Test_Host");

        std::string channelName{ "TestChannel" };
        std::string msg{ "This is the message" };
        std::vector<u8> oneMegabyte((u8*)msg.data(), (u8*)msg.data() + msg.size());
        oneMegabyte.resize(1000000);

        memset(oneMegabyte.data() + 100, 0xcc, 1000000 - 100);

        IOService ioService(0);

        auto thrd = std::thread([&]() {
            setThreadName("Test_Client");

            Session endpoint(ioService, "127.0.0.1", 1212, SessionMode::Client, "endpoint");
            Channel chl = endpoint.addChannel(channelName, channelName);

            std::vector<u8> srvRecv;
            chl.recv(srvRecv);
            auto copy = srvRecv;
            chl.asyncSend(std::move(copy));
            chl.close();

            auto act = chl.getTotalDataRecv();
            auto exp = oneMegabyte.size() + 4;

            if (act != exp)
                throw UnitTestFail("channel recv statistics incorrectly increased." LOCATION);

            if (srvRecv != oneMegabyte)
                throw UnitTestFail("channel recv the wrong value." LOCATION);
        });

        Finally f([&] { thrd.join(); });

        Session endpoint(ioService, "127.0.0.1", 1212, SessionMode::Server, "endpoint");
        auto chl = endpoint.addChannel(channelName, channelName);

        if (chl.getTotalDataSent() != 0)
            throw UnitTestFail("channel send statistics incorrectly initialized." LOCATION);
        if (chl.getTotalDataRecv() != 0)
            throw UnitTestFail("channel recv statistics incorrectly initialized." LOCATION);

        std::vector<u8> clientRecv;
        chl.asyncSend(oneMegabyte);
        chl.recv(clientRecv);
        chl.close();

        if (chl.getTotalDataSent() != oneMegabyte.size() + 4)
            throw UnitTestFail("channel send statistics incorrectly increased." LOCATION);
        if (chl.getTotalDataRecv() != oneMegabyte.size() + 4)
            throw UnitTestFail("channel recv statistics incorrectly increased." LOCATION);

        chl.resetStats();

        if (chl.getTotalDataSent() != 0)
            throw UnitTestFail("channel send statistics incorrectly reset." LOCATION);
        if (chl.getTotalDataRecv() != 0)
            throw UnitTestFail("channel recv statistics incorrectly reset." LOCATION);

        if (clientRecv != oneMegabyte)
            throw UnitTestFail();
    }

    void NetIO_std_Containers_Test() {
        setThreadName("Test_Host");
        std::string channelName{ "TestChannel" }, msg{ "This is the message" };
        IOService ioService;

        std::vector<u32> vec_u32{ 0,1,2,3,4,5,6,7,8,9 };
        std::array<u32, 10> arr_u32_10;
        std::string hello{ "hello world" };

        auto thrd = std::thread([&]() {
            Session ep2(ioService, "127.0.0.1", 1212, SessionMode::Server, "endpoint");
            auto chl2 = ep2.addChannel(channelName, channelName);
            chl2.recv(arr_u32_10);

            if (std::mismatch(vec_u32.begin(), vec_u32.end(), arr_u32_10.begin()).first != vec_u32.end())
                throw UnitTestFail("send vec, recv array");

            chl2.asyncSend(std::move(vec_u32));

            chl2.asyncSend(std::move(hello));
        });

        Session ep1(ioService, "127.0.0.1", 1212, SessionMode::Client, "endpoint");
        auto chl1 = ep1.addChannel(channelName, channelName);

        chl1.send(vec_u32);

        chl1.recv(vec_u32);
            
        if (std::mismatch(vec_u32.begin(), vec_u32.end(), arr_u32_10.begin()).first != vec_u32.end())
            throw UnitTestFail("send vec, recv array");

        chl1.recv(hello);
        if (hello != "hello world") UnitTestFail("std::string move");
    
        thrd.join();
    }

    void NetIO_bitVector_Test() {
        setThreadName("Test_Host");
        std::string channelName{ "TestChannel" }, msg{ "This is the message" };
        IOService ioService;

        BitVector bb(77);
        bb[55] = 1;
        bb[33] = 1;

        auto thrd = std::thread([&]() {
            Session ep1(ioService, "127.0.0.1", 1212, SessionMode::Client, "endpoint");
            auto chl1 = ep1.addChannel(channelName, channelName);
            chl1.send(bb);
        });

        Session ep2(ioService, "127.0.0.1", 1212, SessionMode::Server, "endpoint");
        auto chl2 = ep2.addChannel(channelName, channelName);
        chl2.recv(bb);

        if (!bb[55] || !bb[33])
            throw UnitTestFail();      

        thrd.join();  
    }

    void NetIO_Instantiation_Test() {
        u64 numConnect = 25;
        IOService ioService(0);
        std::vector<Channel> srvChls(numConnect), clientChls(numConnect);

        for (u64 i = 0; i < numConnect; ++i)
        {
            auto thrd = std::thread([&]() {
                Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
                srvChls[i] = s1.addChannel();
            });

            Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
            clientChls[i] = c1.addChannel();

            thrd.join();
        }

        for (u64 i = 0; i < numConnect; ++i)
        {
            auto thrd = std::thread([&]() {
                Session s1(ioService, "127.0.0.1", 1212, SessionMode::Server);
                srvChls[i] = s1.addChannel();
            });

            Session c1(ioService, "127.0.0.1", 1212, SessionMode::Client);
            clientChls[i] = c1.addChannel();

            thrd.join();
        }
    }
}
#pragma once

#include <stdint.h>

#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include "minecraft/network/Socket.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/System.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/thread/C4JThread.h"

class DataInputStream;
class DataOutputStream;
class Packet;
class PacketListener;
class SocketAddress;

// 4J JEV, size of the threads (bytes).
#define READ_STACK_SIZE 0
#define WRITE_STACK_SIZE 0

class ByteArrayOutputStream;

class Connection {
    friend uint32_t runRead(void* lpParam);
    friend uint32_t runWrite(void* lpParam);
    friend uint32_t runSendAndQuit(void* lpParam);
    friend uint32_t runClose(void* lpParam);

private:
    static const int SEND_BUFFER_SIZE = 1024 * 5;

public:
    static int readThreads, writeThreads;

private:
    static const int MAX_TICKS_WITHOUT_INPUT = 20 * 60;

public:
    static const int IPTOS_LOWCOST = 0x02;
    static const int IPTOS_RELIABILITY = 0x04;
    static const int IPTOS_THROUGHPUT = 0x08;
    static const int IPTOS_LOWDELAY = 0x10;

private:
    Socket* socket;
    const SocketAddress* address;
    DataInputStream* dis;
    DataOutputStream*
        bufferedDos;  // 4J This is the same type of dos the java game has

    // 4J Added
    DataOutputStream* byteArrayDos;  // 4J This dos allows us to write
                                     // individual packets to the socket
    ByteArrayOutputStream* baos;
    Socket::SocketOutputStream* sos;

    bool running;

    std::queue<std::shared_ptr<Packet> >
        incoming;            // 4J - was using synchronizedList...
    std::mutex incoming_cs;  // ... now has this mutex
    std::queue<std::shared_ptr<Packet> >
        outgoing;  // 4J - was using synchronizedList - but don't think it is
                   // required as usage is wrapped in writeLock
    std::queue<std::shared_ptr<Packet> >
        outgoing_slow;  // 4J - was using synchronizedList - but don't think it
                        // is required as usage is wrapped in writeLock

    PacketListener* packetListener;
    bool quitting;

    C4JThread* readThread;
    C4JThread* writeThread;

    C4JThread::Event* m_hWakeReadThread;
    C4JThread::Event* m_hWakeWriteThread;

    uint32_t saqThreadID, closeThreadID;

    bool disconnected;
    DisconnectPacket::eDisconnectReason disconnectReason;
    void** disconnectReasonObjects;  // 4J a pointer to an array.

    int noInputTicks;
    int estimatedRemaining;

    int tickCount;  // 4J Added

public:
    static int readSizes[256];
    static int writeSizes[256];

    int fakeLag;

private:
    void _init();

    // 4J Jev, these might be better of as private
    std::mutex threadCounterLock;
    std::mutex writeLock;

public:
    ~Connection();
    Connection(Socket* socket, const std::string& id,
               PacketListener* packetListener);  // throws IOException

    void setListener(PacketListener* packetListener);
    void send(std::shared_ptr<Packet> packet);

public:
    void queueSend(std::shared_ptr<Packet> packet);

private:
    int slowWriteDelay;

    bool writeTick();

public:
    void flush();

private:
    bool readTick();

private:
    /* 4J JEV, removed try/catch
    void handleException(Exception e)
    {
    e.printStackTrace();
    close("disconnect.genericReason", "Internal exception: " + e.toString());
    }*/

public:
    void close(DisconnectPacket::eDisconnectReason reason);

    void tick();

    SocketAddress* getRemoteAddress();

    void sendAndQuit();

    int countDelayedPackets();

    Socket* getSocket() { return socket; }

private:
    static int runRead(void* lpParam);
    static int runWrite(void* lpParam);
    static int runClose(void* lpParam);
    static int runSendAndQuit(void* lpParam);
};

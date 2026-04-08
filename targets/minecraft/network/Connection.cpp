#include "Connection.h"

#include <stdio.h>

#include <chrono>
#include <thread>
#include <vector>

#include "platform/ShutdownManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "app/common/Network/Socket.h"
#include "util/StringHelpers.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "java/InputOutputStream/BufferedOutputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/System.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/KeepAlivePacket.h"
#include "minecraft/network/packet/Packet.h"
#include "minecraft/network/packet/PacketListener.h"

class SocketAddress;

// This should always be enabled, except for debugging use
#if !defined(_DEBUG)
#define CONNECTION_ENABLE_TIMEOUT_DISCONNECT 1
#endif

int Connection::readThreads = 0;
int Connection::writeThreads = 0;

int Connection::readSizes[256];
int Connection::writeSizes[256];

void Connection::_init() {
    //	printf("Con:0x%x init\n",this);
    running = true;
    quitting = false;
    disconnected = false;
    disconnectReason = DisconnectPacket::eDisconnect_None;
    disconnectReasonObjects = nullptr;
    noInputTicks = 0;
    estimatedRemaining = 0;
    fakeLag = 0;
    slowWriteDelay = 50;

    saqThreadID = 0;
    closeThreadID = 0;

    tickCount = 0;
}

Connection::~Connection() {
    // 4J Stu - Just to be sure, make sure the read and write threads terminate
    // themselves before the connection object is destroyed
    running = false;
    if (dis)
        dis->close();  // The input stream needs closed before the readThread,
                       // or the readThread may get stuck whilst blocking
                       // waiting on a read
    readThread->waitForCompletion(C4JThread::kInfiniteTimeout);
    writeThread->waitForCompletion(C4JThread::kInfiniteTimeout);

    delete m_hWakeReadThread;
    delete m_hWakeWriteThread;

    // These should all have been destroyed in close() but no harm in checking
    // again
    delete byteArrayDos;
    byteArrayDos = nullptr;
    delete baos;
    baos = nullptr;
    if (bufferedDos) {
        bufferedDos->deleteChildStream();
        delete bufferedDos;
        bufferedDos = nullptr;
    }
    delete dis;
    dis = nullptr;
}

Connection::Connection(Socket* socket, const std::string& id,
                       PacketListener* packetListener)  // throws IOException
{
    _init();

    this->socket = socket;

    address = socket->getRemoteSocketAddress();

    this->packetListener = packetListener;

    // try {
    socket->setSoTimeout(30000);
    socket->setTrafficClass(IPTOS_THROUGHPUT | IPTOS_LOWDELAY);

    /* 4J JEV no catch
    } catch (SocketException e) {
    // catching this exception because it (apparently?) causes problems
    // on OSX Tiger
    System.err.println(e.getMessage());
    }*/

    dis = new DataInputStream(
        socket->getInputStream(packetListener->isServerPacketListener()));

    sos = socket->getOutputStream(packetListener->isServerPacketListener());
    bufferedDos =
        new DataOutputStream(new BufferedOutputStream(sos, SEND_BUFFER_SIZE));
    baos = new ByteArrayOutputStream(SEND_BUFFER_SIZE);
    byteArrayDos = new DataOutputStream(baos);

    m_hWakeReadThread = new C4JThread::Event;
    m_hWakeWriteThread = new C4JThread::Event;

    std::string szId = id;
    char readThreadName[256];
    char writeThreadName[256];
    sprintf(readThreadName, "%s read\n", szId.c_str());
    sprintf(writeThreadName, "%s write\n", szId.c_str());

    readThread =
        new C4JThread(runRead, (void*)this, readThreadName, READ_STACK_SIZE);
    writeThread =
        new C4JThread(runWrite, this, writeThreadName, WRITE_STACK_SIZE);
    readThread->run();
    writeThread->run();

    /* 4J JEV, java:
    new Thread(string(id).append(" read thread")) {

    };

    writeThread = new Thread(id + " write thread") {
    public void run() {

    };

    readThread->start();
    writeThread->start();
    */
}

void Connection::setListener(PacketListener* packetListener) {
    this->packetListener = packetListener;
}

void Connection::send(std::shared_ptr<Packet> packet) {
    if (quitting) return;

    // 4J Jev, synchronized (&writeLock)
    {
        std::lock_guard<std::mutex> lock(writeLock);

        estimatedRemaining += packet->getEstimatedSize() + 1;
        if (packet->shouldDelay) {
            // 4J We have delayed it enough by putting it in the slow queue, so
            // don't delay when we actually send it
            packet->shouldDelay = false;
            outgoing_slow.push(packet);
        } else {
            outgoing.push(packet);
        }
    }

    // 4J Jev, end synchronized.
}

void Connection::queueSend(std::shared_ptr<Packet> packet) {
    if (quitting) return;
    {
        std::lock_guard<std::mutex> lock(writeLock);
        estimatedRemaining += packet->getEstimatedSize() + 1;
        outgoing_slow.push(packet);
    }
}

bool Connection::writeTick() {
    bool didSomething = false;

    // 4J Stu - If the connection is closed and the output stream has been
    // deleted
    if (bufferedDos == nullptr || byteArrayDos == nullptr) return didSomething;

    // try {
    if (!outgoing.empty() &&
        (fakeLag == 0 ||
         System::currentTimeMillis() - outgoing.front()->createTime >=
             fakeLag)) {
        std::shared_ptr<Packet> packet;

        {
            std::lock_guard<std::mutex> lock(writeLock);

            packet = outgoing.front();
            outgoing.pop();
            estimatedRemaining -= packet->getEstimatedSize() + 1;
        }

        Packet::writePacket(packet, bufferedDos);
#if defined(__linux__)
        bufferedDos->flush();  // Ensure buffered data reaches socket before any
                               // other writes
#endif

#if !defined(_CONTENT_PACKAGE)
        // 4J Added for debugging
        int playerId = 0;
        if (!socket->isLocal()) {
            Socket* socket = getSocket();
            if (socket) {
                INetworkPlayer* player = socket->getPlayer();
                if (player) {
                    playerId = player->GetSmallId();
                }
            }
            Packet::recordOutgoingPacket(packet, playerId);
        }
#endif

        // 4J Stu - Changed this so that rather than writing to the network
        // stream through a buffered stream we want to: a) Only push whole
        // "game" packets to QNet, rather than amalgamated chunks of data that
        // may include many packets, and partial packets b) To be able to change
        // the priority and queue of a packet if required
        // sos->writeWithFlags( baos->buf, 0, baos->size(), 0 );
        // baos->reset();

        writeSizes[packet->getId()] += packet->getEstimatedSize() + 1;
        didSomething = true;
    }

    if ((slowWriteDelay-- <= 0) && !outgoing_slow.empty() &&
        (fakeLag == 0 ||
         System::currentTimeMillis() - outgoing_slow.front()->createTime >=
             fakeLag)) {
        std::shared_ptr<Packet> packet;

        // synchronized (writeLock) {

        {
            std::lock_guard<std::mutex> lock(writeLock);

            packet = outgoing_slow.front();
            outgoing_slow.pop();
            estimatedRemaining -= packet->getEstimatedSize() + 1;
        }

        // If the shouldDelay flag is still set at this point then we want to
        // write it to QNet as a single packet with priority flags Otherwise
        // just buffer the packet with other outgoing packets as the java game
        // did
#if defined(__linux__)
        // Linux fix: For local connections, always use bufferedDos to avoid
        // byte interleaving between the BufferedOutputStream buffer and direct
        // sos writes. The shouldDelay/writeWithFlags path writes directly to
        // sos, which can inject bytes BEFORE unflushed bufferedDos data.
        Packet::writePacket(packet, bufferedDos);
        bufferedDos->flush();  // Ensure data reaches socket immediately for
                               // delayed packets
#else
        if (packet->shouldDelay) {
            Packet::writePacket(packet, byteArrayDos);

            // 4J Stu - Changed this so that rather than writing to the network
            // stream through a buffered stream we want to: a) Only push whole
            // "game" packets to QNet, rather than amalgamated chunks of data
            // that may include many packets, and partial packets b) To be able
            // to change the priority and queue of a packet if required
            int flags = NON_QNET_SENDDATA_ACK_REQUIRED;
            sos->writeWithFlags(baos->buf, 0, baos->size(), flags);
            baos->reset();
        } else {
            Packet::writePacket(packet, bufferedDos);
        }

#endif

#if !defined(_CONTENT_PACKAGE)
        // 4J Added for debugging
        if (!socket->isLocal()) {
            int playerId = 0;
            if (!socket->isLocal()) {
                Socket* socket = getSocket();
                if (socket) {
                    INetworkPlayer* player = socket->getPlayer();
                    if (player) {
                        playerId = player->GetSmallId();
                    }
                }
                Packet::recordOutgoingPacket(packet, playerId);
            }
        }
#endif

        writeSizes[packet->getId()] += packet->getEstimatedSize() + 1;
        slowWriteDelay = 0;
        didSomething = true;
    }
    /* 4J JEV, removed try/catch
    } catch (Exception e) {
    if (!disconnected) handleException(e);
    return false;
    } */

    return didSomething;
}

void Connection::flush() {
    // TODO 4J Stu - How to interrupt threads? Or do we need to change the
    // multithreaded functions a bit more
    // readThread.interrupt();
    // writeThread.interrupt();
    m_hWakeReadThread->set();
    m_hWakeWriteThread->set();
}

bool Connection::readTick() {
    bool didSomething = false;

    // 4J Stu - If the connection has closed and the input stream has been
    // deleted
    if (dis == nullptr) return didSomething;

    // try {

    std::shared_ptr<Packet> packet =
        Packet::readPacket(dis, packetListener->isServerPacketListener());

    if (packet != nullptr) {
        readSizes[packet->getId()] += packet->getEstimatedSize() + 1;
        {
            std::lock_guard<std::mutex> lock(incoming_cs);
            if (!quitting) {
                incoming.push(packet);
            }
        }
        didSomething = true;
    } else {
        //		printf("Con:0x%x readTick close EOS\n",this);

        // 4J Stu - Remove this line
        // Fix for #10410 - UI: If the player is removed from a splitscreened
        // host�s game, the next game that player joins will produce a message
        // stating that the host has left.
        // close(DisconnectPacket::eDisconnect_EndOfStream);
    }

    /* 4J JEV, removed try/catch
    } catch (Exception e) {
    if (!disconnected) handleException(e);
    return false;
    } */

    return didSomething;
}

/* 4J JEV, removed try/catch
void handleException(Exception e)
{
e.printStackTrace();
close("disconnect.genericReason", "Internal exception: " + e.toWString());
}*/

void Connection::close(DisconnectPacket::eDisconnectReason reason) {
    //	printf("Con:0x%x close\n",this);
    if (!running) return;
    //	printf("Con:0x%x close doing something\n",this);
    disconnected = true;

    disconnectReason = reason;  // va_arg( input, const string );
    disconnectReasonObjects = nullptr;

    //	int count = 0, sum = 0, i = first;
    //	va_list marker;
    //
    //	va_start( marker, first );
    //	while( i != -1 )
    //	{
    //	   sum += i;
    //	   count++;
    //	   i = va_arg( marker, int);
    //	}
    //	va_end( marker );
    //	return( sum ? (sum / count) : 0 );

    //	CreateThread(nullptr, 0, runClose, this, 0, &closeThreadID);

    running = false;

    if (dis)
        dis->close();  // The input stream needs closed before the readThread,
                       // or the readThread may get stuck whilst blocking
                       // waiting on a read

    // Make sure that the read & write threads are dead before we go and kill
    // the streams that they depend on
    readThread->waitForCompletion(C4JThread::kInfiniteTimeout);
    writeThread->waitForCompletion(C4JThread::kInfiniteTimeout);

    delete dis;
    dis = nullptr;
    if (bufferedDos) {
        bufferedDos->close();
        bufferedDos->deleteChildStream();
        delete bufferedDos;
        bufferedDos = nullptr;
    }
    if (byteArrayDos) {
        byteArrayDos->close();
        delete byteArrayDos;
        byteArrayDos = nullptr;
    }
    if (socket) {
        socket->close(packetListener->isServerPacketListener());
        socket = nullptr;
    }
}

void Connection::tick() {
    if (estimatedRemaining > 1 * 1024 * 1024) {
        close(DisconnectPacket::eDisconnect_Overflow);
    }
    bool empty;
    {
        std::lock_guard<std::mutex> lock(incoming_cs);
        empty = incoming.empty();
    }
    if (empty) {
#if CONNECTION_ENABLE_TIMEOUT_DISCONNECT
        if (noInputTicks++ == MAX_TICKS_WITHOUT_INPUT) {
            close(DisconnectPacket::eDisconnect_TimeOut);
        }
#endif
    }
    // 4J Stu - Moved this a bit later in the function to stop the race
    // condition of Disconnect packets not being processed when local client
    // leaves
    // else if( socket && socket->isClosing() )
    //{
    //	close(DisconnectPacket::eDisconnect_Closed);
    //}
    else {
        noInputTicks = 0;
    }

    // 4J Added - Send a KeepAlivePacket every now and then to ensure that our
    // read and write threads don't timeout
    tickCount++;
    if (tickCount % 20 == 0) {
        send(std::make_shared<KeepAlivePacket>());
    }

    // 4J Stu - 1.8.2 changed from 100 to 1000
    int max = 1000;

    // 4J-PB - NEEDS CHANGED!!!
    // If we can call connection.close from within a packet->handle, then we can
    // lockup because the loop below has locked incoming_cs, and the
    // connection.close will flag the read and write threads for the connection
    // to close. they are running on other threads, and will try to lock
    // incoming_cs We got this with a pre-login packet of a player who wasn't
    // allowed to play due to parental controls, so was kicked out This has been
    // changed to use a eAppAction_ExitPlayerPreLogin which will run in the main
    // loop, so the connection will not be ticked at that point

    // 4J Stu - If disconnected, then we shouldn't process incoming packets
    std::vector<std::shared_ptr<Packet> > packetsToHandle;
    {
        std::lock_guard<std::mutex> lock(incoming_cs);
        while (!disconnected && !g_NetworkManager.IsLeavingGame() &&
               g_NetworkManager.IsInSession() && !incoming.empty() &&
               max-- >= 0) {
            std::shared_ptr<Packet> packet = incoming.front();
            packetsToHandle.push_back(packet);
            incoming.pop();
        }
    }

    // MGH - moved the packet handling outside of the incoming_cs block, as it
    // was locking up sometimes when disconnecting
    for (int i = 0; i < packetsToHandle.size(); i++) {
        packetsToHandle[i]->handle(packetListener);
    }
    flush();

    // 4J Stu - Moved this a bit later in the function to stop the race
    // condition of Disconnect packets not being processed when local client
    // leaves
    if (socket && socket->isClosing()) {
        close(DisconnectPacket::eDisconnect_Closed);
    }

    // 4J - split the following condition (used to be disconnect &&
    // iscoming.empty()) so we can wrap the access in a mutex
    if (disconnected) {
        bool empty;
        {
            std::lock_guard<std::mutex> lock(incoming_cs);
            empty = incoming.empty();
        }
        if (empty) {
            packetListener->onDisconnect(disconnectReason,
                                         disconnectReasonObjects);
            disconnected =
                false;  // 4J added - don't keep sending this every tick
        }
    }
}

SocketAddress* Connection::getRemoteAddress() {
    return (SocketAddress*)address;
}

void Connection::sendAndQuit() {
    if (quitting) {
        return;
    }
    //	printf("Con:0x%x send & quit\n",this);
    flush();
    quitting = true;
    // TODO 4J Stu - How to interrupt threads? Or do we need to change the
    // multithreaded functions a bit more
    // readThread.interrupt();

    // 4J - this used to be in a thread but not sure why, and is causing trouble
    // for us if we kill the connection whilst the thread is still expecting to
    // be able to send a packet a couple of seconds after starting it
    if (running) {
        // 4J TODO writeThread.interrupt();
        close(DisconnectPacket::eDisconnect_Closed);
    }
}

int Connection::countDelayedPackets() { return (int)outgoing_slow.size(); }

int Connection::runRead(void* lpParam) {
    ShutdownManager::HasStarted(ShutdownManager::eConnectionReadThreads);
    Connection* con = (Connection*)lpParam;

    if (con == nullptr) {
        return 0;
    }

    Compression::UseDefaultThreadStorage();

    std::mutex* cs = &con->threadCounterLock;

    {
        std::lock_guard<std::mutex> lock(*cs);
        con->readThreads++;
    }

    // try {

    while (
        con->running && !con->quitting &&
        ShutdownManager::ShouldRun(ShutdownManager::eConnectionReadThreads)) {
        while (con->readTick());

        // try {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100L));
        // TODO - 4J Stu - 1.8.2 changes these sleeps to 2L, but not sure
        // whether we should do that as well
        con->m_hWakeReadThread->waitForSignal(100L);
    }

    /* 4J JEV, removed try/catch
    } catch (InterruptedException e) {
    }
    }
    } finally {
    synchronized (threadCounterLock) {
    readThreads--;
    }
    } */

    ShutdownManager::HasFinished(ShutdownManager::eConnectionReadThreads);
    return 0;
}

int Connection::runWrite(void* lpParam) {
    ShutdownManager::HasStarted(ShutdownManager::eConnectionWriteThreads);
    Connection* con = dynamic_cast<Connection*>((Connection*)lpParam);

    if (con == nullptr) {
        ShutdownManager::HasFinished(ShutdownManager::eConnectionWriteThreads);
        return 0;
    }

    Compression::UseDefaultThreadStorage();

    std::mutex* cs = &con->threadCounterLock;

    {
        std::lock_guard<std::mutex> lock(*cs);
        con->writeThreads++;
    }

    // 4J Stu - Adding this to force us to run through the writeTick at least
    // once after the event is fired Otherwise there is a race between the
    // calling thread setting the running flag and this loop checking the
    // condition
    unsigned int waitResult = C4JThread::WaitResult::Timeout;

    while (
        (con->running || waitResult == C4JThread::WaitResult::Signaled) &&
        ShutdownManager::ShouldRun(ShutdownManager::eConnectionWriteThreads)) {
        while (con->writeTick());

        // std::this_thread::sleep_for(std::chrono::milliseconds(100L));
        //  TODO - 4J Stu - 1.8.2 changes these sleeps to 2L, but not sure
        //  whether we should do that as well
        waitResult = con->m_hWakeWriteThread->waitForSignal(100L);

        if (con->bufferedDos != nullptr) con->bufferedDos->flush();
        // if (con->byteArrayDos != nullptr) con->byteArrayDos->flush();
    }

    // 4J was in a finally block.
    {
        std::lock_guard<std::mutex> lock(*cs);
        con->writeThreads--;
    }

    ShutdownManager::HasFinished(ShutdownManager::eConnectionWriteThreads);
    return 0;
}

int Connection::runClose(void* lpParam) {
    Connection* con = dynamic_cast<Connection*>((Connection*)lpParam);

    if (con == nullptr) return 0;

    // try {

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    if (con->running) {
        // 4J TODO writeThread.interrupt();
        con->close(DisconnectPacket::eDisconnect_Closed);
    }

    /* 4J Jev, removed try/catch
    } catch (Exception e) {
    e.printStackTrace();
    } */

    return 1;
}

int Connection::runSendAndQuit(void* lpParam) {
    Connection* con = dynamic_cast<Connection*>((Connection*)lpParam);
    //	printf("Con:0x%x runSendAndQuit\n",con);

    if (con == nullptr) return 0;

    // try {

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    if (con->running) {
        // 4J TODO writeThread.interrupt();
        con->close(DisconnectPacket::eDisconnect_Closed);
        //		printf("Con:0x%x runSendAndQuit close\n",con);
    }

    //	printf("Con:0x%x runSendAndQuit end\n",con);
    /* 4J Jev, removed try/catch
    } catch (Exception e) {
    e.printStackTrace();
    } */

    return 0;
}

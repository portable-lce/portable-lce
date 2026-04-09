#include "Socket.h"

#include <stdio.h>

#include <chrono>
#include <thread>
#include <vector>

#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "minecraft/server/network/ServerConnection.h"
#include "platform/NetTypes.h"
#include "platform/ShutdownManager.h"

class SocketAddress {};

// This current socket implementation is for the creation of a single local
// link. 2 sockets can be created, one for either end of this local link, the
// end (0 or 1) is passed as a parameter to the ctor.

std::mutex Socket::s_hostQueueLock[2];
std::queue<std::uint8_t> Socket::s_hostQueue[2];
Socket::SocketOutputStreamLocal* Socket::s_hostOutStream[2];
Socket::SocketInputStreamLocal* Socket::s_hostInStream[2];
ServerConnection* Socket::s_serverConnection = nullptr;

void Socket::EnsureStreamsInitialised() {
    // Thread-safe one-time initialisation via C++11 magic-statics guarantee.
    // The lambda body runs exactly once no matter how many threads call
    // concurrently.
    static bool initialized = []() -> bool {
        for (int i = 0; i < 2; i++) {
            s_hostOutStream[i] = new SocketOutputStreamLocal(i);
            s_hostInStream[i] = new SocketInputStreamLocal(i);
        }
        return true;
    }();
    (void)initialized;
}

void Socket::Initialise(ServerConnection* serverConnection) {
    s_serverConnection = serverConnection;

    // Ensure the host-local stream objects exist (idempotent).
    EnsureStreamsInitialised();

    // Only initialise everything else once - just setting up static data, one
    // time xrnm things, thread for ticking sockets
    static bool init = false;
    if (init) {
        // Streams already exist – just reset queue state and re-open streams.
        for (int i = 0; i < 2; i++) {
            {
                std::unique_lock<std::mutex> lock(s_hostQueueLock[i],
                                                  std::try_to_lock);
                if (lock.owns_lock()) {
                    // Clear the queue
                    std::queue<std::uint8_t> empty;
                    std::swap(s_hostQueue[i], empty);
                }
            }
            s_hostOutStream[i]->m_streamOpen = true;
            s_hostInStream[i]->m_streamOpen = true;
        }
        return;
    }
    init = true;
    // Streams are already guaranteed to exist via EnsureStreamsInitialised()
    // above. Nothing more to do for the first call.
}

Socket::Socket(bool response) {
    m_hostServerConnection = true;
    m_hostLocal = true;
    if (response) {
        m_end = SOCKET_SERVER_END;
    } else {
        m_end = SOCKET_CLIENT_END;
        Socket* socket = new Socket(1);
        if (s_serverConnection != nullptr) {
            s_serverConnection->NewIncomingSocket(socket);
        } else {
            fprintf(
                stderr,
                "SOCKET: Warning - attempted to notify server of new incoming "
                "socket but s_serverConnection is nullptr\n");
        }
    }

    for (int i = 0; i < 2; i++) {
        m_endClosed[i] = false;
    }
    m_socketClosedEvent = nullptr;
    createdOk = true;
    networkPlayerSmallId = g_NetworkManager.GetHostPlayer()->GetSmallId();
}

Socket::Socket(INetworkPlayer* player, bool response /* = false*/,
               bool hostLocal /*= false*/) {
    m_hostServerConnection = false;
    m_hostLocal = hostLocal;

    for (int i = 0; i < 2; i++) {
        m_inputStream[i] = nullptr;
        m_outputStream[i] = nullptr;
        m_endClosed[i] = false;
    }

    if (!response || hostLocal) {
        m_inputStream[0] = new SocketInputStreamNetwork(this, 0);
        m_outputStream[0] = new SocketOutputStreamNetwork(this, 0);
        m_end = SOCKET_CLIENT_END;
    }
    if (response || hostLocal) {
        m_inputStream[1] = new SocketInputStreamNetwork(this, 1);
        m_outputStream[1] = new SocketOutputStreamNetwork(this, 1);
        m_end = SOCKET_SERVER_END;
    }
    m_socketClosedEvent = new C4JThread::Event;
    // printf("New socket made %s\n", player->GetGamertag() );
    networkPlayerSmallId = player->GetSmallId();
    createdOk = true;
}

SocketAddress* Socket::getRemoteSocketAddress() { return nullptr; }

INetworkPlayer* Socket::getPlayer() {
    return g_NetworkManager.GetPlayerBySmallId(networkPlayerSmallId);
}

void Socket::setPlayer(INetworkPlayer* player) {
    if (player != nullptr) {
        networkPlayerSmallId = player->GetSmallId();
    } else {
        networkPlayerSmallId = 0;
    }
}

void Socket::pushDataToQueue(const std::uint8_t* pbData, std::size_t dataSize,
                             bool fromHost /*= true*/) {
    int queueIdx = SOCKET_CLIENT_END;
    if (!fromHost) queueIdx = SOCKET_SERVER_END;

    if (queueIdx != m_end && !m_hostLocal) {
        fprintf(
            stderr,
            "SOCKET: Error pushing data to queue. End is %d but queue idx id "
            "%d\n",
            m_end, queueIdx);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueLockNetwork[queueIdx]);
        for (std::size_t i = 0; i < dataSize; ++i) {
            m_queueNetwork[queueIdx].push(*pbData++);
        }
    }
}

void Socket::addIncomingSocket(Socket* socket) {
    if (s_serverConnection != nullptr) {
        s_serverConnection->NewIncomingSocket(socket);
    }
}

InputStream* Socket::getInputStream(bool isServerConnection) {
    if (!m_hostServerConnection) {
        if (m_hostLocal) {
            if (isServerConnection) {
                return m_inputStream[SOCKET_SERVER_END];
            } else {
                return m_inputStream[SOCKET_CLIENT_END];
            }
        } else {
            return m_inputStream[m_end];
        }
    } else {
        if (s_hostInStream[m_end] == nullptr) {
            fprintf(stderr,
                    "SOCKET: Warning - s_hostInStream[%d] is nullptr in "
                    "getInputStream(); calling EnsureStreamsInitialised()\n",
                    m_end);
            EnsureStreamsInitialised();
        }
        return s_hostInStream[m_end];
    }
}

void Socket::setSoTimeout(int a) {}

void Socket::setTrafficClass(int a) {}

Socket::SocketOutputStream* Socket::getOutputStream(bool isServerConnection) {
    if (!m_hostServerConnection) {
        if (m_hostLocal) {
            if (isServerConnection) {
                return m_outputStream[SOCKET_SERVER_END];
            } else {
                return m_outputStream[SOCKET_CLIENT_END];
            }
        } else {
            return m_outputStream[m_end];
        }
    } else {
        int outIdx = 1 - m_end;
        if (s_hostOutStream[outIdx] == nullptr) {
            fprintf(stderr,
                    "SOCKET: Warning - s_hostOutStream[%d] is nullptr in "
                    "getOutputStream(); calling EnsureStreamsInitialised()\n",
                    outIdx);
            EnsureStreamsInitialised();
        }
        return s_hostOutStream[outIdx];
    }
}

bool Socket::close(bool isServerConnection) {
    bool allClosed = false;
    if (m_hostLocal) {
        if (isServerConnection) {
            m_endClosed[SOCKET_SERVER_END] = true;
            if (m_endClosed[SOCKET_CLIENT_END]) {
                allClosed = true;
            }
        } else {
            m_endClosed[SOCKET_CLIENT_END] = true;
            if (m_endClosed[SOCKET_SERVER_END]) {
                allClosed = true;
            }
        }
    } else {
        allClosed = true;
        m_endClosed[m_end] = true;
    }
    if (allClosed && m_socketClosedEvent != nullptr) {
        m_socketClosedEvent->set();
    }
    if (allClosed) createdOk = false;
    return allClosed;
}

/////////////////////////////////// Socket for input, on local connection
///////////////////////

Socket::SocketInputStreamLocal::SocketInputStreamLocal(int queueIdx) {
    m_streamOpen = true;
    m_queueIdx = queueIdx;
}

// Try and get an input byte, blocking until one is available
int Socket::SocketInputStreamLocal::read() {
    while (m_streamOpen && ShutdownManager::ShouldRun(
                               ShutdownManager::eConnectionReadThreads)) {
        {
            std::unique_lock<std::mutex> lock(s_hostQueueLock[m_queueIdx],
                                              std::try_to_lock);
            if (lock.owns_lock()) {
                if (s_hostQueue[m_queueIdx].size()) {
                    std::uint8_t retval = s_hostQueue[m_queueIdx].front();
                    s_hostQueue[m_queueIdx].pop();
                    return retval;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return -1;
}

// Try and get an input array of bytes, blocking until enough bytes are
// available
int Socket::SocketInputStreamLocal::read(std::vector<uint8_t>& b) {
    return read(b, 0, b.size());
}

// Try and get an input range of bytes, blocking until enough bytes are
// available
int Socket::SocketInputStreamLocal::read(std::vector<uint8_t>& b,
                                         unsigned int offset,
                                         unsigned int length) {
    while (m_streamOpen) {
        {
            std::unique_lock<std::mutex> lock(s_hostQueueLock[m_queueIdx],
                                              std::try_to_lock);
            if (lock.owns_lock()) {
                if (s_hostQueue[m_queueIdx].size() >= length) {
                    for (unsigned int i = 0; i < length; i++) {
                        b[i + offset] = s_hostQueue[m_queueIdx].front();
                        s_hostQueue[m_queueIdx].pop();
                    }
                    return length;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return -1;
}

void Socket::SocketInputStreamLocal::close() {
    m_streamOpen = false;
    {
        std::lock_guard<std::mutex> lock(s_hostQueueLock[m_queueIdx]);
        std::queue<std::uint8_t>().swap(s_hostQueue[m_queueIdx]);
    }
}

/////////////////////////////////// Socket for output, on local connection
///////////////////////

Socket::SocketOutputStreamLocal::SocketOutputStreamLocal(int queueIdx) {
    m_streamOpen = true;
    m_queueIdx = queueIdx;
}

void Socket::SocketOutputStreamLocal::write(unsigned int b) {
    if (m_streamOpen != true) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(s_hostQueueLock[m_queueIdx]);
        s_hostQueue[m_queueIdx].push((std::uint8_t)b);
    }
}

void Socket::SocketOutputStreamLocal::write(const std::vector<uint8_t>& b) {
    write(b, 0, b.size());
}

void Socket::SocketOutputStreamLocal::write(const std::vector<uint8_t>& b,
                                            unsigned int offset,
                                            unsigned int length) {
    if (m_streamOpen != true) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(s_hostQueueLock[m_queueIdx]);
        for (unsigned int i = 0; i < length; i++) {
            s_hostQueue[m_queueIdx].push(b[offset + i]);
        }
    }
}

void Socket::SocketOutputStreamLocal::close() {
    m_streamOpen = false;
    {
        std::lock_guard<std::mutex> lock(s_hostQueueLock[m_queueIdx]);
        std::queue<std::uint8_t>().swap(s_hostQueue[m_queueIdx]);
    }
}

/////////////////////////////////// Socket for input, on network connection
///////////////////////

Socket::SocketInputStreamNetwork::SocketInputStreamNetwork(Socket* socket,
                                                           int queueIdx) {
    m_streamOpen = true;
    m_queueIdx = queueIdx;
    m_socket = socket;
}

// Try and get an input byte, blocking until one is available
int Socket::SocketInputStreamNetwork::read() {
    while (m_streamOpen && ShutdownManager::ShouldRun(
                               ShutdownManager::eConnectionReadThreads)) {
        {
            std::unique_lock<std::mutex> lock(
                m_socket->m_queueLockNetwork[m_queueIdx], std::try_to_lock);
            if (lock.owns_lock()) {
                if (m_socket->m_queueNetwork[m_queueIdx].size()) {
                    std::uint8_t retval =
                        m_socket->m_queueNetwork[m_queueIdx].front();
                    m_socket->m_queueNetwork[m_queueIdx].pop();
                    return retval;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return -1;
}

// Try and get an input array of bytes, blocking until enough bytes are
// available
int Socket::SocketInputStreamNetwork::read(std::vector<uint8_t>& b) {
    return read(b, 0, b.size());
}

// Try and get an input range of bytes, blocking until enough bytes are
// available
int Socket::SocketInputStreamNetwork::read(std::vector<uint8_t>& b,
                                           unsigned int offset,
                                           unsigned int length) {
    while (m_streamOpen) {
        {
            std::unique_lock<std::mutex> lock(
                m_socket->m_queueLockNetwork[m_queueIdx], std::try_to_lock);
            if (lock.owns_lock()) {
                if (m_socket->m_queueNetwork[m_queueIdx].size() >= length) {
                    for (unsigned int i = 0; i < length; i++) {
                        b[i + offset] =
                            m_socket->m_queueNetwork[m_queueIdx].front();
                        m_socket->m_queueNetwork[m_queueIdx].pop();
                    }
                    return length;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return -1;
}

void Socket::SocketInputStreamNetwork::close() { m_streamOpen = false; }

/////////////////////////////////// Socket for output, on network connection
///////////////////////

Socket::SocketOutputStreamNetwork::SocketOutputStreamNetwork(Socket* socket,
                                                             int queueIdx) {
    m_queueIdx = queueIdx;
    m_socket = socket;
    m_streamOpen = true;
}

void Socket::SocketOutputStreamNetwork::write(unsigned int b) {
    if (m_streamOpen != true) return;
    std::uint8_t bb = (std::uint8_t)b;
    std::vector<uint8_t> barray(1, bb);
    write(barray, 0, 1);
}

void Socket::SocketOutputStreamNetwork::write(const std::vector<uint8_t>& b) {
    write(b, 0, b.size());
}

void Socket::SocketOutputStreamNetwork::write(const std::vector<uint8_t>& b,
                                              unsigned int offset,
                                              unsigned int length) {
    writeWithFlags(b, offset, length, 0);
}

void Socket::SocketOutputStreamNetwork::writeWithFlags(
    const std::vector<uint8_t>& b, unsigned int offset, unsigned int length,
    int flags) {
    if (m_streamOpen != true) return;
    if (length == 0) return;

    // If this is a local connection, don't bother going through QNet as it just
    // delivers it straight anyway
    if (m_socket->m_hostLocal) {
        // We want to write to the queue for the other end of this socket stream
        int queueIdx = m_queueIdx;
        if (queueIdx == SOCKET_CLIENT_END)
            queueIdx = SOCKET_SERVER_END;
        else
            queueIdx = SOCKET_CLIENT_END;

        {
            std::lock_guard<std::mutex> lock(
                m_socket->m_queueLockNetwork[queueIdx]);
            for (unsigned int i = 0; i < length; i++) {
                m_socket->m_queueNetwork[queueIdx].push(b[offset + i]);
            }
        }
    } else {
        XRNM_SEND_BUFFER buffer;
        buffer.pbyData = const_cast<uint8_t*>(&b[offset]);
        buffer.dwDataSize = length;

        INetworkPlayer* hostPlayer = g_NetworkManager.GetHostPlayer();
        if (hostPlayer == nullptr) {
            fprintf(
                stderr,
                "Trying to write to network, but the hostPlayer is nullptr\n");
            return;
        }
        INetworkPlayer* socketPlayer = m_socket->getPlayer();
        if (socketPlayer == nullptr) {
            fprintf(stderr,
                    "Trying to write to network, but the socketPlayer is "
                    "nullptr\n");
            return;
        }

        bool lowPriority = false;
        bool requireAck = ((flags & NON_QNET_SENDDATA_ACK_REQUIRED) ==
                           NON_QNET_SENDDATA_ACK_REQUIRED);

        if (m_queueIdx == SOCKET_SERVER_END) {
            // printf( "Sent %u bytes of data from \"%s\" to \"%s\"\n",
            // buffer.dwDataSize,
            // hostPlayer->GetGamertag(),
            // m_socket->networkPlayer->GetGamertag());

            hostPlayer->SendData(socketPlayer, buffer.pbyData,
                                 buffer.dwDataSize, lowPriority, requireAck);

            // 		uint32_t queueSize = hostPlayer->GetSendQueueSize(
            // nullptr, QNET_GETSENDQUEUESIZE_BYTES  ); 		if(
            // queueSize > 24000 )
            // 		{
            // 			//printf("Queue size is: %d, forcing
            // doWork()\n",queueSize); g_NetworkManager.DoWork();
            // 		}
        } else {
            // printf( "Sent %u bytes of data from \"%s\" to \"%s\"\n",
            // buffer.dwDataSize,
            // m_socket->networkPlayer->GetGamertag(),
            // hostPlayer->GetGamertag());

            socketPlayer->SendData(hostPlayer, buffer.pbyData,
                                   buffer.dwDataSize, lowPriority, requireAck);
        }
    }
}

void Socket::SocketOutputStreamNetwork::close() { m_streamOpen = false; }

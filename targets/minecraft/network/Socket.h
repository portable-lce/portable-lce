#pragma once
#include <cstddef>
#include <cstdint>
#include <format>
#include <vector>
#ifndef __linux__
#include <qnet.h>
#include <xrnm.h>
#endif
#include <mutex>
#include <queue>

#include "java/InputOutputStream/InputStream.h"
#include "java/InputOutputStream/OutputStream.h"
#include "platform/network/network.h"
#include "platform/C4JThread.h"

class INetworkPlayer;

#define SOCKET_CLIENT_END 0
#define SOCKET_SERVER_END 1

// Flag bit for Socket::SocketOutputStream::writeWithFlags. Marks
// the message as needing positive acknowledgement before the caller
// can consider it delivered. Defined here so consumers don't have to
// drag in the entire CGameNetworkManager header just for this constant.
inline constexpr int NON_QNET_SENDDATA_ACK_REQUIRED = 1;

class SocketAddress;
class ServerConnection;

class Socket {
public:
    // 4J Added so we can add a priority write function
    class SocketOutputStream : public OutputStream {
    public:
        // The flags are those that can be used for the QNet SendData function
        virtual void writeWithFlags(const std::vector<uint8_t>& b,
                                    unsigned int offset, unsigned int length,
                                    int flags) {
            write(b, offset, length);
        }
    };

private:
    class SocketInputStreamLocal : public InputStream {
    public:
        bool m_streamOpen;

    private:
        int m_queueIdx;

    public:
        SocketInputStreamLocal(int queueIdx);

        virtual int read();
        virtual int read(std::vector<uint8_t>& b);
        virtual int read(std::vector<uint8_t>& b, unsigned int offset,
                         unsigned int length);
        virtual void close();
        virtual int64_t skip(int64_t n) {
            return n;
        }  // 4J Stu - Not implemented
        virtual void flush() {}
    };

    class SocketOutputStreamLocal : public SocketOutputStream {
    public:
        bool m_streamOpen;

    private:
        int m_queueIdx;

    public:
        SocketOutputStreamLocal(int queueIdx);

        virtual void write(unsigned int b);
        virtual void write(const std::vector<uint8_t>& b);
        virtual void write(const std::vector<uint8_t>& b, unsigned int offset,
                           unsigned int length);
        virtual void close();
        virtual void flush() {}
    };

    class SocketInputStreamNetwork : public InputStream {
        bool m_streamOpen;
        int m_queueIdx;
        Socket* m_socket;

    public:
        SocketInputStreamNetwork(Socket* socket, int queueIdx);

        virtual int read();
        virtual int read(std::vector<uint8_t>& b);
        virtual int read(std::vector<uint8_t>& b, unsigned int offset,
                         unsigned int length);
        virtual void close();
        virtual int64_t skip(int64_t n) {
            return n;
        }  // 4J Stu - Not implemented
        virtual void flush() {}
    };
    class SocketOutputStreamNetwork : public SocketOutputStream {
        bool m_streamOpen;
        int m_queueIdx;
        Socket* m_socket;

    public:
        SocketOutputStreamNetwork(Socket* socket, int queueIdx);

        virtual void write(unsigned int b);
        virtual void write(const std::vector<uint8_t>& b);
        virtual void write(const std::vector<uint8_t>& b, unsigned int offset,
                           unsigned int length);
        virtual void writeWithFlags(const std::vector<uint8_t>& b,
                                    unsigned int offset, unsigned int length,
                                    int flags);
        virtual void close();
        virtual void flush() {}
    };

    bool m_hostServerConnection;  // true if this is the connection between the
                                  // host player and server
    bool m_hostLocal;  // true if this player on the same machine as the host
    int m_end;         // 0 for client side or 1 for host side

    // For local connections between the host player and the server
    static std::mutex s_hostQueueLock[2];
    static std::queue<std::uint8_t> s_hostQueue[2];
    static SocketOutputStreamLocal* s_hostOutStream[2];
    static SocketInputStreamLocal* s_hostInStream[2];

    // For network connections
    std::queue<std::uint8_t> m_queueNetwork[2];  // For input data
    std::mutex m_queueLockNetwork[2];            // For input data
    SocketInputStreamNetwork* m_inputStream[2];
    SocketOutputStreamNetwork* m_outputStream[2];
    bool m_endClosed[2];

    // Host only connection class
    static ServerConnection* s_serverConnection;

    std::uint8_t networkPlayerSmallId;

public:
    C4JThread::Event* m_socketClosedEvent;

    INetworkPlayer* getPlayer();
    void setPlayer(INetworkPlayer* player);

public:
    static void
    EnsureStreamsInitialised();  // 4J Fix: idempotent stream creation; safe to
                                 // call before Initialise(connection)
    static void Initialise(ServerConnection* serverConnection);
    Socket(bool response = false);  // 4J - Create a local socket, for end 0 or
                                    // 1 of a connection
    Socket(
        INetworkPlayer* player, bool response = false,
        bool hostLocal = false);  // 4J - Create a socket for an INetworkPlayer
    SocketAddress* getRemoteSocketAddress();
    void pushDataToQueue(const std::uint8_t* pbData, std::size_t dataSize,
                         bool fromHost = true);
    static void addIncomingSocket(Socket* socket);
    InputStream* getInputStream(bool isServerConnection);
    void setSoTimeout(int a);
    void setTrafficClass(int a);
    SocketOutputStream* getOutputStream(bool isServerConnection);
    bool close(bool isServerConnection);
    bool createdOk;
    bool isLocal() { return m_hostLocal; }

    bool isClosing() {
        return m_endClosed[SOCKET_CLIENT_END] || m_endClosed[SOCKET_SERVER_END];
    }
    std::uint8_t getSmallId() { return networkPlayerSmallId; }
};

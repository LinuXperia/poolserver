#ifndef STRATUM_CLIENT_H_
#define STRATUM_CLIENT_H_

#include "Common.h"
#include "Log.h"
#include "JSON.h"
#include "Server.h"
#include "Job.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#define MAX_PACKET 2048

using namespace boost;
using namespace boost::asio::ip;

namespace Stratum
{
    class Server;
    
    class Client : public boost::enable_shared_from_this<Client>
    {
    public:
        Client(Server* server, asio::io_service& io_service) : _server(server), _socket(io_service), _subscribed(false), _jobid(0)
        {
        }
        
        tcp::socket& GetSocket()
        {
            return _socket;
        }
        
        void Start()
        {
            // Start reading socket
            StartRead();
        }
        
        void StartRead()
        {
            // Read until newline
            boost::asio::async_read_until(
                _socket,
                _recvBuffer,
                '\n',
                boost::bind(&Client::_OnReceive, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        
        void SendJob(bool clean);
        
        void SendMessage(JSON msg)
        {
            std::string data = msg.ToString();
            data += '\n';
            sLog.Debug(LOG_SERVER, "Sending: %s", data.c_str());
            boost::asio::async_write(
                _socket,
                boost::asio::buffer(data.c_str(), data.length()),
                boost::bind(&Client::_OnSend, this, boost::asio::placeholders::error));
        }
        
        void OnMiningSubmit(JSON msg);
        void OnMiningSubscribe(JSON msg);
        void OnMiningAuthorize(JSON msg);
        
        void OnMessage(JSON msg)
        {
            std::string method = msg["method"].GetString();
            sLog.Debug(LOG_SERVER, "Decoded: %s", msg.ToString().c_str());
            
            if (method.compare("mining.submit") == 0)
                OnMiningSubmit(msg);
            else if (method.compare("mining.subscribe") == 0)
                OnMiningSubscribe(msg);
            else if (method.compare("mining.authorize") == 0)
                OnMiningAuthorize(msg);
            else
                sLog.Error(LOG_SERVER, "Method '%s' not found.", method.c_str());
        }
        
        // Gets new job from the server
        Job GetJob();
        
        void Disconnect()
        {
            _socket.close();
        }
    public:
        void _OnReceive(const boost::system::error_code& error, size_t bytes_transferred);
        void _OnSend(const boost::system::error_code& error);
        
    private:
        // Networking
        asio::streambuf _recvBuffer;
        tcp::socket _socket;
        
        // Pointer to server
        Stratum::Server* _server;
        
        // Authorization
        std::vector<std::string> _workers;
        
        // Jobs
        bool _subscribed;
        uint32 _extranonce;
        std::map<uint64, Job> _jobs;
        uint32 _jobid;
    };
    
    typedef boost::shared_ptr<Client> ClientPtr;
}

#endif

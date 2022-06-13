#include <map>
#include <list>
#include <ctime>
#include <deque>
#include <mutex>
#include <array>
#include <memory>
#include <thread>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/enable_shared_from_this.hpp>


std::map <std::string, boost::asio::ip::tcp::socket> owner_of_message;


namespace dispatcher
{
    namespace ip = boost::asio::ip;

    class bridge : public boost::enable_shared_from_this<bridge>
    {
    public:

        typedef ip::tcp::socket socket_type;
        typedef boost::shared_ptr<bridge> ptr_type;

        bridge(boost::asio::io_service& ios)
                : downstream_socket_(ios),
                  upstream_socket_  (ios)
        {}

        socket_type& downstream_socket()
        {
            // Client socket
            return downstream_socket_;
        }

        socket_type& upstream_socket()
        {
            // Routing socket
            return upstream_socket_;
        }

        void start(const std::string& upstream_host, unsigned short upstream_port)
        {
            // Attempt connection to Routing
            std::cout << "start new connection " << upstream_host << ":" << upstream_port << std::endl;
            upstream_socket_.async_connect(
                    ip::tcp::endpoint(
                            boost::asio::ip::address::from_string(upstream_host),
                            upstream_port),
                    boost::bind(&bridge::handle_upstream_connect,
                                shared_from_this(),
                                boost::asio::placeholders::error));
        }

        void handle_upstream_connect(const boost::system::error_code& error)
        {
            if (!error)
            {
                std::cout << "upstream connection started" << std::endl;
                // Setup async read from Routing

                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_,max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                // Setup async read from client
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_,max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }

    private:

        // Process data recieved from Routing then send to client.
        // Read from Routing completed, now send data to client.
        void handle_upstream_read(const boost::system::error_code& error,
                                  const size_t& bytes_transferred)
        {
            if (!error)
            {
                std::cout << "handle_upstream_read function upstream_data_ = " << upstream_data_ << std::endl;
                std::cout << "downstream_socket_ info = " << downstream_socket_.remote_endpoint().port() << std::endl;
                async_write(downstream_socket_,
                            boost::asio::buffer(upstream_data_,bytes_transferred),
                            boost::bind(&bridge::handle_downstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            }
            else
                close();
        }

        // Write to client complete, Async read from Routing.
        void handle_downstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                std::cout << "handle_downstream_write function upstream_data_ = " << upstream_data_ << std::endl;
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_,max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }

        // Process data recieved from client then write to Routing.
        // Read from client complete, now send data to Routing.
        void handle_downstream_read(const boost::system::error_code& error,
                                    const size_t& bytes_transferred)
        {
            if (!error)
            {
                std::cout << "downstream_data_ = " << downstream_data_ << std::endl;

                async_write(upstream_socket_,
                            boost::asio::buffer(downstream_data_, bytes_transferred),
                            boost::bind(&bridge::handle_upstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            }
            else
                close();
        }

        // Write to Routing completed, Async read from client
        void handle_upstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                std::cout << "downstream_data_ = " << downstream_data_ << std::endl;
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_,max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }

        void close()
        {
            boost::mutex::scoped_lock lock(mutex_);

            if (downstream_socket_.is_open())
            {
                downstream_socket_.close();
            }

            if (upstream_socket_.is_open())
            {
                upstream_socket_.close();
            }
        }

        socket_type downstream_socket_;
        socket_type upstream_socket_;

        enum { max_data_length = 1024 };
        unsigned char downstream_data_[max_data_length];
        unsigned char upstream_data_  [max_data_length];

        boost::mutex mutex_;

    public:

        class acceptor
        {
        public:

            acceptor(boost::asio::io_service& io_service,
                     const std::string& local_host, unsigned short local_port,
                     const std::string& upstream_host, unsigned short upstream_port)
                    : io_service_(io_service),
                      localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                      acceptor_(io_service_,ip::tcp::endpoint(localhost_address,local_port)),
                      upstream_port_(upstream_port),
                      upstream_host_(upstream_host)
            {
                accept_connections();
            }

            bool accept_connections()
            {
                try
                {
                    session_ = boost::shared_ptr<bridge>(new bridge(io_service_));

                    acceptor_.async_accept(session_->downstream_socket(),
                                           boost::bind(&acceptor::handle_accept,
                                                       this,
                                                       boost::asio::placeholders::error));
                }
                catch(std::exception& e)
                {
                    std::cerr << "acceptor exception: " << e.what() << std::endl;
                    return false;
                }

                return true;
            }

        private:

            void handle_accept(const boost::system::error_code& error)
            {
                if (!error)
                {
                    session_->start(upstream_host_,upstream_port_);

                    if (!accept_connections())
                    {
                        std::cerr << "Failure during call to accept." << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Error: " << error.message() << std::endl;
                }
            }

            boost::asio::io_service& io_service_;
            ip::address_v4 localhost_address;
            ip::tcp::acceptor acceptor_;
            ptr_type session_;
            unsigned short upstream_port_;
            std::string upstream_host_;
        };
    };
}

namespace
{
    class workerThread
    {
    public:
        static void run(std::shared_ptr<boost::asio::io_service> io_service)
        {
            {
                std::lock_guard < std::mutex > lock(m);
                std::cout << "Thread starts" << std::endl;
            }

            io_service->run();

            {
                std::lock_guard < std::mutex > lock(m);
                std::cout << "Thread ends" << std::endl;
            }
        }
    private:
        static std::mutex m;
    };

    std::mutex workerThread::m;
}


int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        std::cerr << "usage: ./dispatcher_server <local host ip> <local port1> <local port2> ... <local portN> <forward host ip> <forward port>" << std::endl;
        return 1;
    }

    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[argc-1]));
    const std::string local_host      = argv[1];
    const std::string forward_host    = argv[argc-2];

    std::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
    boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io_service));
    boost::shared_ptr<boost::asio::io_service::strand> strand(new boost::asio::io_service::strand(*io_service));

    std::cout << "server starts on following ports : " << std::endl;

    try
    {
        std::list < std::shared_ptr < dispatcher::bridge::acceptor > > servers;
        for (int i = 2; i < argc-2; ++i)
        {
            std::cout << "\t\t-->" << argv[i] << std::endl;
            std::shared_ptr<dispatcher::bridge::acceptor> server(new dispatcher::bridge::acceptor(*io_service, local_host,
                                                                                                  static_cast<unsigned short>(::atoi(argv[i])),
                                                                                                  forward_host, forward_port));
            servers.push_back(server);
        }

        boost::thread_group workers;
        for (int i = 0; i < 1; ++i)
        {
            boost::thread * t = new boost::thread{ boost::bind(&workerThread::run, io_service) };

#ifdef __linux__
            // bind cpu affinity for worker thread in linux
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(i, &cpuset);
            pthread_setaffinity_np(t->native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
            workers.add_thread(t);
        }
        workers.join_all();
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

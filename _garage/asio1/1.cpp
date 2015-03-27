#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/basic_streambuf.hpp>

#include "async_mocks.hxx"

using namespace boost;

template < class Socket, class Strand = asio::io_service::strand >
class Connection : public enable_shared_from_this<Connection<Socket>>{
    public:

    typedef string  buffer_t;
    typedef function<void(buffer_t&&)> read_handler_t;
    typedef function<void(size_t)> write_handler_t;

    Connection(Strand& s, asio::io_service& ios) : strand_(s), socket_(ios) {}
    void async_read(read_handler_t handler) {
            buffer_ = "PIPPO";
            auto buffers = asio::mutable_buffers_1((void*)buffer_.data(), 3 );
            auto complhandler = strand_.wrap([this, _this = this->shared_from_this(), handler ](
                                                             const boost::system::error_code& ec,
                                                             size_t len
                                                                                        ) {
                                        handler( std::move(buffer_));
                                     });

            socket_.async_read_some (buffers, complhandler );
    }
    /*
    void async_write( buffer_t&& buffer, write_handler_t handler) {

            socket_.async_read_some (
                                     buffers,
                                     [this, _this = this->shared_from_this(), handler ](
                                                             const boost::system::error_code& ec,
                                                             size_t len
                                                                                        ) {
                                        handler( std::move(buffer_));
                                     }
               );

    }
    */


    private:

    Strand& strand_;
    Socket socket_;
    mutable buffer_t buffer_;
};



int main() {
    asio::io_service ios;
    asio::io_service ios2;
    asio::io_service::strand s(ios);
    auto reader = std::make_shared<Connection<mock_asio_socket>>(s, ios2);
    auto writer = std::make_shared<Connection<mock_asio_socket>>(s, ios2);

    auto readcallback =  []( string&& buff) {
                cerr << buff << endl;
                 };

    auto writecallback =  []( string&& buff) {
                cerr << buff << endl;
                 };
    reader->async_read( readcallback);
    ios2.run();
}

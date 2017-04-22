#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <cassert>
#include <thread>
#include <future>
#include "../tests/test.h"
#include "http_inbound_connection.hxx"
#include "wrapper/http_parser.hxx"

#include "util/stream_pipe.hxx"

using namespace boost::asio;
using boost::asio::mutable_buffers_1;
using boost::asio::buffer_cast;
using boost::asio::mutable_buffer;
using boost::asio::const_buffers_1;
using boost::asio::const_buffer;
using boost::system::error_code;

using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::shared_ptr;
using std::function;
using std::make_shared;
using std::move;

const size_t bodybuffer_size = 1024;
const size_t readbuffer_size = 1024;

const int srv_port = 60001;

class dropall_stream : public std::enable_shared_from_this<dropall_stream>
{
    io_service& ios_;
    int ignored = 0;

  public:
    dropall_stream(boost::asio::io_service &iosvc) : ios_(iosvc) {}
    auto& get_io_service() {
        return ios_;
    }

    template <typename ConstBufferSequence, typename ReadHandler>
    void async_write_some(
        const ConstBufferSequence &buffers,
        ReadHandler handler)
    {
        auto len = buffer_size(buffers);
        ignored += len;
        CERR << "ignored " << ignored << endl;
        ios_.post([this, handler, buffers, len]() {
            
            handler(boost::system::error_code(), len);
        });
    }
};

int main()
{

    io_service ios;

    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint(ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);

    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    typedef mxmz::connection_tmpl<mxmz::nodejs::http_parser_base> conn_t;

    function<void()> start_accept = [&acceptor, &socket, &start_accept]() {
        acceptor.async_accept(socket, [&socket, &start_accept](boost::system::error_code ec) {
            if (not ec)
            {
                CERR << "new connnection" << endl;

                auto conn = make_shared<conn_t>(move(socket), bodybuffer_size, readbuffer_size);
                conn->async_wait_request([conn](boost::system::error_code ec,
                                                conn_t::http_request_header_ptr h) {
                    CERR << ec << endl;
                    CERR << h->method << endl;
                    CERR << h->url << endl;
                    auto br = conn->make_body_reader();
                    
                    auto drop = make_shared<dropall_stream>(br->get_io_service());
                    auto pipe = mxmz::make_stream_pipe(*br, *drop, 4567);
                    pipe->debug = true;

                    pipe->run( [drop,br](const boost::system::error_code& read_ec, const boost::system::error_code& write_ec) {
                        CERR << read_ec.message() << endl;
                        CERR << write_ec.message() << endl;
                  }  );
                  
                });
            }
            //      start_accept();
        });
    };

    start_accept();
    ios.run();
}

#include "wrapper/detail/http_parser_impl.hxx"
#include "detail/http_inbound_connection_impl.hxx"
#include "util/detail/pimpl.hxx"

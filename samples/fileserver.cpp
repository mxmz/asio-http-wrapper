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

#include "wrapper/detail/http_parser_impl.hxx"
#include "detail/http_inbound_connection_impl.hxx"
#include "util/detail/pimpl.hxx"

#include "util/stream_pipe.hxx"

#include <boost/asio/write.hpp>
#include <boost/filesystem.hpp>

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


template< class StreamT, class Handler >
void http_return_file( long int conn_id, StreamT& socket, const boost::filesystem::path& p, Handler handler  );

long int connections = 0;

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
    acceptor.listen(5000);

    typedef mxmz::inbound_connection_tmpl<mxmz::nodejs::http_parser_base> conn_t;

    function<void()> start_accept = [&acceptor, &socket, &start_accept]() {
        acceptor.async_accept(socket, [&socket, &start_accept](boost::system::error_code ec) {
            if ( ec ) {
                        CERR << "async_accept " << ec << " " << ec.message() << endl;
                        exit(-1);
            }
            if (not ec)
            {
                ++connections;
                auto conn = make_shared<conn_t>(connections, move(socket), bodybuffer_size, readbuffer_size);
                CERR << conn->connection_id() << " new connnection" << endl;

                conn->async_wait_request([conn](boost::system::error_code ec,
                                                conn_t::http_request_header_ptr h) {
                    if ( ec ) {
                        CERR << conn->connection_id() << " async_wait_request " << ec << " " << ec.message() << endl;
                        return;
                    }

                    CERR << ec << endl;
                    CERR << h->method << endl;
                    CERR << h->url << endl;
                    auto br = conn->make_body_reader();
                    
                    auto drop = make_shared<dropall_stream>(br->get_io_service());
                    auto pipe = mxmz::make_stream_pipe(*br, *drop, 4567);
                    pipe->debug = true;

                    auto head = shared_ptr<const mxmz::http_request_header>(h.release()) ;


                    pipe->run( [drop,br,head] (const boost::system::error_code& read_ec, const boost::system::error_code& write_ec) mutable  {
                        CERR << br->connection()->connection_id() << " " << read_ec << " " << read_ec.message() << endl;
                        CERR << br->connection()->connection_id() << " "<< write_ec << " "<< write_ec.message() << endl;

                        if ( not read_ec || read_ec == boost::asio::error::eof) {
                                CERR << br->connection()->connection_id() << " "<< "start respond" << endl;

                                boost::filesystem::path p(".");
                                p += head->url;
                                
                                
                                http_return_file (
                                    br->connection()->connection_id(), 
                                    br->connection()->get_socket(),
                                    p,
                                    [br](boost::system::error_code ec) {
                                           CERR << br->connection()->connection_id() << " "<< "http_return_file " << ec << endl; 
                                    }
                                 );
                                

                        }  else {
                                CERR << br->connection()->connection_id() << " "<< "pipe->run read_ec " << read_ec << endl;
                                exit(-1) ;
                            
                        }      
            
                  }  );
                  
                });
            }
            start_accept();
        });
    };

    start_accept();
    ios.run();
}


#include <sstream>
#include <fstream>

string slurp(const boost::filesystem::path& p) {
    std::ifstream  in(p.string());
    std::stringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

template< class StreamT>
class file_writer : public std::enable_shared_from_this<file_writer<StreamT>> {
    long int   conn_id_;
    StreamT&   stream_;
    string              data_;
    public:
    file_writer( long int conn_id, StreamT& stream, const boost::filesystem::path& p )
        :   conn_id_(conn_id),
            stream_(stream) {
            data_ = move( slurp(p)); 
    }

    size_t size() const {
        return data_.size();
    }


    long int connection_id () const {
        return conn_id_;
    }

    template< class Handler >
    void async_write(Handler handler) {
        auto self( this->shared_from_this() );
        boost::asio::async_write( stream_, 
            const_buffers_1 (data_.data(), data_.size()),
            boost::asio::transfer_all(),
            [self,this,handler]( boost::system::error_code ec, std::size_t bytes ) {
                CERR << connection_id() << " file_writer :: async_write " <<  ec.message() << " " <<bytes << endl;
                handler(ec);
        } );

    }    

};

template< class StreamT >
class http_file_writer : public std::enable_shared_from_this<http_file_writer<StreamT>> {
    long int conn_id_;
    StreamT&                            stream_;
    shared_ptr<file_writer<StreamT>>    file_;
        string                          data_;

    public:
    http_file_writer( long int conn_id, StreamT& stream, const boost::filesystem::path& p )
        :   conn_id_(conn_id),
            stream_(stream),
            file_ ( make_shared<file_writer<StreamT>>(conn_id, stream,p) ) {
           
           char header[1024];

           snprintf( header, 1023,
                      "HTTP/1.0 200 OK\r\n"
                                    "Content-Length: %ld\r\n"
                                    "Content-Type: application/octet-stream\r\n"
                                    "\r\n", file_->size() );
           data_ = header;
                    
    }


    long int connection_id () const {
        return conn_id_;
    }

    template< class Handler >
    void async_write(Handler handler) {
        auto self( this->shared_from_this() );
        // CERR << data_ << endl;
        boost::asio::async_write( stream_, 
            const_buffers_1 (data_.data(), data_.size()),
            boost::asio::transfer_all(),
            [self,this,handler]( boost::system::error_code ec, std::size_t bytes ) {
                if ( not ec ) {
                    file_->async_write(handler);
                } else {
                    CERR << connection_id() << " http_file_writer :: async_write " <<  ec.message() << " " <<bytes << endl;

                    handler(ec);
                }
        } );

    }    

};





template< class StreamT, class Handler >
void http_return_file( long int conn_id, StreamT& socket, const boost::filesystem::path& p, Handler handler  ) {
    make_shared<http_file_writer<StreamT>>(conn_id, socket, p)->async_write(handler);
    
}
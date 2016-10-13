#ifndef socketmocks_hxx_92837492837492834721762537162537165317394857398457394857
#define socketmocks_hxx_92837492837492834721762537162537165317394857398457394857

#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>

#include "test.hxx"


using std::move;
using std::min;
using mxmztest::rand_int;
using boost::asio::mutable_buffers_1;
using boost::asio::buffer_cast;
using boost::asio::mutable_buffer;
using boost::asio::const_buffers_1;
using boost::asio::const_buffer;
using boost::system::error_code;

class mock_asio_socket {
    boost::asio::steady_timer timer_;
    std::string     data_;
    size_t          consumed_ = 0;
    
    public:
    mock_asio_socket(boost::asio::io_service& ios, std::string data ) 
            :   timer_(ios),  data_(move(data))
            { };
    ~mock_asio_socket() {};

    mock_asio_socket( mock_asio_socket&& that ) :
        timer_( that.timer_.get_io_service() ),
        data_( move(that.data_)),
        consumed_( that.consumed_)
        {}


    boost::asio::io_service& get_io_service() { return timer_.get_io_service(); }

    template<    typename ConstBufferSequence,    typename ReadHandler>
     void async_write_some(
        const ConstBufferSequence & buffers,
            ReadHandler handler) {
                timer_.expires_from_now( std::chrono::microseconds(2) );
                timer_.async_wait( [handler, buffers]( const boost::system::error_code& ec ) {
                                    handler( ec, 1 );
                                } );
            }

    template<    typename MutableBufferSequence,    typename ReadHandler>
     void async_read_some(
        const MutableBufferSequence & buffers,
            ReadHandler handler) {
                timer_.expires_from_now( std::chrono::microseconds(0) );
                timer_.async_wait([_handler=move(handler),this,buffers]( const boost::system::error_code& ec ) mutable {
                        if ( consumed_ == data_.size() ) {
                               _handler( boost::asio::error::eof, 0 );    
                        } else {
                            size_t len = min( size_t(10 + rand_int() % 65535)  , (data_.size() - consumed_) );
                            size_t used = buffer_copy( buffers, const_buffers_1(data_.data() + consumed_,len) );
                            consumed_ += used;
                            _handler( ec, used );
                        }
                    } );
            }




protected:

private:

};




#endif 

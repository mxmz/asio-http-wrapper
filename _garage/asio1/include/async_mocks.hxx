#ifndef ASYNC_MOCKS_HXX_H
#define ASYNC_MOCKS_HXX_H

#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

using namespace boost;

class mock_asio_socket
{
public:
    mock_asio_socket(boost::asio::io_service& ios ) :  timer_(ios) { };
    ~mock_asio_socket() {};

    template<    typename ConstBufferSequence,    typename ReadHandler>
     void async_write_some(
        const ConstBufferSequence & buffers,
            ReadHandler handler) {
                timer_.expires_from_now( std::chrono::seconds(2) );
                timer_.async_wait( [handler, buffers]( const boost::system::error_code& ec ) {
                                    handler( ec, 1 );
                                } );
            }

    template<    typename MutableBufferSequence,    typename ReadHandler>
     void async_read_some(
        const MutableBufferSequence & buffers,
            ReadHandler handler) {
                timer_.expires_from_now( std::chrono::seconds(3) );

                timer_.async_wait([hndl=std::move(handler),buffers]( const boost::system::error_code& ec ) mutable {
                                  asio::mutable_buffer b0 =   *buffers.begin();
                                  auto buff =   asio::buffer_cast<char*>(b0);
                                  cerr << asio::buffer_size(b0) << endl;
                                  buff[0] = 'W';
                                    hndl( ec, 1 );
                                } );
            }




protected:

private:
    boost::asio::steady_timer timer_;
};




#endif // ASYNC_MOCKS_HXX_H

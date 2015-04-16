#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace boost;



#include "socket_pipe.hxx"

typedef ssocket_pipe_tmpl< asio::ip::tcp::socket > tcp_socket_pipe;


#include "ring_buffer.hxx"

int main()
{

    ring_buffer  rb(1000);

    {
        auto buf = rb.prepare();

        cerr << buffer_size(buf) << endl;

        rb.commit(300);

        auto data = rb.data();

        cerr << buffer_size(data) << endl;

        rb.consume( 1 );

    }


    {
        auto buf = rb.prepare();

        cerr << buffer_size(buf) << endl;

        rb.commit(700);
    }
    {
        auto buf = rb.prepare();

        cerr << "--->" << buffer_size(buf) << endl;

        auto data = rb.data();

        cerr << buffer_size(data) << endl;

    }
    {
        rb.consume(1);
        auto buf = rb.prepare();

        cerr << "--->" << buffer_size(buf) << endl;

        auto data = rb.data();

        cerr << buffer_size(data) << endl;

    }



        asio::io_service ios;
        asio::io_service ios2;
        asio::ip::tcp::socket src(ios);
        asio::ip::tcp::socket dst(ios);

        src.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), 9001) );
        dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("192.168.69.23"), 9002) );
        //dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), 9002) );

        auto pipe = std::make_shared<tcp_socket_pipe>(std::move(src), std::move(dst));

        pipe->run( [pipe](const system::error_code& read_ec, asio::ip::tcp::socket&& read_socket, const system::error_code& write_ec, asio::ip::tcp::socket&& write_socket) {
                    cerr << read_ec.message() << endl;
                    cerr << write_ec.message() << endl;

                  }  );
        ios.run();


}


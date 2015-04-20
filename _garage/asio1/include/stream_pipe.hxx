#ifndef stream_pipe_9384759387593845793845793485739485739485739
#define stream_pipe_9384759387593845793845793485739485739485739

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
#include <boost/asio/streambuf.hpp>

#include "ring_buffer.hxx"

using namespace boost;


int debug_count_read = 0;
int debug_count_write = 0;


#define cerr if(0) cerr

template< class SrcStream,  class DstStream = SrcStream, class Strand = asio::io_service::strand >
class stream_pipe_tmpl: public std::enable_shared_from_this<stream_pipe_tmpl<SrcStream,DstStream,Strand>>
{

public:
    stream_pipe_tmpl( SrcStream&& src, DstStream&& dst )
        :   src_stream_(std::move(src)),
            reading_(false),
            dst_stream_(std::move(dst)),
            writing_(false),
            sbuf_(1024*10024*3),
            strand_(src.get_io_service() ),
            finished_(false)
    {
        assert( &src.get_io_service() == &dst.get_io_service() );

    }

    typedef std::function< void(const system::error_code&, SrcStream&&, const system::error_code& write_ec_, SrcStream&& )> completion_handler;

    void run( completion_handler hndl )
    {

        completion_ = std::move(hndl);
        activate();
    }

private:
    void finish()
    {
        cerr << "finished " << debug_count_read << " " << debug_count_write << endl;
        if ( not finished_ )
        {
            finished_ = true;
            completion_( read_ec_, std::move(src_stream_), write_ec_, std::move(dst_stream_) );
        }

    }
    void activate()
    {
        cerr << "full: " << sbuf_.full() << ", empty: " << sbuf_.empty() << endl;
        cerr << "reading " << reading_ << " writing " << writing_ << endl;
        cerr << "read_ec_ " << read_ec_ << " write_ec_" << write_ec_ << endl;

        if( not sbuf_.full() and not reading_ and not finished_ and not read_ec_ )
        {
            auto _this = this->shared_from_this();
            auto handler = strand_.wrap([this,_this ](const system::error_code& ec, std::size_t bytes_transferred)
            {
                reading_ = false;
                if ( not ec )
                {
                    auto bufs = sbuf_.prepare();
                    auto rv = src_stream_.receive(bufs, 0, read_ec_ );
                    cerr << "**** " << buffer_size(bufs) <<  " " << read_ec_  << endl;
                    assert( buffer_size(bufs) > 0 );
                    cerr << "read " << rv << " is writing: " << writing_ << endl;
                    sbuf_.commit(rv);
                    debug_count_read += rv;
                    activate();

                }
                else
                {
                    read_ec_ = ec;
                    activate();
                }
            });
            src_stream_.async_read_some( asio::null_buffers(), handler );
            reading_ = true;
        }

        if ( sbuf_.empty() and read_ec_ and not finished_ )
        {
            finish();
        }
        else if ( not writing_ and not sbuf_.empty() and not finished_)
        {
            auto _this = this->shared_from_this();
            auto handler = strand_.wrap([this,_this ](const boost::system::error_code& ec, std::size_t bytes_transferred)
            {
                writing_ = false;
                if ( not ec )
                {
                    auto bufs = sbuf_.data();
                    assert( buffer_size(bufs) > 0 );
                    auto rv = dst_stream_.send( bufs,0, write_ec_ );
                    cerr << "write " << rv << " is reading: " << reading_ << endl;
                    sbuf_.consume(rv);
                    debug_count_write += rv;
                    activate();
                }
                else
                {
                    write_ec_ = ec;
                    finish();
                }
            });
            dst_stream_.async_write_some( asio::null_buffers(), handler );
            writing_ = true;
        }




    }



private:
    SrcStream src_stream_;
    system::error_code read_ec_;
    bool      reading_;

    DstStream dst_stream_;
    system::error_code write_ec_;
    bool      writing_;

    ring_buffer  sbuf_;
    Strand strand_;
    completion_handler completion_;
    bool      finished_;
};




#undef cerr












#endif // stream_pipe_9384759387593845793845793485739485739485739


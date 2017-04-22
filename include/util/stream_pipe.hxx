#ifndef stream_pipe_9384759387593845793845793485739485739485739
#define stream_pipe_9384759387593845793845793485739485739485739
#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>

#include "ring_buffer.hxx"


int debug_count_read = 0;
int debug_count_write = 0;

using std::cerr;
using std::endl;

#define cerr if(debug) cerr

namespace mxmz { //

template< class SrcStream,  
        class DstStream = SrcStream, 
        class Strand = boost::asio::io_service::strand >
class stream_pipe_tmpl: public std::enable_shared_from_this<stream_pipe_tmpl<SrcStream,DstStream,Strand>>
{
    public:
    static bool debug;

public:
    stream_pipe_tmpl( SrcStream& src, DstStream& dst, size_t buffer_size )
        :   src_stream_(src),
            reading_(false),
            dst_stream_(dst),
            writing_(false),
            sbuf_(buffer_size),
            strand_(src.get_io_service() ),
            finished_(false)
    {
        assert( &src.get_io_service() == &dst.get_io_service() );

    }

    typedef std::function< void(const boost::system::error_code& read_ec, const boost::system::error_code& write_ec )> completion_handler;

    void run( completion_handler hndl )
    {
        completion_ = std::move(hndl);
        activate();
    }

private:
    void finish()
    {
        CERR << "finished " << debug_count_read << " " << debug_count_write << endl;
        if ( not finished_ and not writing_ and not reading_ )
        {
            finished_ = true;
            completion_( read_ec_, write_ec_ );
        }

    }

    void activate()
    {
        CERR << "writable: " << sbuf_.writable() << "  - readable: " << sbuf_.readable() << endl;
        CERR << "full: " << sbuf_.full() << " - empty: " << sbuf_.empty() << endl;
        CERR << "reading: " << reading_ << " - writing: " << writing_ << endl;
        CERR << "read_ec_: " << read_ec_ << " - write_ec_: " << write_ec_ << endl;

        if( not sbuf_.full() and not reading_ and not finished_ and not read_ec_ )
        {
            auto _this = this->shared_from_this();
            auto handler = strand_.wrap([this,_this ](const boost::system::error_code& ec, std::size_t bytes_transferred)
            {
                CERR << "read handler " << ec << " " << bytes_transferred << endl;
                if ( bytes_transferred )
                {
                   // auto bufs = sbuf_.prepare();
                   // auto rv = src_stream_.read_some(bufs, read_ec_ );
                   // CERR << "**** " << buffer_size(bufs) <<  " " << read_ec_  << endl;
                   // assert( buffer_size(bufs) > 0 );
                   //  CERR << "read: " << rv << " is_writing_: " << writing_ << endl;
                   // sbuf_.commit(rv);
                    sbuf_.commit(bytes_transferred);
                    debug_count_read += bytes_transferred;
                 
                }
                {
                    read_ec_ = ec;
                    reading_ = false;
                    activate();
                }
            });
            //src_stream_.async_read_some( boost::asio::null_buffers(), handler );
            auto bufs = sbuf_.prepare();
            src_stream_.async_read_some( bufs, handler );
            reading_ = true;
        }

        if ( sbuf_.empty() and read_ec_ and not finished_ )
        {
            finish();
        }
        if ( sbuf_.full() and write_ec_ and not finished_ )
        {
            finish();
        }
        else if ( not writing_ and not sbuf_.empty() and not write_ec_ and not finished_)
        {
            auto _this = this->shared_from_this();
            auto handler = strand_.wrap([this,_this ](const boost::system::error_code& ec, std::size_t bytes_transferred)
            {
                CERR << "write handler " << ec << " " << bytes_transferred << endl;
                if ( not ec )
                {
                    //auto bufs = sbuf_.data();
                    //assert( buffer_size(bufs) > 0 );
                    //auto rv = dst_stream_.write_some( bufs, write_ec_ );
                    //CERR << "write " << rv << " is_reading_: " << reading_ << endl;
                    sbuf_.consume(bytes_transferred);
                    debug_count_write += bytes_transferred;
                    writing_ = false;
                    activate();
                }
                else
                {
                    write_ec_ = ec;
                    writing_ = false;
                    finish();
                }
            });
            auto bufs = sbuf_.data();
            dst_stream_.async_write_some( bufs, handler );
            writing_ = true;
        }




    }



private:
    SrcStream& src_stream_;
    boost::system::error_code read_ec_;
    bool      reading_;

    DstStream& dst_stream_;
    boost::system::error_code write_ec_;
    bool      writing_;

    mxmz::ring_buffer  sbuf_;
    Strand strand_;
    completion_handler completion_;
    bool      finished_;
};



template< class SrcStream,  
        class DstStream, 
        class Strand >
bool stream_pipe_tmpl<SrcStream,DstStream,Strand>::debug;



template< class SrcStream,  
        class DstStream,
        class Strand = boost::asio::io_service::strand >
std::shared_ptr< stream_pipe_tmpl<SrcStream,DstStream,Strand> > make_stream_pipe( SrcStream& s, DstStream& d, size_t buffer_size ) {
    typedef stream_pipe_tmpl<SrcStream,DstStream,Strand> pipe_t;
    return std::make_shared<pipe_t>( s, d, buffer_size) ;
} 
}


#undef cerr



#endif // stream_pipe_9384759387593845793845793485739485739485739


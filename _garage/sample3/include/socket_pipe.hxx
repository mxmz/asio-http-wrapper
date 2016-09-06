#ifndef socket_pipe_9384759387593845793845793485739485739485739
#define socket_pipe_9384759387593845793845793485739485739485739

#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <memory>

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

template<class SrcSocket, class DstSocket = SrcSocket,
		class Strand = asio::io_service::strand>
class socket_pipe_tmpl: public std::enable_shared_from_this<
		socket_pipe_tmpl<SrcSocket, DstSocket, Strand>> {

public:
	socket_pipe_tmpl(SrcSocket&& src, DstSocket&& dst) :
			src_socket_(std::move(src)), reading_(false), dst_socket_(
					std::move(dst)), writing_(false), sbuf_(1024 * 10024 * 3), strand_(
					src.get_io_service()), finished_(false) {
		assert(&src.get_io_service() == &dst.get_io_service());

	}

	typedef std::function<
			void(const system::error_code&, SrcSocket&&,
					const system::error_code& write_ec_, SrcSocket&&)> completion_handler;

	void run(completion_handler hndl) {

		completion_ = std::move(hndl);
		activate();
	}
private:
	void finish() {
		cerr << "finished " << debug_count_read << " " << debug_count_write
				<< endl;
		if (not finished_) {
			finished_ = true;
			completion_(read_ec_, std::move(src_socket_), write_ec_,
					std::move(dst_socket_));
		}

	}
	void activate() {
		cerr << "full: " << sbuf_.full() << ", empty: " << sbuf_.empty()
				<< endl;
		cerr << "reading: " << reading_ << " writing: " << writing_ << endl;
		cerr << "read_ec_: " << read_ec_ << " write_ec_; " << write_ec_ << endl;

		if (not sbuf_.full() and not reading_ and not finished_
				and not read_ec_) {
			auto _this = this->shared_from_this();
			auto handler =
					strand_.wrap(
							[this,_this ](const system::error_code& ec, std::size_t bytes_transferred)
							{
								reading_ = false;
								if ( not ec )
								{
									auto bufs = sbuf_.prepare();
									auto rv = src_socket_.receive(bufs, 0, read_ec_ );
									cerr << "**** " << buffer_size(bufs) << " " << read_ec_ << endl;
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
			src_socket_.async_read_some(asio::null_buffers(), handler);
			reading_ = true;
		}

		if (sbuf_.empty() and read_ec_ and not finished_) {
			finish();
		} else if (not writing_ and not sbuf_.empty() and not finished_) {
			auto _this = this->shared_from_this();
			auto handler =
					strand_.wrap(
							[this,_this ](const boost::system::error_code& ec, std::size_t bytes_transferred)
							{
								writing_ = false;
								if ( not ec )
								{
									auto bufs = sbuf_.data();
									assert( buffer_size(bufs) > 0 );
									auto rv = dst_socket_.send( bufs,0, write_ec_ );
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
			dst_socket_.async_write_some(asio::null_buffers(), handler);
			writing_ = true;
		}

	}

private:
	SrcSocket src_socket_;
	system::error_code read_ec_;bool reading_;

	DstSocket dst_socket_;
	system::error_code write_ec_;bool writing_;

	ring_buffer sbuf_;
	Strand strand_;
	completion_handler completion_;bool finished_;
};

#undef cerr

/*
 template< class SrcSocket,  class DstSocket = SrcSocket, class Strand = asio::io_service::strand >
 class socket_pipe_tmpl: public std::enable_shared_from_this<socket_pipe_tmpl<SrcSocket,DstSocket,Strand>>
 {

 public:
 socket_pipe_tmpl( SrcSocket&& src, DstSocket&& dst )
 :   src_socket_(std::move(src)),
 reading_(false),
 dst_socket_(std::move(dst)),
 writing_(false),
 strand_(src.get_io_service() ),
 finished_(false)
 {
 assert( &src.get_io_service() == &dst.get_io_service() );

 }

 typedef std::function< void(const system::error_code& read_ec_, const system::error_code& write_ec_ )> completion_handler;

 void run( completion_handler hndl )
 {
 completion_ = std::move(hndl);
 activate(system::error_code(), system::error_code());
 }

 private:
 void finish(const system::error_code& read_ec_, const system::error_code& write_ec_ )
 {
 if ( not finished_ )
 {
 finished_ = true;
 completion_( read_ec_, write_ec_);
 }

 }
 void activate(const system::error_code& read_ec_, const system::error_code& write_ec_ )
 {
 if ( sbuf_.size() >  1024*1024* 50 )
 {
 cerr << "stop reading" << endl;
 }
 else if ( not reading_ and not finished_ and not read_ec_ )
 {
 auto _this = this->shared_from_this();
 auto handler = strand_.wrap([this,_this, write_ec_ ](const system::error_code& ec, std::size_t bytes_transferred)
 {
 reading_ = false;
 if ( not ec )
 {
 auto bufs = sbuf_.prepare(1024*1024);
 system::error_code ec;
 auto rv = src_socket_.receive(bufs, 0, ec);
 if ( not ec)
 {
 cerr << "read " << rv << " is writing: " << writing_ << endl;
 sbuf_.commit(rv);
 activate( system::error_code(), write_ec_ );
 }
 else
 {
 activate( ec, write_ec_ );
 }

 }
 else
 {
 activate( ec, write_ec_ );
 }
 });
 src_socket_.async_read_some( asio::null_buffers(), handler );
 reading_ = true;
 }

 if ( not sbuf_.size() and read_ec_ )
 {
 finish( read_ec_, write_ec_ );
 }
 else if ( not writing_ and sbuf_.size() and not finished_)
 {
 auto _this = this->shared_from_this();
 auto handler = strand_.wrap([this,_this, read_ec_](const boost::system::error_code& ec, std::size_t bytes_transferred)
 {
 writing_ = false;
 if ( not ec )
 {
 auto bufs = sbuf_.data();
 system::error_code ec;
 auto rv = dst_socket_.send( bufs,0, ec );
 if ( not ec )
 {
 cerr << "write " << rv << " is reading: " << reading_ << endl;
 sbuf_.consume(rv);
 activate( read_ec_, system::error_code() );
 }
 else
 {
 finish( read_ec_, ec );
 }

 }
 else
 {
 finish( read_ec_, ec );
 }
 });
 dst_socket_.async_write_some( asio::null_buffers(), handler );
 writing_ = true;
 }




 }



 private:
 SrcSocket src_socket_;
 bool      reading_;
 DstSocket dst_socket_;
 bool      writing_;
 asio::streambuf  sbuf_;
 Strand strand_;
 completion_handler completion_;
 bool                finished_;
 };














 */
#endif // socket_pipe_9384759387593845793845793485739485739485739


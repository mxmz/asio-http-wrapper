#ifndef http_server_394853985739857398623572653726537625495679458679458679486749
#define http_server_394853985739857398623572653726537625495679458679458679486749

#include <memory>
#include <map>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace mxmz {

using std::string;
using std::map;
using std::move;
using std::shared_ptr;
using std::unique_ptr;
using namespace boost::asio;




template < class ConnectionT >
class http_server_tmpl : public std::enable_shared_from_this<http_server_tmpl<ConnectionT>> {

    typedef typename ConnectionT::socket_t      socket_t;
    typedef typename socket_t::endpoint_type    endpoint_t; 
    typedef basic_socket_acceptor< typename socket_t::protocol_type> acceptor_t;
    
    socket_t    socket_;
    acceptor_t  acceptor_;
            
    void set_options() {}
    template< class Option1, class ...OptionT >
    void set_options(Option1 o1, OptionT ...opts ) {
       acceptor_.set_option(o1);
       set_options(opts...);
    }

    public:
    template< class ...OptionT>
    http_server_tmpl( io_service& ios, const endpoint_t& endpoint, OptionT ...opts  ) : 
            socket_(ios),
            acceptor_(ios)
            {
                acceptor_.open(endpoint.protocol());
                acceptor_.bind(endpoint);
                acceptor_.listen(5000);
                set_options(opts...);
            }

   template < class Handler >            
   void start_listen(Handler handler) {
        auto self( this->shared_from_this() );
        acceptor_.async_accept(socket, [self,this](boost::system::error_code ec) {
            if ( ec ) {
                    CERR << "async_accept " << ec << " " << ec.message() << endl;
                    
            }
        

   }         
};




















}












#endif
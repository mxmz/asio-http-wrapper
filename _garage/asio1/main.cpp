#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

using namespace boost::asio;


template <class Message, class Derived >
class ProducerBase:  public std::enable_shared_from_this<Derived>  {
        public:
        typedef Message message_t;
        typedef std::function<void()>  continue_t;
        typedef std::function< void(message_t, const continue_t& ) > callback_t;

        private:
        io_service& ios_;
        callback_t  cb_;


        public:
        ProducerBase( io_service& ios, callback_t cb ) : ios_(ios), cb_(cb) { }

        void emit(message_t m, const boost::system::error_code& ec) {
                cerr << __func__ << " " << m << " " << ec << endl;
                auto derived_this = static_cast<Derived*>(this);
                cb_( m, bind( &Derived::next, derived_this->shared_from_this() ) );
        }

};


class Producer : public ProducerBase<int,Producer> {
       public:

        boost::asio::steady_timer timer_;
        typedef  ProducerBase<int,Producer> base;
    public:
        Producer(io_service& ios, callback_t cb) : base(ios,cb), timer_(ios) {



        }

        void run () {
                cerr << __func__  << endl;
                auto _this = shared_from_this();
                timer_.expires_from_now( std::chrono::seconds(0) );
                timer_.async_wait( [this,_this]( const boost::system::error_code& ec ) {
                                    emit(42,ec);
                                } );
        }


        void next() {
            cerr << __func__ << endl;
            run();

        }
        ~Producer() {
            cerr << __func__ << endl;
        }

};

class Consumer : public enable_shared_from_this<Consumer> {
   boost::asio::steady_timer timer_;
   int counter_;
   int rounds_;
    public:
        Consumer(io_service& ios) : timer_(ios), counter_(0), rounds_(0) {
        }

        void run () {
            auto _this = shared_from_this();
            auto callback = [this,_this]( int v, const Producer::continue_t& next) {
                                    cerr << "callabck" << endl;
                                    ++counter_;
                                    if ( counter_ == 3 ) {
                                            ++ rounds_;
                                            if ( rounds_ == 4 ) {
                                                    return;
                                            }
                                            counter_ =0;
                                            timer_.expires_from_now( chrono::seconds(5) );
                                            timer_.async_wait( [this,_this, next](const boost::system::error_code &) {
                                                                next();
                                                            } );
                                    } else {
                                        next();
                                    }
                            } ;
            auto p = std::make_shared<Producer>(timer_.get_io_service(), callback);
            p->run();
        }
        ~Consumer() {
            cerr << __func__ << endl;

        }


};


/*
int main() {

    io_service ios;
    auto p = std::make_shared<Consumer>(ios);
    p->run();
    ios.run();
    std::string a;



}
*/



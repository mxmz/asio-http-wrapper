#ifndef ringbuffer209384029384209348028172368173618736187363498305983045
#define ringbuffer209384029384209348028172368173618736187363498305983045
#include <iostream>
#include <boost/asio/buffer.hpp>
namespace mxmz {

using boost::asio::mutable_buffers_1;
using boost::asio::const_buffers_1;
using boost::asio::buffer_cast;
using std::cerr;
using std::endl;

class ring_buffer
{
    public:

        ring_buffer(size_t len) :
            beg_(new char[len + 2 ] ),
            end_(beg_+ len + 2 ),
            a_(beg_),
            b_(beg_)
        {

        }
        ~ring_buffer() {
            delete[] beg_;
        }

        ring_buffer( ring_buffer&& that ) :
            beg_(new char[2] ),
            end_(beg_ + 2 ),
            a_(beg_),
            b_(beg_)
        {   swap(that);
        }

        void swap( ring_buffer& that ) {
            std::swap( const_cast<char*&>(beg_), const_cast<char*&>(that.beg_) );
            std::swap( const_cast<char*&>(end_), const_cast<char*&>(that.end_) );
            std::swap( a_, that.a_ );
            std::swap( b_, that.b_ );
        }

        ring_buffer( const ring_buffer& ) = delete ;

        ring_buffer() = delete;

        bool empty() const
        {
            return a_ == b_;
        }



        bool full() const
        {
            return buffer_size(prepare()) == 0;
        }

        size_t writable() const
        {
            return buffer_size(prepare());
        }

        size_t readable() const
        {
            return buffer_size(data());
        }

        /* 1-----------------------------------------! 
                ^               ^
                a               b
                -------data----- ------ buffer --------

                ^               ^
                b               a
                ---- buffer---|  ------ data -----------


        */        

        mutable_buffers_1 prepare() const
        {

           //cerr << "prepare: a_ " <<  (a_ - beg_) << endl;
           //cerr <<  "prepare: b_ " <<  ( b_ - beg_ )  << endl;

                if ( b_ < a_ ) {
                       return mutable_buffers_1( b_, a_ - b_ - 1 );
                }  else {
                    return a_ == beg_ ? 
                            mutable_buffers_1( b_, end_ - b_ - 1 ):
                            mutable_buffers_1( b_, end_ - b_  );
                }
        }


        const_buffers_1 data() const
        {
            //cerr << "data: a_ " <<  (a_ - beg_) << endl;
            //cerr <<  "data: b_ " <<  ( b_ - beg_ )  << endl;

            if ( a_ <= b_ ) {
                return const_buffers_1( a_, b_ - a_);
            } else {
                return const_buffers_1( a_, end_ - a_);
            }
            
        }

        void commit( size_t len )
        {
            b_ += len;
            if ( b_ == end_) b_ = beg_;
           
        }

        void consume(size_t len)
        {
            a_ += len;
            if ( a_ == end_ ) a_ = beg_;
            
        }





    private:

        char* const  beg_;
        char* const  end_;
        char*        a_;
        char*        b_;
};


}







#endif // ringbuffer209384029384209348028172368173618736187363498305983045


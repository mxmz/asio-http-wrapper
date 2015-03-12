#ifndef ringbuffer209384029384209348028172368173618736187363498305983045
#define ringbuffer209384029384209348028172368173618736187363498305983045

#include <boost/asio/buffer.hpp>

using asio::mutable_buffers_1;
using asio::const_buffers_1;
using asio::buffer_cast;

class ring_buffer {
public:

	ring_buffer(size_t len) :
			beg_(new char[len]), end_(beg_ + len), head_(beg_), tail_(beg_) {

	}
	~ring_buffer() {
		delete beg_;
	}

	ring_buffer() = delete;

	bool empty() const {
		return head_ == tail_;
	}

	mutable_buffers_1 prepare() const {
//        cerr << "tail_== end_ " << (tail_ == end_ ) << endl;

		auto tail = tail_ == end_ ? beg_ : tail_;
		auto head = head_;

		if (tail >= head) {
			return mutable_buffers_1(tail, end_ - tail);
		} else {
			return head - tail > 1 ?
					mutable_buffers_1(tail, head - tail - 1) :
					mutable_buffers_1(tail, 0);
		}
	}

	bool full() const {
		return buffer_size(prepare()) == 0;
	}

	const_buffers_1 data() const {
		//cerr <<  (void*) beg_ << " " << (void*) end_ << endl;
		//cerr <<  (void*) head_ << " " << (void*)tail_ << endl;
		auto tail = tail_;
		auto head = head_ == end_ ? beg_ : head_;

		if (tail >= head) {
			return const_buffers_1(head, tail - head);
		} else {
			return const_buffers_1(head, end_ - head);
		}
	}

	void commit(size_t len) {
		if (tail_ == end_)
			tail_ = beg_ + len;
		else
			tail_ += len;
	}

	void consume(size_t len) {
		if (head_ == end_)
			head_ = beg_ + len;
		else
			head_ += len;
	}

private:

	char* const beg_;
	char* const end_;
	char* head_;
	char* tail_;
};

#endif // ringbuffer209384029384209348028172368173618736187363498305983045


#ifndef pimpl_h_39845039850398503948530945830495830495830495830495832763547236
#define pimpl_h_39845039850398503948530945830495830495830495830495832763547236
#include "pimpl.h"
#include <memory>
namespace mxmz {

template< class T, int S, int A >
class pimpl {
    typename std::aligned_storage<S,A>::type mem[1];

    public:

    template<typename ...Args> 
     pimpl(Args&&... args  );
     
    pimpl();
    ~pimpl();
    
    pimpl(const pimpl&) = delete;


    T&    operator *() ;
    T*    operator ->() ;
    const T&    operator *() const ;
    const T*    operator ->() const ;

};


}
#endif
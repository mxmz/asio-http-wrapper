#ifndef pimpl_hxx_39845039850398503948530945830495830495830495830495832763547236
#define pimpl_hxx_39845039850398503948530945830495830495830495830495832763547236
#include "util/pimpl.h"

namespace mxmz {

template< class T, int Size, int Align >
inline void static_check_size()
{
  // 10% tolerance margin
  static_assert( (sizeof(T) <= Size) , "impl<> Size must be bigger");
  static_assert( (Size <= sizeof(T) * 1.1), "impl<> Size must be smaller");
  static_assert(Align == alignof(T), "impl<> Align must be changed");
  //CERR << sizeof(T) << endl; abort();
}


template< class T, int Size, int Align >
template<typename ...Args> 
pimpl<T,Size,Align>::pimpl(Args&&... args  )
{
  static_check_size<T,Size,Align>();
  new(mem)T(std::forward<Args>(args)...);
}
template< class T, int Size, int Align >
pimpl<T,Size,Align>::pimpl()
{
  static_check_size<T,Size,Align>();
  new(mem)T();
}

template< class T, int Size, int Align >
pimpl<T,Size,Align>::~pimpl() {
        reinterpret_cast<const T*>(mem)->~T();
}

template< class T, int S, int A >
T& 
pimpl<T,S,A>::operator *() {
	return reinterpret_cast<T&>(mem);
}

template< class T, int S, int A >
T*  
pimpl<T,S,A>::operator ->() {
	return reinterpret_cast<T *>(mem);
}


template< class T, int S, int A >
const T& 
pimpl<T,S,A>::operator *() const {
	return reinterpret_cast<const T&>(mem);
}

template< class T, int S, int A >
const T*  
pimpl<T,S,A>::operator ->() const {
	return reinterpret_cast<const T*>(mem);
}





}


#endif
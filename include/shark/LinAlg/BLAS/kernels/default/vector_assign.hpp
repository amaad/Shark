/*!
 * \brief       Assignment kernels for vector expressions
 * 
 * \author      O. Krause
 * \date        2015
 *
 *
 * \par Copyright 1995-2015 Shark Development Team
 * 
 * <BR><HR>
 * This file is part of Shark.
 * <http://image.diku.dk/shark/>
 * 
 * Shark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Shark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Shark.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef REMORA_KERNELS_DEFAULT_VECTOR_ASSIGN_HPP
#define REMORA_KERNELS_DEFAULT_VECTOR_ASSIGN_HPP

#include "../../expression_types.hpp"

namespace remora{namespace bindings{

template<class F, class V>
void assign(vector_expression<V, cpu_tag>& v, typename V::value_type t) {
	F f;
	typedef typename V::iterator iterator;
	iterator end = v().end();
	for (iterator it = v().begin(); it != end; ++it){
		*it = f(*it, t);
	}
}

/////////////////////////////////////////////////////////
//direct assignment of two vectors
////////////////////////////////////////////////////////

// Dense-Dense case
template< class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v, vector_expression<E, cpu_tag> const& e, 
	dense_tag, dense_tag
) {
	for(std::size_t i = 0; i != v().size(); ++i){
		v()(i) = static_cast<typename V::value_type>(e()(i));
	}
}
// Dense-packed case
template< class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v, vector_expression<E, cpu_tag> const& e, 
	dense_tag, packed_tag
) {
	typedef typename E::const_iterator EIterator;
	typedef typename V::value_type value_type;
	EIterator eiter = e.begin();
	EIterator eend = e.end();
	//special case:
	//right hand side is completely 0
	if(eiter == eend){
		v().clear();
		return;
	}
	EIterator viter = v.begin();
	EIterator vend = v.end();
	
	//set the first elements to zero
	for(;viter.index() != eiter.index(); ++viter){
		*viter= value_type/*zero*/();
	}
	//copy contents of right-hand side
	for(;eiter != eend; ++eiter,++viter){
		*viter= *eiter;
	}
	
	for(;viter!= vend; ++viter){
		*viter= value_type/*zero*/();
	}
}

// packed-packed case
template< class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v, vector_expression<E, cpu_tag> const& e, 
	packed_tag, packed_tag
) {
	typedef typename E::const_iterator EIterator;
	EIterator eiter = e.begin();
	EIterator eend = e.end();
	//special case:
	//right hand side is completely 0
	if(eiter == eend){
		v().clear();
		return;
	}
	EIterator viter = v.begin();
	EIterator vend = v.end();
	
	//check for compatible layout
	REMORA_SIZE_CHECK(vend-viter);//empty ranges can't be compatible
	//check whether the right hand side range is included in the left hand side range
	REMORA_SIZE_CHECK(viter.index() <= eiter.index());
	REMORA_SIZE_CHECK(viter.index()+(vend-viter) >= eiter.index()+(eend-eiter));
	
	//copy contents of right-hand side
	viter += eiter.index()-viter.index();
	for(;eiter != eend; ++eiter,++viter){
		*viter= *eiter;
	}
}

//Dense-Sparse case
template<class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e, 
	dense_tag, 
	sparse_tag
) {
	v().clear();
	typedef typename E::const_iterator iterator;
	iterator end = e().end();
	for(iterator it = e().begin(); it != end; ++it){
		v()(it.index()) = *it;
	}
}
//Sparse-Dense
template<class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	sparse_tag,
	dense_tag
) {
	v().clear();
	v().reserve(e().size());
	typename V::iterator targetPos = v().begin();
	REMORA_RANGE_CHECK(targetPos == v().end());//as v is cleared, pos must be equal to end
	for(std::size_t i = 0; i != e().size(); ++i,++targetPos){
		targetPos = v().set_element(targetPos,i,e()(i));
	}
}
// Sparse-Sparse case
template<class V, class E>
void vector_assign(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	sparse_tag,
	sparse_tag
) {
	v().clear();
	typedef typename E::const_iterator iteratorE;
	typename V::iterator targetPos = v().begin();
	REMORA_RANGE_CHECK(targetPos == v().end());//as v is cleared, pos must be equal to end
	iteratorE end = e().end();
	for(iteratorE it = e().begin(); it != end; ++it,++targetPos){
		targetPos = v().set_element(targetPos,it.index(),*it);
	}
}

////////////////////////////////////////////
//assignment with functor
////////////////////////////////////////////

//dense dense case
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	dense_tag, dense_tag
) {
	for(std::size_t i = 0; i != v().size(); ++i){
		v()(i) = f(v()(i),e()(i));
	}
}

//dense packed case
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	dense_tag, packed_tag
) {
	typedef typename E::const_iterator EIterator;
	typedef typename V::const_iterator VIterator;
	typedef typename V::value_type value_type;
	EIterator eiter = e().begin();
	EIterator eend = e().end();
	VIterator viter = v().begin();
	VIterator vend = v().end();
	//right hand side hasnonzero elements
	if(eiter != eend){
		//apply f to the first elements for which the right hand side is 0, unless f is the identity
		for(;viter.index() != eiter.index() &&!F::right_zero_identity; ++viter){
			*viter = f(*viter,value_type/*zero*/());
		}
		//copy contents of right-hand side
		for(;eiter != eend; ++eiter,++viter){
			*viter = f(*viter,*eiter);
		}
	}
	//apply f to the last elements for which the right hand side is 0, unless f is the identity
	for(;viter!= vend &&!F::right_zero_identity; ++viter){
		*viter= value_type/*zero*/();
	}
}

//packed-packed case
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	packed_tag, packed_tag
) {
	typedef typename E::const_iterator EIterator;
	typedef typename V::const_iterator VIterator;
	typedef typename V::value_type value_type;
	EIterator eiter = e().begin();
	EIterator eend = e().end();
	VIterator viter = v().begin();
	VIterator vend = v().end();
	
	//right hand side has nonzero elements
	if(eiter != eend){
		
		//check for compatible layout
		REMORA_SIZE_CHECK(vend-viter);//empty ranges can't be compatible
		//check whether the right hand side range is included in the left hand side range
		REMORA_SIZE_CHECK(viter.index() <= eiter.index());
		REMORA_SIZE_CHECK(viter.index()+(vend-viter) >= eiter.index()+(eend-eiter));
		
		//apply f to the first elements for which the right hand side is 0, unless f is the identity
		for(;viter.index() != eiter.index() &&!F::right_zero_identity; ++viter){
			*viter = f(*viter,value_type/*zero*/());
		}
		//copy contents of right-hand side
		for(;eiter != eend; ++eiter,++viter){
			*viter = f(*viter,*eiter);
		}
	}
	//apply f to the last elements for which the right hand side is 0, unless f is the identity
	for(;viter!= vend &&!F::right_zero_identity; ++viter){
		*viter= f(*viter,value_type/*zero*/());
	}
}

//Dense-Sparse case
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	dense_tag, sparse_tag
) {
	typedef typename E::const_iterator iterator;
	iterator end = e().end();
	for(iterator it = e().begin(); it != end; ++it){
		v()(it.index()) = f(v()(it.index()),*it);
	}
}

//sparse-dense case
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	sparse_tag tag, dense_tag
){	
	typedef typename V::value_type value_type;
	typedef typename V::size_type size_type;
	value_type zero = value_type();
	size_type size = e().size();
	
	typename V::iterator it = v().begin();
	for(size_type i = 0; i != size; ++i,++it){
		if(it == v().end() || it.index() != i){//insert missing elements
			it = v().set_element(it,i,zero); 
		}
		*it = f(*it, e()(i));
	}
}

// Sparse-Sparse case has three implementations.
//the stupidity of this case is, that we have to assume in the general case v and e share the same 
//array memory and thus changing v might invalidate the iterators of e. 
//This is not the same as aliasing of v and e, as v might be for example one matrix row and e another
//of the same matrix.
//thus we look at the cases where (at least) one of the arguments is a vector-container, which means
//that we are not facing the problem of same memory as this would otherwise mean that we are aliasing
//in which case the expression is not defined anyways. 

//called for independent argumeents v and e
template<class V, class E, class F>
void assign_sparse(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f
){	
	typedef typename V::value_type value_type;
	typedef typename V::size_type size_type;
	value_type zero = value_type();

	typename V::iterator it = v().begin();
	typename E::const_iterator ite = e().begin();
	typename E::const_iterator ite_end = e().end();
	while(it != v().end() && ite != ite_end) {
		size_type it_index = it.index();
		size_type ite_index = ite.index();
		if (it_index == ite_index) {
			*it = f(*it, *ite);
			++ ite;
		} else if (it_index < ite_index) {
			*it = f(*it, zero);
		} else{//it_index > ite_index so insert new element in v()
			it = v().set_element(it,ite_index,zero); 
			*it = f(*it, *ite);
			++ite;
		}
		++it;
	}
	//apply zero transformation on elements which are not transformed yet
	for(;it != v().end();++it) {
		*it = f(*it, zero);
	}
	//add missing elements
	for(;ite != ite_end;++it,++ite) {
		it = v().set_element(it,ite.index(),zero); 
		*it = f(*it, *ite);
	}
}
//as long as one argument is not a proxy, we are in the good case.
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_container<E, cpu_tag> const& e,
	F f,
	sparse_tag tag, sparse_tag
){	
	assign_sparse(v,e);
}
template<class V, class E, class F>
void vector_assign_functor(
	vector_container<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	sparse_tag tag, sparse_tag
){	
	assign_sparse(v,e,f);
}
template<class V, class E, class F>
void vector_assign_functor(
	vector_container<V, cpu_tag>& v,
	vector_container<E, cpu_tag> const& e,
	F f,
	sparse_tag tag, sparse_tag
){	
	assign_sparse(v,e,f);
}


//In the general case we have to take one bullet: 
//either use a temporary, which has copying time and allocation time
//or count the non-zero elements of e which might be expensive as well. we decide
//to take the first route for now.
template<class V, class E, class F>
void vector_assign_functor(
	vector_expression<V, cpu_tag>& v,
	vector_expression<E, cpu_tag> const& e,
	F f,
	sparse_tag tag, sparse_tag
){	
	typename vector_temporary<V>::type temporary(v());
	assign_sparse(temporary,e, f);
	v().clear();
	bindings::vector_assign(v, temporary, tag, tag);
}

}}
#endif

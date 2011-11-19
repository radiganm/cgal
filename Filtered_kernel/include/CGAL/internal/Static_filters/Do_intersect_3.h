// Copyright (c) 2008 ETH Zurich (Switzerland)
// Copyright (c) 2008-2009 INRIA Sophia-Antipolis (France)
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Andreas Fabri


#ifndef CGAL_INTERNAL_STATIC_FILTERS_DO_INTERSECT_3_H
#define CGAL_INTERNAL_STATIC_FILTERS_DO_INTERSECT_3_H

#include <CGAL/Bbox_3.h>
#include <CGAL/Profile_counter.h>
#include <CGAL/internal/Static_filters/Static_filter_error.h>
#include <CGAL/internal/Static_filters/tools.h>
#include <cmath>
#include <iostream>

// inspired from http://cag.csail.mit.edu/~amy/papers/box-jgt.pdf

namespace CGAL {

namespace internal {

namespace Static_filters_predicates {


template < typename K_base >
class Do_intersect_3
  : public K_base::Do_intersect_3
{
  typedef typename K_base::Point_3   Point_3;
  typedef typename K_base::Segment_3 Segment_3;
  typedef typename K_base::Do_intersect_3 Base;

public:


#ifndef CGAL_CFG_MATCHING_BUG_6
  using Base::operator();
#else 

  // TODO: add them all!!

#endif


  typedef typename Base::result_type  result_type;

  Sign sign_with_error(const double x, const double error) const {
    if(x > error) return POSITIVE;
    else if( x < - error) return NEGATIVE;
    else return ZERO;
  }

  result_type 
  operator()(const Segment_3 &s, const Bbox_3& b) const
  {
    CGAL_BRANCH_PROFILER_3("semi-static failures/attempts/calls to   : Do_intersect_3", tmp);

    Get_approx<Point_3> get_approx; // Identity functor for all points
    // but lazy points.
    const Point_3& p = s.source(); 
    const Point_3& q = s.target(); 

    double px, py, pz, qx, qy, qz;
    double bxmin = b.xmin(), bymin = b.ymin(), bzmin = b.zmin(), 
      bxmax = b.xmax(), bymax = b.ymax(), bzmax = b.zmax();

    if (fit_in_double(get_approx(p).x(), px) && fit_in_double(get_approx(p).y(), py) &&
        fit_in_double(get_approx(p).z(), pz) &&
        fit_in_double(get_approx(q).x(), qx) && fit_in_double(get_approx(q).y(), qy) &&
        fit_in_double(get_approx(q).z(), qz) )   
    {
      // std::cerr << "\n"
      //           << px << " " <<  py << " " <<  pz << "\n" 
      //           << qx << " " <<  qy << " " <<  qz << "\n" 
      //           << bxmin << " " <<  bymin << " " <<  bzmin << "\n" 
      //           << bxmax << " " <<  bymax << " " <<  bzmax << "\n";
      CGAL_BRANCH_PROFILER_BRANCH_1(tmp);

     
      // AF: I copy pasted the code to call the max

      // -----------------------------------
      // treat x coord
      // -----------------------------------
      double dmin, dmax, tmin, tmax;
      if ( qx >= px )  // this is input and needs no epsilon
      {
        if(px > bxmax) return false; // segment on the right of bbox
        if(qx < bxmin) return false; // segment on the left of bbox

        tmax = bxmax - px;
        
        dmax = qx - px;
        if ( bxmin < px ) // tmin < 0 means px is in the x-range of bbox
        {
          tmin = 0;
          dmin = 1;
        } else {
          tmin = bxmin - px;
          dmin = dmax;
        }
      }
      else
      {
        if(qx > bxmax) return false; // segment on the right of bbox
        if(px < bxmin) return false; // segment on the left of bbox

        tmax = px - bxmin;

        dmax = px - qx;
        if ( px < bxmax ) // tmin < 0 means px is in the x-range of bbox
        {
          tmin = 0;
          dmin = 1;
        } else {
          tmin = px - bxmax;
          dmin = dmax;
        }
      }   

      // std::cerr << "t1 ";
   
      double m = CGAL::abs(tmin), m2;
      m2 = CGAL::abs(tmax); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(dmin); if(m2 > m) { m = m2; }

      if(m < 7e-294) {
        // underflow in the computation of 'error'
        return Base::operator()(s,b);
      }

      const double EPS_1 = 3.55618e-15;

      double error =  EPS_1 * m;

      switch(sign_with_error( tmax - dmax, error)) {
      case POSITIVE:
        // std::cerr << "t2 ";
        tmax = 1;
        dmax = 1;
        break;
      case NEGATIVE:
        break;
      default:
        // ambiguity of the comparison tmax > dmin
        // let's call the exact predicate
        // std::cerr << "\ntest2 NEED EXACT\n";
        return Base::operator()(s,b);
      }

      // -----------------------------------
      // treat y coord
      // -----------------------------------
      double d_, tmin_, tmax_;
      if ( qy >= py )   // this is input and needs no epsilon
      {
        tmin_ = bymin - py;
        tmax_ = bymax - py;
        d_ = qy - py;
      }
      else
      {
        tmin_ = py - bymax;
        tmax_ = py - bymin;
        d_ = py - qy;
      }
    
      m2 = CGAL::abs(tmin_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(tmax_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(d_); if(m2 > m) { m = m2; }

      if(m < 3e-147) {
        // underflow in the computation of 'error'
        return Base::operator()(s,b);
      }
      
      error =  EPS_1 * m * m;

      // std::cerr << dmin << " " << tmax_ << " " << d_ << " "
      //           << tmin << " " << dmax << " " << tmin_ << std::endl;

      if(m > 1e153) { /* sqrt(max_double [hadamard]/2) */ 
        // potential overflow on the computation of 'sign1' and 'sign2'
        return Base::operator()(s,b);
      }
      Sign sign1 = sign_with_error( (d_*tmin) - (dmin*tmax_) , error);
      Sign sign2 = sign_with_error( (dmax*tmin_) - (d_*tmax) , error);

      if(sign1 == POSITIVE || sign2 == POSITIVE) 
        return false; // We are *sure* the segment is outside the box, on one
                      // side or the other.
      if(sign1 == ZERO || sign2 == ZERO) {
        // std::cerr << "\ntest3 NEED EXACT\n";
        return Base::operator()(s,b); // We are *unsure*: one *may be*
                                      // positive.
      }

      // std::cerr << "t3 ";

      // Here we are sure the two signs are negative. We can continue with
      // the rest of the function...

      // epsilons needed
      switch(sign_with_error((dmin*tmin_) - (d_*tmin) , error)) {
      case POSITIVE:
        tmin = tmin_;
        dmin = d_;
        // std::cerr << "t4 ";
        break;
      case NEGATIVE:
        break;
      default: // uncertainty
        // std::cerr << "\ntest4 NEED EXACT\n";
        return Base::operator()(s,b);
      }
    
      // epsilons needed
      switch(sign_with_error((d_*tmax) - (dmax*tmax_) , error)) {
      case POSITIVE:
        tmax = tmax_;
        dmax = d_;
        // std::cerr << "t5 ";break;
      case NEGATIVE:
        break;
      default: // uncertainty
        // std::cerr << "\ntest5 NEED EXACT\n";
        return Base::operator()(s,b);
      }
    
      // -----------------------------------
      // treat z coord
      // -----------------------------------
      if ( qz >= pz )   // this is input and needs no epsilon
      {
        tmin_ = bzmin - pz;
        tmax_ = bzmax - pz;
        d_ = qz - pz;
      }
      else
      {
        tmin_ = pz - bzmax;
        tmax_ = pz - bzmin;
        d_ = pz - qz;
      }
    
      m2 = CGAL::abs(tmin_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(tmax_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(d_); if(m2 > m) { m = m2; }

      // m may have changed
      error =  EPS_1 * m * m;
    
      if(m > 1e153) { /* sqrt(max_double [hadamard]/2) */ 
        // potential overflow on the computation of 'sign1' and 'sign2'
        return Base::operator()(s,b);
      }
      sign1 = sign_with_error( (dmin*tmax_) - (d_*tmin) , error);
      sign2 = sign_with_error( (d_*tmax) - (dmax*tmin_) , error);
      if(sign1 == NEGATIVE || sign2 == NEGATIVE) {
        // std::cerr << "f6";
        return false; // We are *sure* the segment is outside the box, on one
                      // side or the other.
      }
      if(sign1 == ZERO || sign2 == ZERO) {
        // std::cerr << "\test6 NEED EXACT\n";
        return Base::operator()(s,b); // We are *unsure*: one *may be*
                                      // negative.
      }
      // std::cerr << "t6";
      return true; // We are *sure* the two signs are positive.
    }
    return Base::operator()(s,b);
  }



  result_type 
  operator()(const Ray_3 &r, const Bbox_3& b) const
  {
    CGAL_BRANCH_PROFILER_3("semi-static failures/attempts/calls to   : Do_intersect_3", tmp);

    Get_approx<Point_3> get_approx; // Identity functor for all points
    // but lazy points.
    const Point_3& p = r.source(); 
    const Point_3& q = r.point(1); 

    double px, py, pz, qx, qy, qz;
    double bxmin = b.xmin(), bymin = b.ymin(), bzmin = b.zmin(), 
      bxmax = b.xmax(), bymax = b.ymax(), bzmax = b.zmax();

    if (fit_in_double(get_approx(p).x(), px) && fit_in_double(get_approx(p).y(), py) &&
        fit_in_double(get_approx(p).z(), pz) &&
        fit_in_double(get_approx(q).x(), qx) && fit_in_double(get_approx(q).y(), qy) &&
        fit_in_double(get_approx(q).z(), qz) )   
    {
      // std::cerr << "\n"
      //           << px << " " <<  py << " " <<  pz << "\n" 
      //           << qx << " " <<  qy << " " <<  qz << "\n" 
      //           << bxmin << " " <<  bymin << " " <<  bzmin << "\n" 
      //           << bxmax << " " <<  bymax << " " <<  bzmax << "\n";
      CGAL_BRANCH_PROFILER_BRANCH_1(tmp);

     
      // AF: I copy pasted the code to call the max

      // -----------------------------------
      // treat x coord
      // -----------------------------------
      double dmin, dmax, tmin, tmax;
      if ( qx >= px )  // this is input and needs no epsilon
      {
        // different from segment: if(px > bxmax) return false; // segment on the right of bbox
        //                         if(qx < bxmin) return false; // segment on the left of bbox
        if (bxmax < px)  return false; // different from segment: added
        tmax = bxmax - px;
        
        dmax = qx - px;
        if ( bxmin < px ) // tmin < 0 means px is in the x-range of bbox
        {
          tmin = 0;
          dmin = 1;
        } else {
          tmin = bxmin - px;
          dmin = dmax;
        }
      }
      else
      {
        // different from segment: if(qx > bxmax) return false; // segment on the right of bbox
        //                         if(px < bxmin) return false; // segment on the left of bbox
        if(px < bxmin) return false; // different from segment: added
        tmax = px - bxmin;

        dmax = px - qx;
        if ( px < bxmax ) // tmin < 0 means px is in the x-range of bbox
        {
          tmin = 0;
          dmin = 1;
        } else {
          tmin = px - bxmax;
          dmin = dmax;
        }
      }   

      // std::cerr << "t1 ";
   
      double m = CGAL::abs(tmin), m2;
      m2 = CGAL::abs(tmax); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(dmin); if(m2 > m) { m = m2; }

      if(m < 7e-294) {
        // underflow in the computation of 'error'
        return Base::operator()(r,b);
      }

      const double EPS_1 = 3.55618e-15;

      double error =  EPS_1 * m;

      switch(sign_with_error( tmax - dmax, error)) {
      case POSITIVE:
        // std::cerr << "t2 ";
        tmax = 1;
        dmax = 1;
        break;
      case NEGATIVE:
        break;
      default:
        // ambiguity of the comparison tmax > dmin
        // let's call the exact predicate
        // std::cerr << "\ntest2 NEED EXACT\n";
        return Base::operator()(r,b);
      }

      // -----------------------------------
      // treat y coord
      // -----------------------------------
      double d_, tmin_, tmax_;
      if ( qy >= py )   // this is input and needs no epsilon
      {
        tmin_ = bymin - py;
        tmax_ = bymax - py;
        d_ = qy - py;
      }
      else
      {
        tmin_ = py - bymax;
        tmax_ = py - bymin;
        d_ = py - qy;
      }
    
      m2 = CGAL::abs(tmin_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(tmax_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(d_); if(m2 > m) { m = m2; }

      if(m < 3e-147) {
        // underflow in the computation of 'error'
        return Base::operator()(r,b);
      }
      
      error =  EPS_1 * m * m;

      // std::cerr << dmin << " " << tmax_ << " " << d_ << " "
      //           << tmin << " " << dmax << " " << tmin_ << std::endl;

      if(m > 1e153) { /* sqrt(max_double [hadamard]/2) */ 
        // potential overflow on the computation of 'sign1' and 'sign2'
        return Base::operator()(r,b);
      }
      Sign sign1 = sign_with_error( (d_*tmin) - (dmin*tmax_) , error);
      Sign sign2 = sign_with_error( (dmax*tmin_) - (d_*tmax) , error);

      if(sign1 == POSITIVE || sign2 == POSITIVE) 
        return false; // We are *sure* the segment is outside the box, on one
                      // side or the other.
      if(sign1 == ZERO || sign2 == ZERO) {
        // std::cerr << "\ntest3 NEED EXACT\n";
        return Base::operator()(r,b); // We are *unsure*: one *may be*
                                      // positive.
      }

      // std::cerr << "t3 ";

      // Here we are sure the two signs are negative. We can continue with
      // the rest of the function...

      // epsilons needed
      switch(sign_with_error((dmin*tmin_) - (d_*tmin) , error)) {
      case POSITIVE:
        tmin = tmin_;
        dmin = d_;
        // std::cerr << "t4 ";
        break;
      case NEGATIVE:
        break;
      default: // uncertainty
        // std::cerr << "\ntest4 NEED EXACT\n";
        return Base::operator()(r,b);
      }
    
      // epsilons needed
      switch(sign_with_error((d_*tmax) - (dmax*tmax_) , error)) {
      case POSITIVE:
        tmax = tmax_;
        dmax = d_;
        // std::cerr << "t5 ";break;
      case NEGATIVE:
        break;
      default: // uncertainty
        // std::cerr << "\ntest5 NEED EXACT\n";
        return Base::operator()(r,b);
      }
    
      // -----------------------------------
      // treat z coord
      // -----------------------------------
      if ( qz >= pz )   // this is input and needs no epsilon
      {
        tmin_ = bzmin - pz;
        tmax_ = bzmax - pz;
        d_ = qz - pz;
      }
      else
      {
        tmin_ = pz - bzmax;
        tmax_ = pz - bzmin;
        d_ = pz - qz;
      }
    
      m2 = CGAL::abs(tmin_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(tmax_); if(m2 > m) { m = m2; }
      m2 = CGAL::abs(d_); if(m2 > m) { m = m2; }

      // m may have changed
      error =  EPS_1 * m * m;
    
      if(m > 1e153) { /* sqrt(max_double [hadamard]/2) */ 
        // potential overflow on the computation of 'sign1' and 'sign2'
        return Base::operator()(r,b);
      }
      sign1 = sign_with_error( (dmin*tmax_) - (d_*tmin) , error);
      sign2 = sign_with_error( (d_*tmax) - (dmax*tmin_) , error);
      if(sign1 == NEGATIVE || sign2 == NEGATIVE) {
        // std::cerr << "f6";
        return false; // We are *sure* the segment is outside the box, on one
                      // side or the other.
      }
      if(sign1 == ZERO || sign2 == ZERO) {
        // std::cerr << "\test6 NEED EXACT\n";
        return Base::operator()(r,b); // We are *unsure*: one *may be*
                                      // negative.
      }
      // std::cerr << "t6";
      return true; // We are *sure* the two signs are positive.
    }
    return Base::operator()(r,b);
  }



  // Computes the epsilon for Bbox_3_Segment_3_do_intersect.
  static double compute_epsilon_bbox_segment_3()
  {
    typedef Static_filter_error F;
    F t1 = F(1);

    // TODO: write the most complex arithmetic expression that happens
    //       in the operator above 

    F f = ((t1 - t1) * (t1 - t1)) - ((t1 - t1) * (t1 - t1));
    F f1 = (t1 - t1);
    F f1bis = (t1 - t1) - (t1 - t1);
    F f2 = f1*f1;
    F f3 = f2 - f2;
    std::cerr << "epsilons:\n"
              << "  degre " << f1.degree() << ": " <<  f1.error() << "\n"
              << "  degre " << f1bis.degree() << ": " <<  f1bis.error() << "\n"
              << "  degre " << f2.degree() << ": " <<  f2.error() << "\n"
              << "  degre " << f3.degree() << ": " <<  f3.error() << "\n";
      
    double err = f.error();
    err += err * 2 *  F::ulp(); // Correction due to "eps * m * m".  Do we need 2 ?
    std::cerr << "*** epsilon for Do_intersect_3(Bbox_3, Segment_3) = "
              << err << std::endl;
    std::cerr << "\n"
              << "Now for underflow/overflows...\n"
              << "        min_double/eps = " 
              << std::numeric_limits<double>::min() / err << std::endl
              << "  sqrt(min_double/eps) = "
              << CGAL::sqrt(std::numeric_limits<double>::min() / err) << std::endl;
    return err;
  }

}; // class Do_intersect_3

}  // namespace Static_filters_predicates

} // namespace internal


} //namespace CGAL

#endif  // CGAL_INTERNAL_STATIC_FILTERS_DO_INTERSECT_3_H

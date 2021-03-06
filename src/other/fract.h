/*
  -------------------------------------------------------------------
  
  Copyright (C) 2020-2021, Andrew W. Steiner
  
  This file is part of O2scl.
  
  O2scl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  O2scl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with O2scl. If not, see <http://www.gnu.org/licenses/>.

  -------------------------------------------------------------------
*/
/** \file frac.h
    \brief Desc
*/
#ifndef O2SCL_FRACT_H
#define O2SCL_FRACT_H

#include <iostream>
#include <string>

#include <o2scl/constants.h>
#include <o2scl/err_hnd.h>
#include <o2scl/table3d.h>

#ifdef O2SCL_OPENMP
#include <omp.h>
#endif

#ifndef DOXYGEN_NO_O2NS
namespace o2scl {
#endif

  /// Jacobian function (not necessarily square) in src/root/jacobian.h
  typedef std::function<
    int(size_t,const boost::numeric::ublas::vector<double> &,
	boost::numeric::ublas::vector<double> &,
	boost::numeric::ublas::matrix<double> &) > nrf_funct;

  /** \brief The function \f$ z^4-1 \f$ and its Jacobian
   */
  int nrf_z4m1(size_t nv,
               const boost::numeric::ublas::vector<double> &x,
               boost::numeric::ublas::vector<double> &f,
               boost::numeric::ublas::matrix<double> &J);
  
  typedef std::function<int(boost::numeric::ublas::vector<double> &,
                            boost::numeric::ublas::vector<double>) >
  itf_funct;
  
  /** \brief The function \f$ z \rightarrow z^2+c \f$ 
   */
  int itf_mandel(boost::numeric::ublas::vector<double> &z,
                 boost::numeric::ublas::vector<double> c);
  
  /** \brief Generate fractal images
   */
  class fract {
    
  public:

    /// Verbosity parameter
    int verbose;

    fract() {
      verbose=1;
    }
    
    typedef boost::numeric::ublas::vector<double> ubvector;
    typedef boost::numeric::ublas::matrix<double> ubmatrix;

    /** \brief Generate a Newton-Raphson fractal and store in 
        the \ref o2scl::table3d object \c t3d

	If the iteration converges to a root, then the <tt>"root"</tt>
	slice in the \ref o2scl::table3d object is set to the index of
	the root (beginning with 1 not 0) and the <tt>"it"</tt> slice
	in the \ref o2scl::table3d object is set to the iteration
	number at which convergence was achieved. If \f$
	x^2+y^2>r_{\mathrm{max}}^2 \f$ then the iteration is presumed
	to have diverged, then <tt>"root"</tt> is set to -1 and
	<tt>"it"</tt> is set to \c kmax. If, after \c kmax iterations,
	the iteration has not converged, then <tt>"root"</tt> is set
	to 0 and <tt>"it"</tt> is set to \c kmax.
     */
    template<class func_t=nrf_funct, class mat_t=ubmatrix,
      class vec_t=ubvector, class vec2_t=std::vector<double>,
      class vec2_size_t=std::vector<size_t>, class fp_t=double>
      int nrf(func_t &f, uniform_grid<fp_t> &gx,
	      uniform_grid<fp_t> &gy, size_t kmax, fp_t rmax,
	      o2scl::table3d &t3d, vec2_t &roots_x,
	      vec2_t &roots_y, vec2_size_t &min_count,
	      vec2_size_t &max_count) {
      
      t3d.clear();

      if (roots_x.size()!=roots_y.size()) {
	O2SCL_ERR("Root coordinate vectors do not have the same size.",
		  o2scl::exc_einval);
      }

      if (min_count.size()!=roots_x.size()) {
	min_count.resize(roots_x.size());
      } 
      if (max_count.size()!=roots_x.size()) {
	max_count.resize(roots_x.size());
      } 
      for(size_t i=0;i<roots_x.size();i++) {
	min_count[i]=kmax;
	max_count[i]=0;
      }

      t3d.set_xy("x",gx,"y",gy);
      t3d.new_slice("it");
      t3d.new_slice("root");
      t3d.set_slice_all("it",0.0);
      t3d.set_slice_all("root",0.0);

#ifdef O2SCL_OPENMP
#pragma omp parallel for
#endif
      for(size_t i=0;i<t3d.get_nx();i++) {

	vec_t x0(2), x1(2), fx(2);
	mat_t J(2,2);
	
	if (verbose>0 && (i+1)%10==0) {
#ifdef O2SCL_OPENMP
	  std::cout << "nrf progress: " << i+1 << "/" << t3d.get_nx()
		    << " (" << omp_get_thread_num() << "/"
		    << omp_get_num_threads() << " threads)." << std::endl;
#else
	  std::cout << "nrf progress: " << i+1 << "/" << t3d.get_nx()
		    << std::endl;
#endif
	}
	
	for(size_t j=0;j<t3d.get_ny();j++) {
	  
	  x0[0]=gx[i];
	  x0[1]=gy[j];
	  
	  bool found=false;
	  
	  for(size_t k=0;k<kmax && found==false;k++) {

	    // x_{n+1} = x_n - J(x_n)^{-1} f(x_n)
	    f(2,x0,fx,J);
	    // The inverse of the Jacobian
	    fp_t det=J(0,0)*J(1,1)-J(0,1)*J(1,0);
	    fp_t ai=J(1,1)/det;
	    fp_t bi=-J(0,1)/det;
	    fp_t ci=-J(1,0)/det;
	    fp_t di=J(0,0)/det;
	    // The NR step
	    x1[0]=x0[0]-ai*fx[0]-bi*fx[1];
	    x1[1]=x0[1]-ci*fx[0]-di*fx[1];

	    // Test for divergence or convergence
	    fp_t dx=x1[0]-x0[0];
	    fp_t dy=x1[1]-x0[1];
	    if (dx*dx+dy*dy>rmax*rmax) {
	      found=true;
	      t3d.set(i,j,"it",kmax);
	      t3d.set(i,j,"root",-1);
	    } else if (fabs(dx)+fabs(dy)<1.0e-14) {
	      found=true;
	      bool root_found=false;
	      for(size_t m=0;m<roots_x.size();m++) {
		if (fabs(x1[0]-roots_x[m])<1.0e-13 &&
		    fabs(x1[1]-roots_y[m])<1.0e-13) {
		  root_found=true;
		  t3d.set(i,j,"it",k+1);
		  t3d.set(i,j,"root",m+1);
		  if (k+1>max_count[m]) {
		    max_count[m]=k+1;
		  }
		  if (k+1<min_count[m]) {
		    min_count[m]=k+1;
		  }
		}
	      }
	      if (root_found==false) {
#ifdef O2SCL_OPENMP
		std::cout << x0[0] << " " << x0[1] << std::endl;
		std::cout << x1[0] << " " << x1[1] << std::endl;
		o2scl::vector_out(std::cout,roots_x,true);
		o2scl::vector_out(std::cout,roots_y,true);
		O2SCL_ERR2("Fract cannot handle new roots when ",
			   "OpenMP is enabled.",o2scl::exc_einval);
#else
		size_t m=roots_x.size();
		roots_x.push_back(x1[0]);
		roots_y.push_back(x1[1]);
		max_count.push_back(k+1);
		min_count.push_back(k+1);
		t3d.set(i,j,"it",k+1);
		t3d.set(i,j,"root",m+1);
#endif
	      }
	    }

	    // Proceed to next iteration
	    x0[0]=x1[0];
	    x0[1]=x1[1];
	  }

	  // In the case of neither convergence nor divergence
	  if (found==false) {
	    t3d.set(i,j,"it",kmax);
	  }

	  // Proceed to next point
	}
      }

      return 0;
    }

    /** \brief Generate a Newton-Raphson fractal for \f$ z^4-1 \f$ and
        store in the \ref o2scl::table3d object \c t3d
    */
    template<class mat_t=ubmatrix,
      class vec_t=ubvector, class vec2_t=std::vector<double>,
             class vec2_size_t=std::vector<size_t>, class fp_t=double>
    int nrf_z4m1(uniform_grid<fp_t> &gx,
                 uniform_grid<fp_t> &gy, size_t kmax, fp_t rmax,
                 o2scl::table3d &t3d, vec2_t &roots_x,
                 vec2_t &roots_y, vec2_size_t &min_count,
                 vec2_size_t &max_count) {
      nrf_funct f=o2scl::nrf_z4m1;
      return nrf(f,gx,gy,kmax,rmax,t3d,roots_x,roots_y,min_count,
                 max_count);
    }
      
    /** \brief Generate an iteration fractal and
        store in the \ref o2scl::table3d object \c t3d

        FIXME: fix parallelization
     */
    template<class func_t, class vec_t=ubvector, class fp_t=double>
    int itf(func_t &f, uniform_grid<fp_t> &gx,
            uniform_grid<fp_t> &gy, size_t kmax, fp_t rmax,
            o2scl::table3d &t3d, size_t &min, size_t &max) {
      
      t3d.clear();

      min=kmax;
      max=0;

      t3d.set_xy("x",gx,"y",gy);
      t3d.new_slice("it");
      t3d.set_slice_all("it",0.0);

      vec_t z(2), c(2);

      // AWS 5/6/21: for some reason the parallelization of this
      // causes problems at the moment.
      
      //#ifdef O2SCL_OPENMP
      //#pragma omp parallel for
      //#endif
      for(size_t i=0;i<t3d.get_nx();i++) {

	if (verbose>0 && (i+1)%10==0) {
#ifdef O2SCL_OPENMP
          std::cout << "itf progress: " << i+1 << "/" << t3d.get_nx()
		    << " (" << omp_get_thread_num() << "/"
		    << omp_get_num_threads() << " threads)." << std::endl;
#else
	  std::cout << "itf progress: " << i+1 << "/" << t3d.get_nx()
		    << std::endl;
#endif
	}
	
	for(size_t j=0;j<t3d.get_ny();j++) {

	  z[0]=0.0;
	  z[1]=0.0;
          c[0]=gx[i];
          c[1]=gy[j];
          /*
            std::cout.setf(std::ios::showpos);
            std::cout << i << "." << j << std::endl;
            std::cout << z[0] << " " << z[1] << " " << c[0] << " " << c[1]
            << std::endl;
          */

	  bool found=false;
	  
	  for(size_t k=0;k<kmax && found==false;k++) {

            f(z,c);
            /*
              std::cout << k << " " << z[0] << " " << z[1] << " "
              << c[0] << " " << c[1]
              << std::endl;
            */

            if (z[0]*z[0]+z[1]*z[1]>rmax*rmax) {
	      found=true;
	      t3d.set(i,j,"it",k);
              if (k<min) min=k;
              if (k>max) max=k;
	    }

	  }

	  // In the case of neither convergence nor divergence
	  if (found==false) {
	    t3d.set(i,j,"it",kmax);
            if (kmax<min) min=kmax;
            if (kmax>max) max=kmax;
	  }

	  // Proceed to next point
	}
      }

      return 0;
    }

#ifdef O2SCL_NEVER_DEFINED

    /** \brief

        FIXME: explain what was intended here, maybe an animation?
     */
    template<class func_t, class vec_t=ubvector, class fp_t=double>
    int itfa(func_t &f,
             fp_t x_min_0, fp_t x_max_0, fp_t y_min_0, fp_t y_max_0, 
             fp_t x_min_1, fp_t x_max_1, fp_t y_min_1, fp_t y_max_1,
             size_t nf, size_t nx=200, size_t ny=200, 
             size_t kmax=100, fp_t rmax=10.0, o2scl::table3d &t3d) {
      
      t3d.clear();

      min=kmax;
      max=0;

      t3d.set_xy("x",gx,"y",gy);
      t3d.new_slice("it");
      t3d.set_slice_all("it",0.0);

      vec_t z(2), c(2);

      // AWS 5/6/21: for some reason the parallelization of this
      // causes problems at the moment.
      
      //#ifdef O2SCL_OPENMP
      //#pragma omp parallel for
      //#endif
      for(size_t i=0;i<t3d.get_nx();i++) {

	if (verbose>0 && (i+1)%10==0) {
#ifdef O2SCL_OPENMP
          std::cout << "itf progress: " << i+1 << "/" << t3d.get_nx()
		    << " (" << omp_get_thread_num() << "/"
		    << omp_get_num_threads() << " threads)." << std::endl;
#else
	  std::cout << "itf progress: " << i+1 << "/" << t3d.get_nx()
		    << std::endl;
#endif
	}
	
	for(size_t j=0;j<t3d.get_ny();j++) {

	  z[0]=0.0;
	  z[1]=0.0;
          c[0]=gx[i];
          c[1]=gy[j];
          /*
            std::cout.setf(std::ios::showpos);
            std::cout << i << "." << j << std::endl;
            std::cout << z[0] << " " << z[1] << " " << c[0] << " " << c[1]
            << std::endl;
          */

	  bool found=false;
	  
	  for(size_t k=0;k<kmax && found==false;k++) {

            f(z,c);
            /*
              std::cout << k << " " << z[0] << " " << z[1] << " "
              << c[0] << " " << c[1]
              << std::endl;
            */

            if (z[0]*z[0]+z[1]*z[1]>rmax*rmax) {
	      found=true;
	      t3d.set(i,j,"it",k);
              if (k<min) min=k;
              if (k>max) max=k;
	    }

	  }

	  // In the case of neither convergence nor divergence
	  if (found==false) {
	    t3d.set(i,j,"it",kmax);
            if (kmax<min) min=kmax;
            if (kmax>max) max=kmax;
	  }

	  // Proceed to next point
	}
      }

      return 0;
    }
    
#endif

    /** \brief Generate an iteration fractal for the Mandelbrot set
        and store in the \ref o2scl::table3d object \c t3d
    */
    template<class vec_t=ubvector, class fp_t=double>
    int itf_mandel(uniform_grid<fp_t> &gx, uniform_grid<fp_t> &gy,
                   size_t kmax, fp_t rmax, o2scl::table3d &t3d,
                   size_t &min, size_t &max) {
      itf_funct f=o2scl::itf_mandel;
      return itf(f,gx,gy,kmax,rmax,t3d,min,max);
    }
    
  };
  
#ifndef DOXYGEN_NO_O2NS
}
#endif

#endif

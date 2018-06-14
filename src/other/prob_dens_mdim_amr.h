/*
  -------------------------------------------------------------------
  
  Copyright (C) 2018, Andrew W. Steiner
  
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
/** \file prob_dens_mdim_amr.h
    \brief File defining \ref o2scl::matrix_view, 
    \ref o2scl::matrix_view_table, and \ref o2scl::prob_dens_mdim_amr
*/
#ifndef O2SCL_PROB_DENS_MDIM_AMR_H
#define O2SCL_PROB_DENS_MDIM_AMR_H

#include <o2scl/table.h>
#include <o2scl/err_hnd.h>
#include <o2scl/prob_dens_func.h>
#include <o2scl/rng_gsl.h>
#include <o2scl/vector.h>

#ifndef DOXYGEN_NO_O2NS
namespace o2scl {
#endif

  /** \brief Probability distribution from an adaptive mesh
      created using a matrix of points

      \note This class is experimental.

      \future The storage required by the mesh is larger
      than necessary, and could be replaced by a tree-like
      structure which uses less storage.
  */
  template<class vec_t=std::vector<double>,
    class mat_t=matrix_view_table<vec_t> >
    class prob_dens_mdim_amr : public o2scl::prob_dens_mdim<vec_t> {

  public:

  /** \brief A hypercube class for \ref o2scl::prob_dens_mdim_amr
   */
  class hypercube {

  public:

    /** \brief The number of dimensions
     */
    size_t n_dim;
    /** \brief The corner of smallest values 
     */
    std::vector<double> low;
    /** \brief The corner of largest values 
     */
    std::vector<double> high;
    /** \brief The list of indices inside
     */
    std::vector<size_t> inside;
    /** \brief The fractional volume enclosed
     */
    double frac_vol;
    /** \brief The weight 
     */
    double weight;

    /** \brief Create an empty hypercube
     */
    hypercube() {
      n_dim=0;
    }

    /** \brief Set the hypercube information
     */
    template<class vec2_t>
      void set(vec2_t &l, vec2_t &h, size_t in, double fvol, double wgt) {
      n_dim=l.size();
      low.resize(l.size());
      high.resize(h.size());
      inside.resize(1);
      inside[0]=in;
      for(size_t i=0;i<l.size();i++) {
	if (low[i]>high[i]) {
	  low[i]=h[i];
	  high[i]=l[i];
	} else {
	  low[i]=l[i];
	  high[i]=h[i];
	}
      }
      frac_vol=fvol;
      weight=wgt;
      return;
    }
  
    /** \brief Copy constructor
     */
    hypercube(const hypercube &h) {
      n_dim=h.n_dim;
      low=h.low;
      high=h.high;
      inside=h.inside;
      frac_vol=h.frac_vol;
      weight=h.weight;
      return;
    }

    /** \brief Copy constructor through <tt>operator=()</tt>
     */
    hypercube &operator=(const hypercube &h) {
      if (this!=&h) {
	n_dim=h.n_dim;
	low=h.low;
	high=h.high;
	inside=h.inside;
	frac_vol=h.frac_vol;
	weight=h.weight;
      }
      return *this;
    }
  
    /** \brief Test if point \c v is inside this hypercube
     */
    template<class vec2_t> bool is_inside(const vec2_t &v) const {
      for(size_t i=0;i<n_dim;i++) {
	if (high[i]<v[i] || v[i]<low[i]) {
	  return false;
	}
      }
      return true;
    }
  
  };

  /// \name Dimension choice setting
  int dim_choice;
  static const int max_variance=1;
  static const int user_scale=2;
  static const int random=3;

  /** \brief Internal random number generator
   */
  mutable o2scl::rng_gsl rg;
 
  /** \brief Number of dimensions
   */
  size_t n_dim;

  /** \brief Corner of smallest values
   */
  vec_t low;
  
  /** \brief Corner of largest values
   */
  vec_t high;

  /** \brief Vector of length scales
   */
  vec_t scale;

  /** \brief Mesh stored as an array of hypercubes
   */
  std::vector<hypercube> mesh;

  /** \brief Verbosity parameter
   */
  int verbose;

  /** \brief Desc
   */
  void clear() {
    mesh.clear();
    low.clear();
    high.clear();
    scale.clear();
    n_dim=0;
    return;
  }
  
  /** \brief Desc
   */
  void clear_mesh() {
    mesh.clear();
    return;
  }
  
  prob_dens_mdim_amr() {
    n_dim=0;
    dim_choice=max_variance;
  }
  
  /** \brief Initialize a probability distribution from the corners
   */
  prob_dens_mdim_amr(vec_t &l, vec_t &h) {
    dim_choice=max_variance;
    set(l,h);
  }
  
  /** \brief Copy the object data to a few numbers and two vectors
   */
  void copy_to_vectors(size_t &nd, size_t &dc, size_t &ms,
		       std::vector<double> &data,
		       std::vector<size_t> &insides) {
    nd=n_dim;
    dc=dim_choice;
    ms=mesh.size();
    data.clear();
    for(size_t k=0;k<nd;k++) {
      data.push_back(low[k]);
    }
    for(size_t k=0;k<nd;k++) {
      data.push_back(high[k]);
    }
    if (dim_choice==user_scale) {
      for(size_t k=0;k<nd;k++) {
	data.push_back(scale[k]);
      }
    }
    for(size_t k=0;k<ms;k++) {
      data.push_back(mesh[k].weight);
      data.push_back(mesh[k].frac_vol);
      for(size_t k2=0;k2<n_dim;k2++) {
	data.push_back(mesh[k].low[k2]);
	data.push_back(mesh[k].high[k2]);
      }
    }
    insides.clear();
    for(size_t k=0;k<ms;k++) {
      insides.push_back(mesh[k].inside.size());
      for(size_t k2=0;k2<mesh[k].inside.size();k2++) {
	insides.push_back(mesh[k].inside[k2]);
      }
    }
    return;
  }

  /** \brief Set the object from data specified as a set
      of two vectors
   */
  void set_from_vectors(size_t &nd, size_t &dc, size_t &ms,
			const std::vector<double> &data,
			const std::vector<size_t> &insides) {
    n_dim=nd;
    dim_choice=dc;
    low.resize(n_dim);
    high.resize(n_dim);
    size_t ix=0;
    for(size_t k=0;k<nd;k++) {
      low[k]=data[ix];
      ix++;
    }
    for(size_t k=0;k<nd;k++) {
      high[k]=data[ix];
      ix++;
    }
    if (dim_choice==user_scale) {
      scale.resize(n_dim);
      for(size_t k=0;k<nd;k++) {
	scale[k]=data[ix];
	ix++;
      }
    }
    mesh.resize(ms);
    for(size_t k=0;k<ms;k++) {
      mesh[k].n_dim=n_dim;
      mesh[k].weight=data[ix];
      ix++;
      mesh[k].frac_vol=data[ix];
      ix++;
      mesh[k].low.resize(n_dim);
      mesh[k].high.resize(n_dim);
      for(size_t k2=0;k2<n_dim;k2++) {
	mesh[k].low[k2]=data[ix];
	ix++;
	mesh[k].high[k2]=data[ix];
	ix++;
      }
    }
    ix=0;
    for(size_t k=0;k<ms;k++) {
      size_t isize=insides[ix];
      ix++;
      mesh[k].inside.resize(isize);
      for(size_t k2=0;k2<isize;k2++) {
	mesh[k].inside[k2]=insides[ix];
	ix++;
      }
    }
    return;
  }
  
  /** \brief Set the mesh limits

      This function is called by the constructor
   */
  void set(vec_t &l, vec_t &h) {
    clear_mesh();
    if (h.size()<l.size()) {
      O2SCL_ERR2("Vector sizes not correct in ",
		"prob_dens_mdim_amr::set().",o2scl::exc_einval);
    }
    low.resize(l.size());
    high.resize(h.size());
    for(size_t i=0;i<l.size();i++) {
      low[i]=l[i];
      high[i]=h[i];
    }
    n_dim=low.size();
    verbose=1;
    return;
  }

  /** \brief Set scales for dimension choice
   */
  template<class vec2_t>
  void set_scale(vec2_t &v) {
    scale.resize(v.size());
    o2scl::vector_copy(v,scale);
    return;
  }
  
  /** \brief Insert point at row \c ir, creating a new hypercube 
      for the new point
   */
  void insert(size_t ir, mat_t &m, bool log_mode=false) {
    if (n_dim==0) {
      O2SCL_ERR2("Region limits and scales not set in ",
		 "prob_dens_mdim_amr::insert().",o2scl::exc_einval);
    }
    if (log_mode==false && m(ir,n_dim)<0.0) {
      O2SCL_ERR2("Weight negative in ",
		 "prob_dens_mdim_amr::insert().",o2scl::exc_einval);
    }

    if (mesh.size()==0) {
      // Initialize the mesh with the first point
      mesh.resize(1);
      if (log_mode) {
	if (m(ir,n_dim)>-800.0) {
	  mesh[0].set(low,high,ir,1.0,exp(m(ir,n_dim)));
	} else {
	  mesh[0].set(low,high,ir,1.0,0.0);
	}
      } else {
	mesh[0].set(low,high,ir,1.0,m(ir,n_dim));
      }
      if (verbose>1) {
	std::cout << "First hypercube from index "
	<< ir << "." << std::endl;
	for(size_t k=0;k<n_dim;k++) {
	  std::cout.width(3);
	  std::cout << k << " ";
	  std::cout.setf(std::ios::showpos);
	  std::cout << low[k] << " "
		    << m(ir,k) << " " << high[k] << std::endl;
	  std::cout.unsetf(std::ios::showpos);
	}
	std::cout << "weight: " << mesh[0].weight << std::endl;
	if (verbose>2) {
	  std::cout << "Press a character and enter to continue: " << std::endl;
	  char ch;
	  std::cin >> ch;
	}
      }

      return;
    }
   
    // Convert the row to a vector
    std::vector<double> v;
    for(size_t k=0;k<n_dim;k++) {
      v.push_back(m(ir,k));
    }
    if (verbose>1) {
      std::cout << "Finding cube with point ";
      for(size_t k=0;k<n_dim;k++) {
	std::cout << v[k] << " ";
      }
      std::cout << std::endl;
    }
   
    // Find the right hypercube
    bool found=false;
    size_t jm=0;
    for(size_t j=0;j<mesh.size() && found==false;j++) {
      if (mesh[j].is_inside(v)) {
	found=true;
	jm=j;
      }
    }
    if (found==false) {
      for(size_t k=0;k<n_dim;k++) {
	std::cerr << k << " " << low[k] << " " << v[k] << " "
	<< high[k] << std::endl;
      }
      O2SCL_ERR2("Couldn't find point inside mesh in ",
		 "prob_dens_mdim_amr::insert().",o2scl::exc_efailed);
    }
    hypercube &h=mesh[jm];
    if (verbose>1) {
      std::cout << "Found cube " << jm << std::endl;
    }
   
    // Find coordinate to separate
    size_t max_ip=0;
    if (dim_choice==random) {
      max_ip=((size_t)(rg.random()*((double)n_dim)));
      if (verbose>1) {
	std::cout << "Randomly chose coordinate " << max_ip
		  << std::endl;
      }
      
    } else {
      double max_var;
      if (dim_choice==max_variance) {
	max_var=fabs(v[0]-m(h.inside[0],0))/(h.high[0]-h.low[0]);
      } else {
	if (scale.size()==0) {
	  O2SCL_ERR2("Scales not set in ",
		     "prob_dens_mdim_amr::insert().",o2scl::exc_einval);
	}
	max_var=fabs(v[0]-m(h.inside[0],0))/scale[0];
      }
      for(size_t ip=1;ip<n_dim;ip++) {
	double var;
	if (dim_choice==max_variance) {
	  var=fabs(v[ip]-m(h.inside[0],ip))/(h.high[ip]-h.low[ip]);
	} else {
	  var=fabs(v[ip]-m(h.inside[0],ip))/scale[ip % scale.size()];
	}
	if (var>max_var) {
	  max_ip=ip;
	  max_var=var;
	}
      }
    }
   
    // Slice the mesh in coordinate max_ip
    double loc=(v[max_ip]+m(h.inside[0],max_ip))/2.0;
    if (verbose>1) {
      std::cout << "Chose coordinate " << max_ip << "."
		<< std::endl;
      std::cout << "Point, between, previous, low, high:\n\t"
		<< v[max_ip] << " " << loc << " "
		<< m(h.inside[0],max_ip) << " "
		<< h.low[max_ip] << " " << h.high[max_ip] << std::endl;
    }
    double old_vol=h.frac_vol;
    double old_low=h.low[max_ip];
    double old_high=h.high[max_ip];

    size_t old_inside=h.inside[0];
    double old_weight=h.weight;

    // Set values for hypercube currently in mesh
    h.low[max_ip]=loc;
    h.high[max_ip]=old_high;
    h.frac_vol=old_vol*(old_high-loc)/(old_high-old_low);
    if (!std::isfinite(h.frac_vol)) {
      std::cout << "Mesh has non-finite fractional volume: "
		<< old_vol << " " << old_high << " "
		<< loc << " " << old_low << std::endl;
      O2SCL_ERR2("Mesh has non finite fractional volume",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_esanity);
    }
   
    // Set values for new hypercube
    hypercube h_new;
    std::vector<double> low_new, high_new;
    o2scl::vector_copy(h.low,low_new);
    o2scl::vector_copy(h.high,high_new);
    low_new[max_ip]=old_low;
    high_new[max_ip]=loc;
    double new_vol=old_vol*(loc-old_low)/(old_high-old_low);
    if (log_mode) {
      if (m(ir,n_dim)>-800.0) {
	h_new.set(low_new,high_new,ir,new_vol,exp(m(ir,n_dim)));
      } else {
	h_new.set(low_new,high_new,ir,new_vol,0.0);
      }
    } else {
      h_new.set(low_new,high_new,ir,new_vol,m(ir,n_dim));
    }
    if (!std::isfinite(h_new.weight)) {
      O2SCL_ERR2("Mesh has non finite weight ",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_einval);
    }

    // --------------------------------------------------------------
    // Todo: this test is unnecessarily slow, and can be replaced by a
    // simple comparison between v[max_ip], old_low, old_high, and
    // m(h.inside[0],max_ip)
    
    if (h.is_inside(v)) {
      h.inside[0]=ir;
      h_new.inside[0]=old_inside;
      if (log_mode) {
	if (m(ir,n_dim)>-800.0) {
	  h.weight=exp(m(ir,n_dim));
	} else {
	  h.weight=0.0;
	}
      } else {
	h.weight=m(ir,n_dim);
      }
      h_new.weight=old_weight;
    } else {
      h.inside[0]=old_inside;
      h_new.inside[0]=ir;
      h.weight=old_weight;
      if (log_mode) {
	if (m(ir,n_dim)>-800.0) {
	  h_new.weight=exp(m(ir,n_dim));
	} else {
	  h_new.weight=0.0;
	}
      } else {
	h_new.weight=m(ir,n_dim);
      }
    }
    if (!std::isfinite(h.weight)) {
      O2SCL_ERR2("Mesh has non finite weight ",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_einval);
    }
    if (!std::isfinite(h_new.weight)) {
      O2SCL_ERR2("Mesh has non finite weight ",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_einval);
    }
    if (!std::isfinite(h.frac_vol)) {
      O2SCL_ERR2("Mesh has non finite fractional volume",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_esanity);
    }
    if (!std::isfinite(h_new.frac_vol)) {
      O2SCL_ERR2("Mesh has non finite fractional volume",
		 "in prob_dens_mdim_amr::insert().",o2scl::exc_esanity);
    }

    // --------------------------------------------------------------
    
    if (verbose>1) {
      std::cout << "Modifying hypercube with index "
      << jm << " and inserting new hypercube for row " << ir
      << "." << std::endl;
      for(size_t k=0;k<n_dim;k++) {
	if (k==max_ip) std::cout << "*";
	else std::cout << " ";
	std::cout.width(3);
	std::cout << k << " ";
	std::cout.setf(std::ios::showpos);
	std::cout << h.low[k] << " "
		  << h.high[k] << " "
		  << h_new.low[k] << " " << m(ir,k) << " "
		  << h_new.high[k] << std::endl;
	std::cout.unsetf(std::ios::showpos);
      }
      std::cout << "Weights: " << h.weight << " " << h_new.weight
      << std::endl;
      std::cout << "Frac. volumes: " << h.frac_vol << " "
      << h_new.frac_vol << std::endl;
      if (verbose>2) {
	std::cout << "Press a character and enter to continue: " << std::endl;
	char ch;
	std::cin >> ch;
      }
    }

    // Add new hypercube to mesh
    mesh.push_back(h_new);
   
    return;
  }
  
  /** \brief Parse the matrix \c m, creating a new hypercube
      for every point 
   */
  void initial_parse(mat_t &m, bool log_mode=false) {

    for(size_t ir=0;ir<m.size1();ir++) {
      insert(ir,m,log_mode);
    }
    if (verbose>0) {
      std::cout << "Done in initial_parse(). "
      << "Volumes: " << total_volume() << " "
      << total_weighted_volume() << std::endl;
    }
    if (verbose>2) {
      std::cout << "Press a character and enter to continue: " << std::endl;
      char ch;
      std::cin >> ch;
    }
    
    return;
  }

  /** \brief Parse the matrix \c m, creating a new hypercube for every
      point, ensuring hypercubes are more optimally arranged

      This algorithm is slower, but may result in more balanced
      meshes, particularly when \ref dim_choice is not equal to
      <tt>random</tt> .

      \future This method computes distances twice, once here 
      and once in the insert() function. There is likely a
      faster approach.
  */
  void initial_parse_new(mat_t &m) {

    size_t N=m.size1();
    std::vector<bool> added(N);
    for(size_t i=0;i<N;i++) added[i]=false;

    std::vector<double> scale2(n_dim);
    for(size_t i=0;i<n_dim;i++) {
      scale2[i]=fabs(high[i]-low[i]);
    }

    // First, find the two furthest points
    size_t p0, p1;
    {
      std::vector<size_t> iarr, jarr;
      std::vector<double> distarr;
      for(size_t i=0;i<N;i++) {
	for(size_t j=i+1;j<N;j++) {
	  iarr.push_back(i);
	  jarr.push_back(j);
	  double dist=0.0;
	  for(size_t k=0;k<n_dim;k++) {
	    dist+=pow((m(i,k)-m(j,k))/scale2[k],2.0);
	  }
	  distarr.push_back(sqrt(dist));
	}
      }
      std::vector<size_t> indexarr(iarr.size());
      vector_sort_index(distarr,indexarr);
      p0=iarr[indexarr[indexarr.size()-1]];
      p1=jarr[indexarr[indexarr.size()-1]];
    }

    // Add them to the mesh
    insert(p0,m);
    added[p0]=true;
    insert(p1,m);
    added[p1]=true;

    // Now loop through all points, find the point furthest from the
    // point already in the hypercube in which it would lie
    bool done=false;
    while (done==false) {
      done=true;

      // First compute distances for all points not already added
      std::vector<size_t> iarr;
      std::vector<double> distarr;
      for(size_t i=0;i<N;i++) {
	if (added[i]==false) {
	  done=false;
	  std::vector<double> x(n_dim);
	  for(size_t k=0;k<n_dim;k++) x[k]=m(i,k);
	  const hypercube &h=find_hc(x);
	  iarr.push_back(i);
	  double dist=0.0;
	  for(size_t k=0;k<n_dim;k++) {
	    dist+=pow((m(i,k)-m(h.inside[0],k))/(h.high[k]-h.low[k]),2.0);
	  }
	  distarr.push_back(dist);
	}
      }

      // If we've found at least one point, add it to the mesh
      if (done==false) {
	std::vector<size_t> indexarr(iarr.size());
	vector_sort_index(distarr,indexarr);
	insert(iarr[indexarr[indexarr.size()-1]],m);
	added[iarr[indexarr[indexarr.size()-1]]]=true;
      }

      // Proceed to the next point
    }

    return;
  }

  /** \brief Set the weight in each hypercube equal to the
      inverse of the volume (the density)
   */
  void weight_is_inv_volume() {
    for(size_t i=0;i<mesh.size();i++) {
      mesh[i].weight=1.0/mesh[i].frac_vol;
    }
    return;
  }
  
  /** \brief Check the total volume by adding up the fractional
      part of the volume in each hypercube
   */
  double total_volume() {
    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh empty in ",
		 "prob_dens_mdim_amr::total_volume().",o2scl::exc_einval);
    }
    double ret=0.0;
    for(size_t i=0;i<mesh.size();i++) {
      if (!std::isfinite(mesh[i].frac_vol)) {
	O2SCL_ERR2("Mesh has non finite fractional volume",
		   "in prob_dens_mdim_amr::insert().",o2scl::exc_esanity);
      }
      ret+=mesh[i].frac_vol;
    }
    return ret;
  }

  /** \brief Check the total volume by adding up the fractional
      part of the volume in each hypercube
   */
  double total_weighted_volume() {
    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh empty in ",
		 "prob_dens_mdim_amr::total_weighted_volume().",
		 o2scl::exc_einval);
    }
    double ret=0.0;
    for(size_t i=0;i<mesh.size();i++) {
      ret+=mesh[i].frac_vol*mesh[i].weight;
    }
    return ret;
  }

  /** \brief Return a reference to the hypercube containing the
      specified point
   */
  const hypercube &find_hc(const vec_t &x) const {
    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh has zero size in ",
		 "prob_dens_mdim_amr::find_hc().",o2scl::exc_efailed);
    }
    for(size_t j=0;j<n_dim;j++) {
      if (x[j]<low[j] || x[j]>high[j]) {
	O2SCL_ERR2("Point outside region in ",
		   "prob_dens_mdim_amr::find_hc().",o2scl::exc_einval);
      }
    }
    for(size_t j=0;j<mesh.size();j++) {
      if (mesh[j].is_inside(x)) {
	return mesh[j];
      }
    }
    O2SCL_ERR2("Could not find hypercube in ",
	       "prob_dens_mdim_amr::find_hc().",o2scl::exc_efailed);
    return mesh[0];
  }
  
  /// The normalized density 
  virtual double pdf(const vec_t &x) const {

    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh empty in ",
		 "prob_dens_mdim_amr::pdf().",o2scl::exc_einval);
    }

    // Find the right hypercube
    bool found=false;
    size_t jm=0;
    for(size_t j=0;j<mesh.size() && found==false;j++) {
      if (mesh[j].is_inside(x)) {
	found=true;
	jm=j;
      }
    }
    if (found==false) {
      O2SCL_ERR("Error 2.",o2scl::exc_esanity);
    }
    return mesh[jm].weight;
  }

  /// Select a random point in the largest weighted box
  virtual void select_in_largest(vec_t &x) const {
   
    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh empty in ",
		 "prob_dens_mdim_amr::select_in_largest().",o2scl::exc_einval);
    }

    size_t im=0;
    double wgt=mesh[0].frac_vol*mesh[0].weight;
    for(size_t i=1;i<mesh.size();i++) {
      if (mesh[i].frac_vol*mesh[i].weight>wgt) {
	im=i;
	wgt=mesh[i].frac_vol*mesh[i].weight;
      }
    }
    for(size_t j=0;j<n_dim;j++) {
      x[j]=rg.random()*(mesh[im].high[j]-mesh[im].low[j])+mesh[im].low[j];
    }

    return;
  }

  /// Sample the distribution
  virtual void operator()(vec_t &x) const {
   
    if (mesh.size()==0) {
      O2SCL_ERR2("Mesh empty in ",
		 "prob_dens_mdim_amr::operator()().",o2scl::exc_einval);
    }

    double total_weight=0.0;
    for(size_t i=0;i<mesh.size();i++) {
      total_weight+=mesh[i].weight*mesh[i].frac_vol;
    }
   
    double this_weight=rg.random()*total_weight;
    double cml_wgt=0.0;
    for(size_t j=0;j<mesh.size();j++) {
      cml_wgt+=mesh[j].frac_vol*mesh[j].weight;
      if (this_weight<cml_wgt || j==mesh.size()-1) {
	for(size_t i=0;i<n_dim;i++) {
	  x[i]=mesh[j].low[i]+rg.random()*
	    (mesh[j].high[i]-mesh[j].low[i]);
	}
	return;
      }
    }

    return;
  }
 
  };
 
#ifndef DOXYGEN_NO_O2NS
}
#endif

#endif

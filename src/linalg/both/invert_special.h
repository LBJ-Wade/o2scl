/*
  -------------------------------------------------------------------
  
  Copyright (C) 2006-2018, Andrew W. Steiner
  
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
#ifndef O2SCL_INVERT_SPECIAL_H
#define O2SCL_INVERT_SPECIAL_H

#if !defined (O2SCL_COND_FLAG) || defined (O2SCL_ARMA)
#include <armadillo>
namespace o2scl_linalg {

  /** \brief Armadillo inverse 

      This class is only defined if Armadillo support was enabled
      during installation
  */
  template<class arma_mat_t> class matrix_invert_arma : 
    public matrix_invert<arma_mat_t> {
    
    virtual void invert(size_t n, const arma_mat_t &A, arma_mat_t &Ainv) {
      Ainv=inv(A);
      return;
    }

    virtual void invert_inplace(size_t n, arma_mat_t &A) {
      A=inv(A);
      return;
    }

    virtual ~matrix_invert_arma() {}
    
  };

  /** \brief Armadillo inverse of symmetric positive definite matrix

      This class is only defined if Armadillo support was enabled
      during installation
  */
  template<class arma_mat_t> class matrix_invert_sympd_arma : 
    public matrix_invert<arma_mat_t> {
    
    virtual void invert(size_t n, const arma_mat_t &A, arma_mat_t &Ainv) {
      Ainv=inv_sympd(A);
      return;
    }

    virtual void invert_inplace(size_t n, arma_mat_t &A) {
      A=inv_sympd(A);
      return;
    }

    virtual ~matrix_invert_sympd_arma() {}
    
  };
}

#endif

#if !defined (O2SCL_COND_FLAG) || defined (O2SCL_EIGEN)
#include <eigen3/Eigen/Dense>
namespace o2scl_linalg {

  /** \brief Eigen inverse using QR decomposition with 
      column pivoting

      This class is only defined if Eigen support was enabled during
      installation.

  */
  template<class eigen_mat_t>
  class matrix_invert_eigen : 
    public matrix_invert<eigen_mat_t> {
    
    /// Desc
    virtual void invert(size_t n, const eigen_mat_t &A, eigen_mat_t &Ainv) {
      Ainv=A.inverse();
      return;
    }
    
    /// Desc
    virtual void invert_inplace(size_t n, eigen_mat_t &A) {
      A=A.inverse();
      return;
    }
    
  };
  

}
#endif

#endif

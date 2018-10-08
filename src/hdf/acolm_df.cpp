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
#include "acolm.h"

#include <o2scl/cloud_file.h>
#include <o2scl/vector_derint.h>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>

using namespace std;
using namespace o2scl;
using namespace o2scl_hdf;
using namespace o2scl_acol;

typedef boost::numeric::ublas::vector<double> ubvector;
typedef boost::numeric::ublas::matrix<double> ubmatrix;

int acol_manager::comm_diag(std::vector<std::string> &sv, bool itive_com) {

  if (type=="tensor") {

    size_t rk=tensor_obj.get_rank();
    size_t n=tensor_obj.get_size(0);
    for(size_t i=1;i<rk;i++) {
      if (tensor_obj.get_size(i)<n) {
	n=tensor_obj.get_size(i);
      }
    }

    doublev_obj.clear();
    vector<size_t> ix(rk);
    for(size_t i=0;i<n;i++) {
      for(size_t j=0;j<rk;j++) {
	ix[j]=i;
      }
      doublev_obj.push_back(tensor_obj.get(ix));
    }
    
    command_del();
    clear_obj();
    command_add("double[]");
    type="double[]";
    
  } else {
    
    cerr << "Cannot use command 'diag' for type "
	 << type << "." << endl;
    return exc_efailed;
  }
  
  return 0;
}

int acol_manager::comm_download(std::vector<std::string> &sv, bool itive_com) {

  cloud_file cf;
  std::string file, hash, url, fname;

  if (sv.size()==3) {
    
    file=sv[1];
    url=sv[2];
    if (verbose>0) {
      cout << "No hash specified, so download is not verified." << endl;
    }
    cf.get_file(file,url,"");
    return 0;
    
  } 
  
  vector<string> in, pr;
  pr.push_back("Destination filename");
  pr.push_back("URL");
  pr.push_back("Hash");
  int ret=get_input(sv,pr,in,"download",itive_com);
  if (ret!=0) return ret;

  file=in[0];
  url=in[1];
  hash=in[2];

  if ((hash[0]=='f' || hash[0]=='F') &&
      (hash[1]=='i' || hash[1]=='I') &&
      (hash[2]=='l' || hash[2]=='L') &&
      (hash[3]=='e' || hash[3]=='E') &&
      hash[4]==':') {
    string hash_file=hash.substr(5,hash.size()-5);
    ifstream fin;
    fin.open(hash_file.c_str());
    fin >> hash;
    fin.close();
    if (verbose>0) {
      cout << "Obtained hash " << hash << " from file " << hash_file << endl;
    }
  }

  cf.verbose=verbose;
  if (hash==((std::string)"None") ||
      hash==((std::string)"none") || hash.length()==0) {
    cf.get_file(file,url,"");
  } else {
    cf.get_file_hash(file,hash,url,"");
  }
  
  return 0;
}

int acol_manager::comm_deriv(std::vector<std::string> &sv, bool itive_com) {

  if (type=="table") {
    
    if (table_obj.get_nlines()==0) {
      cerr << "No table with columns to take derivatives of." << endl;
      return exc_efailed;
    }
    
    vector<string> pr, in;
    pr.push_back("Enter 'x' column");
    pr.push_back("Enter 'y' column");
    pr.push_back("Enter name of new column");
    int ret=get_input(sv,pr,in,"deriv",itive_com);
    if (ret!=0) return ret;
    
    if (table_obj.is_column(in[0])==false) {
      cerr << "Could not find column named '" << in[0] << "'." << endl;
      return exc_efailed;
    }
    if (table_obj.is_column(in[1])==false) {
      cerr << "Could not find column named '" << in[1] << "'." << endl;
      return exc_efailed;
    }
    
    table_obj.deriv(in[0],in[1],in[2]);
    
  } else if (type=="double[]") {
    
    std::vector<double> vderiv(doublev_obj.size());
    o2scl::vector_deriv_interp(doublev_obj.size(),doublev_obj,vderiv,
			       interp_type);
    doublev_obj=vderiv;
    
  } else if (type=="int[]") {

    doublev_obj.resize(intv_obj.size());
    o2scl::vector_copy(intv_obj,doublev_obj);
    std::vector<double> vderiv(doublev_obj.size());
    o2scl::vector_deriv_interp(doublev_obj.size(),doublev_obj,vderiv,
			       interp_type);
    o2scl::vector_copy(vderiv,intv_obj);
    doublev_obj.resize(0);
    
  } else if (type=="size_t[]") {
    
    doublev_obj.resize(size_tv_obj.size());
    o2scl::vector_copy(size_tv_obj,doublev_obj);
    std::vector<double> vderiv(doublev_obj.size());
    o2scl::vector_deriv_interp(doublev_obj.size(),doublev_obj,vderiv,
			       interp_type);
    o2scl::vector_copy(vderiv,size_tv_obj);
    doublev_obj.resize(0);
    
  } else {
    
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
    
  }

  return 0;
}

int acol_manager::comm_deriv_x(std::vector<std::string> &sv, bool itive_com) {

  if (type!="table3d") {
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }
  
  if (table3d_obj.get_nslices()==0) {
    cerr << "No table3d with slices to take derivatives of." << endl;
    return exc_efailed;
  }
  
  vector<string> pr, in;
  pr.push_back("Enter slice containing function");
  pr.push_back("Enter name of new slice");
  int ret=get_input(sv,pr,in,"deriv",itive_com);
  if (ret!=0) return ret;

  size_t iz;
  if (table3d_obj.is_slice(in[0],iz)==false) {
    cerr << "Could not find slice named '" << in[0] << "'." << endl;
    return exc_efailed;
  }
  
  table3d_obj.deriv_x(in[0],in[1]);

  return 0;
}

int acol_manager::comm_deriv_y(std::vector<std::string> &sv, bool itive_com) {

  if (type!="table3d") {
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }
  
  if (table3d_obj.get_nslices()==0) {
    cerr << "No table3d with slices to take derivatives of." << endl;
    return exc_efailed;
  }
  
  vector<string> pr, in;
  pr.push_back("Enter slice containing function");
  pr.push_back("Enter name of new slice");
  int ret=get_input(sv,pr,in,"deriv",itive_com);
  if (ret!=0) return ret;
  
  size_t iz;
  if (table3d_obj.is_slice(in[0],iz)==false) {
    cerr << "Could not find slice named '" << in[0] << "'." << endl;
    return exc_efailed;
  }
  
  table3d_obj.deriv_y(in[0],in[1]);

  return 0;
}

int acol_manager::comm_deriv2(std::vector<std::string> &sv, bool itive_com) {

  if (type=="table") {
    
    if (table_obj.get_nlines()==0) {
      cout << "No table with columns to take derivatives of." << endl;
      return exc_efailed;
    }
    vector<string> pr, in;
    pr.push_back("Enter 'x' column");
    pr.push_back("Enter 'y' column");
    pr.push_back("Enter name of new column");
    int ret=get_input(sv,pr,in,"deriv2",itive_com);
    if (ret!=0) return ret;
    
    if (table_obj.is_column(in[0])==false) {
      cerr << "Could not find column named '" << in[0] << "'." << endl;
      return exc_efailed;
    }
    if (table_obj.is_column(in[1])==false) {
      cerr << "Could not find column named '" << in[1] << "'." << endl;
      return exc_efailed;
    }
    
    table_obj.deriv2(in[0],in[1],in[2]);
    
  } else {
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }    

  return 0;
}

int acol_manager::comm_delete_rows(std::vector<std::string> &sv, 
				   bool itive_com) {

  if (type!="table") {
    cout << "Not implemented for type " << type << endl;
    return 0;
  }

  if (table_obj.get_nlines()==0) {
    cerr << "No table to delete rows from." << endl;
    return exc_efailed;
  }

  std::string i1;
  int ret=get_input_one(sv,"Function to specify rows",
			i1,"delete-rows",itive_com);
  if (ret!=0) return ret;
  
  table_obj.delete_rows_func(i1);

  return 0;
}

int acol_manager::comm_delete_rows_tol(std::vector<std::string> &sv, 
				       bool itive_com) {

  if (type!="table") {
    cout << "Not implemented for type " << type << endl;
    return 0;
  }

  if (table_obj.get_nlines()==0) {
    cerr << "No table to delete rows from." << endl;
    return exc_efailed;
  }

  double tr, ta;
  if (itive_com || sv.size()>=3) {
    vector<string> in, pr;
    pr.push_back("Relative tolerance");
    pr.push_back("Absolute tolerance");
    int ret=get_input(sv,pr,in,"to-hist",itive_com);
    if (ret!=0) return ret;
    if (o2scl::stod_nothrow(in[0],tr)!=0) {
      cerr << "Failed to convert " << in[0] << " to number." << endl;
      return 1;
    }
    if (o2scl::stod_nothrow(in[1],ta)!=0) {
      cerr << "Failed to convert " << in[1] << " to number." << endl;
      return 2;
    }
  } else if (sv.size()>=2) {
    if (o2scl::stod_nothrow(sv[1],tr)!=0) {
      cerr << "Failed to convert " << sv[1] << " to number." << endl;
      return 3;
    }
    ta=1.0e-20;
  } else {
    tr=1.0e-12;
    ta=1.0e-20;
  }
  
  table_obj.delete_rows_tolerance(tr,ta,verbose);

  return 0;
}

int acol_manager::comm_delete_col(std::vector<std::string> &sv, 
				  bool itive_com) {

  if (type!="table") {
    cout << "Not implemented for type " << type << endl;
    return 0;
  }

  if (table_obj.get_nlines()==0) {
    cerr << "No table to delete columns from." << endl;
    return exc_efailed;
  }

  std::string i1;
  int ret=get_input_one(sv,"Column to delete",i1,"delete-col",
			itive_com);
  if (ret!=0) return ret;
    
  if (table_obj.is_column(i1)==false) {
    cerr << "Could not find column named '" << i1 << "'." << endl;
    return exc_efailed;
  }

  table_obj.delete_column(i1);

  return 0;
}

int acol_manager::comm_entry(std::vector<std::string> &sv, bool itive_com) {

  if (type=="table") {

    vector<string> pr, in;
    pr.push_back("Enter column name");
    pr.push_back("Enter row index");
    int ret=get_input(sv,pr,in,"function",itive_com);
    if (ret!=0) return ret;

    int row;
    int ret2=o2scl::stoi_nothrow(in[1],row);
    if (ret2!=0) {
      std::cerr << "Failed to convert " << in[1]
		<< " to a number." << endl;
      return exc_efailed;
    }
    if (row<0) {
      std::cerr << "Conversion of " << in[1]
		<< " resulted in, " << row << ", a negative number." << endl;
      return exc_efailed;
    }
    
    cout << "Entry for column " << in[0] << " at row " << in[1] << " is "
	 << table_obj.get(in[0],row) << endl;
    
  } else {
    cerr << "Command 'entry' not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }

  return 0;
}

int acol_manager::comm_function(std::vector<std::string> &sv, bool itive_com) {

  if (type=="table3d") {
    
    vector<string> pr, in;
    pr.push_back("Enter function for new slice");
    pr.push_back("Enter name for new slice");
    int ret=get_input(sv,pr,in,"function",itive_com);
    if (ret!=0) return ret;
    
    // Remove single or double quotes just in case
    if (in[0].size()>=3 && ((in[0][0]=='\'' && in[0][in[0].size()-1]=='\'') ||
			    (in[0][0]=='\"' && in[0][in[0].size()-1]=='\"'))) {
      in[0]=in[0].substr(1,in[0].size()-2);
    }
    
    table3d_obj.function_slice(in[0],in[1]);
    if (ret!=0) {
      cerr << "Function make_slice() failed." << endl;
      return exc_efailed;
    }
    
    return 0;

  } else if (type=="table") {
    
    if (table_obj.get_nlines()==0) {
      cerr << "No table to add a column to." << endl;
      return exc_efailed;
    }
    
    vector<string> pr, in;
    pr.push_back("Enter function for new column");
    pr.push_back("Enter name for new column");
    int ret=get_input(sv,pr,in,"function",itive_com);
    if (ret!=0) return ret;
    
    // Remove single or double quotes just in case
    if (in[0].size()>=3 && ((in[0][0]=='\'' && in[0][in[0].size()-1]=='\'') ||
			    (in[0][0]=='\"' && in[0][in[0].size()-1]=='\"'))) {
      in[0]=in[0].substr(1,in[0].size()-2);
    }
    
    table_obj.function_column(in[0],in[1]);

  } else if (type=="double[]") {

    std::string function;
    int ret=get_input_one(sv,"Enter function of index 'i'",
			  function,"function",itive_com);
    if (ret!=0) return ret;

    // Parse function
    calculator calc;
    std::map<std::string,double> vars;
    calc.compile(function.c_str(),&vars);

    // Create column from function
    for(size_t j=0;j<doublev_obj.size();j++) {
      vars["i"]=((double)j);
      doublev_obj[j]=calc.eval(&vars);
    }
    
  } else if (type=="int[]") {

    std::string function;
    int ret=get_input_one(sv,"Enter function of index 'i'",
			  function,"function",itive_com);
    if (ret!=0) return ret;

    // Parse function
    calculator calc;
    std::map<std::string,double> vars;
    calc.compile(function.c_str(),&vars);

    // Create column from function
    for(size_t j=0;j<intv_obj.size();j++) {
      vars["i"]=((double)j);
      intv_obj[j]=(int)calc.eval(&vars);
    }
    
  } else if (type=="size_t[]") {

    std::string function;
    int ret=get_input_one(sv,"Enter function of index 'i'",
			  function,"function",itive_com);
    if (ret!=0) return ret;

    // Parse function
    calculator calc;
    std::map<std::string,double> vars;
    calc.compile(function.c_str(),&vars);

    // Create column from function
    for(size_t j=0;j<size_tv_obj.size();j++) {
      vars["i"]=((double)j);
      size_tv_obj[j]=(size_t)calc.eval(&vars);
    }
    
  } else {
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }

  return 0;
}

int acol_manager::comm_filelist(std::vector<std::string> &sv, 
				bool itive_com) {

  std::string i1;
  int ret=get_input_one(sv,"Enter filename",i1,"filelist",
			itive_com);
  if (ret!=0) return ret;

  // Use hdf_file to open the file
  hdf_file hf;
  int hfret=hf.open(i1.c_str(),false,false);
  if (hfret!=0) {
    cerr << "Failed to read file named " << i1.c_str() << endl;
    return exc_efailed;
  }

  // Set the proper output precision and mode
  if (scientific) cout.setf(ios::scientific);
  else cout.unsetf(ios::scientific);
  cout.precision(prec);

  hf.file_list(verbose);
  
  return 0;
}

int acol_manager::comm_find_row(std::vector<std::string> &sv, bool itive_com) {

  if (type!="table") {
    cerr << "Not implemented for type " << type << " ." << endl;
    return exc_efailed;
  }

  if (table_obj.get_nlines()==0 || table_obj.get_nlines()==0) {
    cerr << "No table or empty table in find-row." << endl;
    return exc_efailed;
  }

  // If they didn't specify any parameters
  if (sv.size()==1) {
    
    string stmp=cl->cli_gets
      ("Enter function to be maximized or column name and value: ");
    vector<string> in;
    split_string(stmp,in);
    if (in.size()==2) {
      sv.push_back(in[0]);
      sv.push_back(in[1]);
    } else if (in.size()==1) {
      sv.push_back(in[0]);
    } else {
      cerr << "Gave three parameters but expected either one or two "
	   << "in find-row. Aborting." << endl;
      return exc_efailed;
    }
  
  } 
  
  if (sv.size()>=3) {
    
    // If they specified two parameters, then it is presumed that they
    // specified a column and a value
    
    if (table_obj.is_column(sv[1])) {
      size_t row=table_obj.lookup(sv[1],o2scl::function_to_double(sv[2]));
      
      // Call get_row() for the row that was found
      std::vector<std::string> sc;
      sc.push_back("-get-row");
      sc.push_back(itos(row));
      comm_get_row(sc,itive_com);
    }

    return 0;
  }

  // Otherwise, they specified a function to be maximized.

  // Find the appropriate row
  size_t row=table_obj.function_find_row(sv[1]);
  
  // Call get_row() for the row that was found
  std::vector<std::string> sc;
  sc.push_back("get-row");
  sc.push_back(itos(row));
  comm_get_row(sc,itive_com);
  
  return 0;
}

int acol_manager::comm_fit(std::vector<std::string> &sv, bool itive_com) {

  cout << "Not implemented." << endl;
  return 0;

#ifdef O2SCL_NEVER_DEFINED
  if (type=="table3d") {
    cout << "Not implemented for table3d." << endl;
    return 0;
  }

  if (table_obj.get_nlines()==0) {
    cerr << "No table with data to fit." << endl;
    return exc_efailed;
  }

  const size_t nargs=7;
  std::string in[nargs], pr[nargs]=
    {"Enter column name of independent variable (or blank to stop): ",
     "Enter column name of dependent variable (or blank to stop): ",
     "Enter column name of errors on dependent variable (or blank to stop): ",
     "Enter column name of fitted values (or blank to stop): ",
     "Enter comma-delimited parameter name list (or blank to stop): ",
     "Enter function to fit (or blank to stop): ",
     "Enter space-delimited list of initial values (or blank to stop): "};
  if (sv.size()>=nargs) {
    in[0]=sv[1];
    in[1]=sv[2];
    in[2]=sv[3];
    in[3]=sv[4];
    in[4]=sv[5];
    in[5]=sv[6];
    in[6]=sv[7];
  } else {
    if (itive_com) {
      for(size_t is=0;is<nargs;is++) {
	in[is]=cl->cli_gets(pr[is].c_str());
	if (in[is].length()==0) {
	  cout << "Command 'fit' cancelled." << endl;
	  return 0;
	}
      }
    } else {
      cerr << "Not enough arguments to 'fit'" << endl;
      return exc_efailed;
    }
  }
  
  // Create data to fit to
  size_t ndat=table_obj.get_nlines();
  ubvector xdat(ndat), ydat(ndat), yerr(ndat);
  for(size_t i=0;i<ndat;i++) {
    xdat[i]=table_obj.get(in[0],i);
    ydat[i]=table_obj.get(in[1],i);
    yerr[i]=table_obj.get(in[2],i);
  }
  if (verbose>=2) {
    cout << "Data summary:" << endl;
    for(size_t i=0;i<ndat;i+=ndat/10) {
      cout.width(4);
      cout << i << " ";
      cout << xdat[i] << " " << ydat[i] << " " << yerr[i] << endl;
    }
  }

  // Parse initial parameter values
  std::vector<std::string> param_list;
  split_string(in[6],param_list);
  size_t n_parms=param_list.size();
  ubvector params(n_parms);
  for(size_t k=0;k<n_parms;k++) {
    params[k]=o2scl::stod(param_list[k]);
  }
  if (verbose>=1) {
    cout << "Initial parameters: ";
    for(size_t k=0;k<n_parms;k++) {
      cout << params[k] << " ";
    }
    cout << endl;
    cout << "Function: " << in[5] << endl;
  }

  // Set up fitting function
  fit_funct_strings ffs(in[5],in[4],in[0]);
  ffs.set_aux_parms(params);

  // Fitting function object
  chi_fit_funct<ubvector,ubmatrix,fit_funct_strings<> > 
    cff(ndat,xdat,ydat,yerr,ffs);
  
  // Perform fit
  double chi2;
  ubmatrix covar(n_parms,n_parms);
  fit_nonlin<> gf;
  gf.err_nonconv=false;
  int ret=gf.fit(n_parms,params,covar,chi2,cff);

  if (ret!=0) {
    cout << "Fit failed." << endl;
    return exc_einval;
  }

  // Create and fill the new fitted value column
  table_obj.new_column(in[3]);
  for(int k=0;k<((int)table_obj.get_nlines());k++) {
    table_obj.set(in[3],k,ffs(n_parms,params,(xdat)[k]));
  }
  
  // Output results to cout
  cout << "\nFit results: " << endl;
  for(int k=0;k<((int)n_parms);k++) {
    cout << "Parameter " << k+1 << ": ";
    if (params[k]<0.0) cout << params[k] << endl;
    else cout << " " << params[k] << endl;
  }
  cout << "Covariance matrix:" << endl;
  matrix_out(cout,n_parms,n_parms,covar);
  cout << "Chi-squared: " << chi2 << endl;
  cout << endl;
  
#endif
  
  return 0;
}
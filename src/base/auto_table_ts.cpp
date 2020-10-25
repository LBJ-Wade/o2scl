/*
  -------------------------------------------------------------------
  
  Copyright (C) 2020, Andrew W. Steiner
  
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
#include <o2scl/auto_table.h>
#include <o2scl/test_mgr.h>

using namespace std;
using namespace o2scl;
using namespace o2scl_auto_table;

int main(void) {

  test_mgr t;
  t.set_output_level(2);

  auto_table at;
  double d=3.0;
  //at << d << o2scl_auto_table::endl << d*d << o2scl_auto_table::endl;
  at << d << 1.0e-200 << endo;
  at << 2*d << 2*d*d << endo;
  at << 4*d << 4*d*d << endo;
  at.done();
  
  t.report();
  return 0;
}


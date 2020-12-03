/* Main routine to run a piecewise Hermite finite-difference method
   for solution of a second-order, scalar-valued BVP:

      u'' = p(t)*u' + q(t)*u + r(t),  a<t<b,
      u(a) = ua,  u(b) = ub

   where the problem has stiffness that may be adjusted using
   the real-valued parameter lambda<0 [read from the command line]

   D.R. Reynolds
   Math 6321 @ SMU
   Fall 2020  */

#include <iostream>
#include <iomanip>
#include <vector>
#include <armadillo>
#include "bvp.hpp"

using namespace std;
using namespace arma;

// utility routine to map from physical/component space to linear algebra index space
//    interval:  physical interval index [1 <= interval <= N]
//    location:  location in interval [0=left, 1=right]
//    component: solution component at this location [0=u, 1=u']
int idx(int interval, int location, int component) {
  return ( 2*(interval-1) + 2*location + component );
}

// utility routines for Hermite basis functions (and corresponding derivatives)
double phi1(double tleft, double h, double t) {
  return (2.0*pow((t-tleft)/h,3) - 3.0*pow((t-tleft)/h,2) + 1.0);
}
double dphi1(double tleft, double h, double t) {
  return (6.0/h*pow((t-tleft)/h,2) - 6.0*(t-tleft)/h/h);
}
double ddphi1(double tleft, double h, double t) {
  return (12.0*(t-tleft)/h/h/h - 6.0/h/h);
}
double phi2(double tleft, double h, double t) {
  return (h*pow((t-tleft)/h,3) - 2.0*h*pow((t-tleft)/h,2) + (t-tleft));
}
double dphi2(double tleft, double h, double t) {
  return (3.0*pow((t-tleft)/h,2) - 4.0*(t-tleft)/h + 1.0);
}
double ddphi2(double tleft, double h, double t) {
  return (6.0*(t-tleft)/h/h - 4.0/h);
}
double phi3(double tleft, double h, double t) {
  return (-2.0*pow((t-tleft)/h,3) + 3.0*pow((t-tleft)/h,2));
}
double dphi3(double tleft, double h, double t) {
  return (-6.0/h*pow((t-tleft)/h,2) + 6.0*(t-tleft)/h/h);
}
double ddphi3(double tleft, double h, double t) {
  return (-12.0*(t-tleft)/h/h/h + 6.0/h/h);
}
double phi4(double tleft, double h, double t) {
  return (h*pow((t-tleft)/h,3) - h*pow((t-tleft)/h,2));
}
double dphi4(double tleft, double h, double t) {
  return (3.0*pow((t-tleft)/h,2) - 2.0*(t-tleft)/h);
}
double ddphi4(double tleft, double h, double t) {
  return (6.0*(t-tleft)/h/h - 2.0/h);
}


// main routine
int main(int argc, char **argv) {

  // get lambda from the command line, otherwise set to -10
  double lambda;
  if (argc > 1)
    lambda = atof(argv[1]);
  if (lambda >= 0.0)
    lambda = -10.0;

  // define BVP object
  BVP bvp(lambda);

  // test 'idx' function by outputting mapping for small N
  cout << "Test output from 'idx' function for N = 5, M = 12:";
  for (int interval=1; interval<=5; interval++) {
    cout << "\n  interval " << interval << ", (loc,comp,idx):";
    for (int location=0; location<=1; location++) {
      for (int component=0; component<=1; component++) {
        cout << "  (" << location << ", " << component << ", "
             << idx(interval,location,component) << ")";
      }
    }
  }

  // test basis functions
  //   check {1,0} properties with 'random' tleft=0.5, h=0.1, tright=0.6
  cout << "\n\nOsculatory interpolation tests:\n\n";
  cout << "    phi1(tleft)   = " << phi1(0.5,0.1,0.5) << endl;
  cout << "    phi1(tright)  = " << phi1(0.5,0.1,0.6) << endl;
  cout << "    phi1'(tleft)  = " << 1.e8*(phi1(0.5,0.1,0.5+1.e-8)-phi1(0.5,0.1,0.5)) << endl;
  cout << "    phi1'(tright) = " << 1.e8*(phi1(0.5,0.1,0.6+1.e-8)-phi1(0.5,0.1,0.6)) << endl << endl;
  cout << "    phi2(tleft)   = " << phi2(0.5,0.1,0.5) << endl;
  cout << "    phi2(tright)  = " << phi2(0.5,0.1,0.6) << endl;
  cout << "    phi2'(tleft)  = " << 1.e8*(phi2(0.5,0.1,0.5+1.e-8)-phi2(0.5,0.1,0.5)) << endl;
  cout << "    phi2'(tright) = " << 1.e8*(phi2(0.5,0.1,0.6+1.e-8)-phi2(0.5,0.1,0.6)) << endl << endl;
  cout << "    phi3(tleft)   = " << phi3(0.5,0.1,0.5) << endl;
  cout << "    phi3(tright)  = " << phi3(0.5,0.1,0.6) << endl;
  cout << "    phi3'(tleft)  = " << 1.e8*(phi3(0.5,0.1,0.5+1.e-8)-phi3(0.5,0.1,0.5)) << endl;
  cout << "    phi3'(tright) = " << 1.e8*(phi3(0.5,0.1,0.6+1.e-8)-phi3(0.5,0.1,0.6)) << endl << endl;
  cout << "    phi4(tleft)   = " << phi4(0.5,0.1,0.5) << endl;
  cout << "    phi4(tright)  = " << phi4(0.5,0.1,0.6) << endl;
  cout << "    phi4'(tleft)  = " << 1.e8*(phi4(0.5,0.1,0.5+1.e-8)-phi4(0.5,0.1,0.5)) << endl;
  cout << "    phi4'(tright) = " << 1.e8*(phi4(0.5,0.1,0.6+1.e-8)-phi4(0.5,0.1,0.6)) << endl << endl;
  //   check analytical derivative tests with 'random' tleft=0.5, h=0.1, t=0.53
  cout << "Derivative tests:\n";
  double dtest;
  bool failed = false;
  dtest = 1.e8*(phi1(0.5,0.1,0.53+1.e-8)-phi1(0.5,0.1,0.53));
  if (std::abs(dtest - dphi1(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  dphi1 error, value = " << dphi1(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(dphi1(0.5,0.1,0.53+1.e-8)-dphi1(0.5,0.1,0.53));
  if (std::abs(dtest - ddphi1(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  ddphi1 error, value = " << ddphi1(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(phi2(0.5,0.1,0.53+1.e-8)-phi2(0.5,0.1,0.53));
  if (std::abs(dtest - dphi2(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  dphi2 error, value = " << dphi2(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(dphi2(0.5,0.1,0.53+1.e-8)-dphi2(0.5,0.1,0.53));
  if (std::abs(dtest - ddphi2(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  ddphi2 error, value = " << ddphi2(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(phi3(0.5,0.1,0.53+1.e-8)-phi3(0.5,0.1,0.53));
  if (std::abs(dtest - dphi3(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  dphi3 error, value = " << dphi3(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(dphi3(0.5,0.1,0.53+1.e-8)-dphi3(0.5,0.1,0.53));
  if (std::abs(dtest - ddphi3(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  ddphi3 error, value = " << ddphi3(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(phi4(0.5,0.1,0.53+1.e-8)-phi4(0.5,0.1,0.53));
  if (std::abs(dtest - dphi4(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  dphi4 error, value = " << dphi4(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  dtest = 1.e8*(dphi4(0.5,0.1,0.53+1.e-8)-dphi4(0.5,0.1,0.53));
  if (std::abs(dtest - ddphi4(0.5,0.1,0.53)) > 1e-4) {
    failed = true;
    cout << "  ddphi4 error, value = " << ddphi4(0.5,0.1,0.53) << ", approx = " << dtest << endl;
  }
  if (!failed) cout << "  all tests pass\n";


  // loop over spatial resolutions for tests
  vector<int> N = {100, 1000, 10000};
  for (int i=0; i<N.size(); i++) {

    // output problem information
    cout << "\nPiecewise Hermite FD method for BVP with lambda = "
         << lambda << ":" << ",  N = " << N[i] << endl;

    // compute/store analytical solution
    vec t = zeros(N[i]+1);
    t(0) = 0.0;
    t(N[i]) = 1.0;
    for (int j=1; j<=N[i]-1; j++)
      t(j) = 0.5*(1.0-cos((2*j-1)*M_PI/(2*(N[i]-1))));
    vec utrue(N[i]+1);
    for (int j=0; j<=N[i]; j++)
      utrue(j) = bvp.utrue(t(j));

    // set integer for overall linear algebra problem size
    int M = 2*N[i]+2;

    // create matrix and right-hand side vectors
#ifdef USE_SPARSE
    sp_mat A(M,M);
#else
    mat A(M,M);
    A.fill(0.0);
#endif
    vec b(M);
    b.fill(0.0);

    //*** BEGIN UPDATING FROM HERE DOWNWARD ***//

    // set up linear system:
    //   note: y = [y_{0,1} y_{0,2} y_{1,1} y_{1,2} ... y_{N,1} y_{N,2}]
    A(0,0) = 1.0;
    b(0) = bvp.ua;
    A(1,2*N[i]) = 1.0;
    b(1) = bvp.ub;
    for (int j=1; j<=N[i]; j++) {

      // setup interval-specific information
      double h = t(j)-t(j-1);
      double thalf = 0.5*(t(j)+t(j-1));
      double alpha = -h*bvp.q(thalf);
      double beta = h*bvp.p(thalf);
      double gamma = 2.0*h*bvp.r(thalf);

      // setup eqn in row 2*j:
      //    2*y_{j,1} - 2*y_{j-1,1} - h*y_{j,2} - h*y_{j-1,2} = 0
      A(2*j,2*j) = 2.0;
      A(2*j,2*j-2) = -2.0;
      A(2*j,2*j+1) = -h;
      A(2*j,2*j-1) = -h;
      b(2*j) = 0.0;

      // setup eqn in row 2*j+1:
      //     alpha*y_{j,1} + alpha*y_{j-1,1} + (2-beta)*y_{j,2} - (2+beta)*y_{j-1,2} = gamma_j
      A(2*j+1,2*j) = alpha;
      A(2*j+1,2*j-2) = alpha;
      A(2*j+1,2*j+1) = 2.0-beta;
      A(2*j+1,2*j-1) = -(2.0+beta);
      b(2*j+1) = gamma;
    }

    // solve linear system for BVP solution
#ifdef USE_SPARSE
    vec y = arma::spsolve(A,b);
#else
    vec y = arma::solve(A,b);
#endif

    // output maximum error
    vec u(N[i]+1);
    for (int j=0; j<=N[i]; j++)
       u(j) = y(2*j);
    vec uerr = abs(u-utrue);
    cout << "  Maximum BVP solution error = " << std::setprecision(4)
       << uerr.max() << "\n";
  }

  return 0;
}

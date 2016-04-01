/*
 * constraints.cc
 *
 *  Created on: Mar 25, 2016
 *      Author: winklera
 */

#include <xpp/zmp/constraints.h>

namespace xpp {
namespace zmp {

Constraints::Constraints (const xpp::hyq::SupportPolygonContainer& supp_triangle_container,
                           const xpp::zmp::ContinuousSplineContainer& zmp_spline_container,
                           const MatVec& qp_equality_constraints)
    :planned_footholds_(supp_triangle_container.footholds_)
{
  supp_polygon_container_ = supp_triangle_container;
  zmp_spline_container_    = zmp_spline_container;

  spline_junction_constraints_ = qp_equality_constraints;

  // inequality constraints
  double walking_height = 0.58;
  x_zmp_ = zmp_spline_container.ExpressZmpThroughCoefficients(walking_height, xpp::utils::X);
  y_zmp_ = zmp_spline_container.ExpressZmpThroughCoefficients(walking_height, xpp::utils::Y);

  first_constraint_eval_ = true;
}





Eigen::VectorXd
Constraints::EvalContraints(const Eigen::VectorXd& x_coeff, const Footholds& footholds)
{
  std::vector<Eigen::VectorXd> g_std;

  g_std.push_back(SmoothAccJerkAtSplineJunctions(x_coeff));
  g_std.push_back(KeepZmpInSuppPolygon(x_coeff, footholds));
//  g_std.push_back(FixFootholdPosition(footholds));
  g_std.push_back(RestrictMaxStepLength(footholds));
//  g_vec.push_back(RestrictFootholdToCogPos(x_coeff, footholds));


  CombineToEigenVector(g_std, g_);


  first_constraint_eval_ = false;
  return g_;
}


Eigen::VectorXd
Constraints::KeepZmpInSuppPolygon(const Eigen::VectorXd& x_coeff, const Footholds& footholds)
{
  for (uint i=0; i<footholds.size(); ++i) {
    supp_polygon_container_.footholds_.at(i).p.x() = footholds.at(i).x();
    supp_polygon_container_.footholds_.at(i).p.y() = footholds.at(i).y();
  }


  MatVec ineq = supp_polygon_container_.AddLineConstraints(x_zmp_, y_zmp_, zmp_spline_container_);


  AddBounds(ineq.v.rows(), 0.0, +1.0e19);

  return ineq.M*x_coeff + ineq.v;
}


Eigen::VectorXd
Constraints::FixFootholdPosition(const Footholds& footholds)
{
  // constraints on the footsteps
  std::vector<int> fixed_dim = {xpp::utils::X, xpp::utils::Y};
  Eigen::VectorXd g(fixed_dim.size()*footholds.size());
  int c=0;

  for (uint i=0; i<footholds.size(); ++i) {
    xpp::hyq::Foothold f = planned_footholds_.at(i);

    // fix footholds in x and y direction
    for (int dim : fixed_dim)
      g(c++) = footholds.at(i)(dim) - f.p(dim);
  }


  AddBounds(g.rows(), 0.0, 0.0);

  return g;
}


Eigen::VectorXd
Constraints::RestrictMaxStepLength(const Footholds& footholds)
{
  Eigen::VectorXd g(footholds.size());
  int c=0;

  for (uint i=0; i<footholds.size(); ++i)
  {
    xpp::hyq::LegID leg = planned_footholds_.at(i).leg; // leg sequence stays the same as initial
    Eigen::Vector2d f_prev = supp_polygon_container_.start_stance_[leg].p.segment<2>(0);
    if (i>=4) {
      f_prev = footholds.at(i-4); // FIXME: only works for repeating same step sequence
    }

    double dx = footholds.at(i).x() - f_prev.x();
    double dy = footholds.at(i).y() - f_prev.y();

    g(c++) = hypot(dx,dy);
    // this seems to converge better than the combination of both
//    g(c++) = dx*dx;
//    g(c++) = dy*dy;
  }


  // add bounds that step length should never exceed max step length
  double max_step_length = 0.3;
  AddBounds(g.rows(), 0.0, max_step_length);

  return g;
}


Eigen::VectorXd
Constraints::RestrictFootholdToCogPos(const Eigen::VectorXd& x_coeff,
                                         const Footholds& footholds)
{
  int n_footholds = footholds.size();
  Eigen::VectorXd g(n_footholds);
  int c=0;

//   get position of cog through xpline coefficients
  zmp_spline_container_.AddOptimizedCoefficients(x_coeff);


  const double dt = zmp_spline_container_.GetTotalTime()/n_footholds;
  double t_global = 0.0;
  for (int i=0; i<g.rows(); ++i) {

    // time during which this foothold is in contact
    // equivalent to global start and end time of the corresponding spline
//    if (zmp_spline_container_.splines_.at(i).four_leg_supp_

    xpp::utils::Point2d cog_xy;

    zmp_spline_container_.GetCOGxy(t_global, cog_xy);
    t_global += dt; //s


    // calculate distance cog to foothold for each leg and time
    double dx = footholds.at(i).x() - cog_xy.p.x();
    double dy = footholds.at(i).y() - cog_xy.p.y();

    g(c++) = hypot(dx,dy);
  }


  // add bounds that step length should never exceed max step length
  double max_range = 0.3;
  AddBounds(g.rows(), 0.0, max_range);

  return g;
}


Eigen::VectorXd
Constraints::SmoothAccJerkAtSplineJunctions(const Eigen::VectorXd& x_coeff)
{
  Eigen::VectorXd g = spline_junction_constraints_.M*x_coeff + spline_junction_constraints_.v;
  AddBounds(g.rows(), 0.0, 0.0);
  return g;
}


void Constraints::AddBounds(int m_constraints, double lower, double upper)
{
  if (first_constraint_eval_) {
    Bound eq_bound(lower, upper);
    for (int c=0; c<m_constraints; ++c) {
      bounds_.push_back(eq_bound);
    }
  }
}


void
Constraints::CombineToEigenVector(const std::vector<Eigen::VectorXd>& g_std, Eigen::VectorXd& g_eig) const
{
  // create correct size constraint vector the first time this function is called
  if (first_constraint_eval_) {
    int n_constraints = 0;
    for (const Eigen::VectorXd& g : g_std) {
      n_constraints += g.rows();
    }
    g_eig.resize(n_constraints);
  }

  //  combine all the g vectors
  //  g_ << g_vec[0], g_vec[1], g_vec[2];
  int c = 0;
  for (const Eigen::VectorXd& g : g_std) {
    g_eig.middleRows(c, g.rows()) = g; //g.normalized()
    c += g.rows();
  }
}


} /* namespace zmp */
} /* namespace xpp */

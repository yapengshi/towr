/**
 @file    optimization_visualizer.h
 @author  Alexander W. Winkler (winklera@ethz.ch)
 @date    May 31, 2016
 @brief   Defines a class that visualizes the optimization values using ROS.
 */

#ifndef USER_TASK_DEPENDS_XPP_OPT_INCLUDE_XPP_ROS_OPTIMIZATION_VISUALIZER_H_
#define USER_TASK_DEPENDS_XPP_OPT_INCLUDE_XPP_ROS_OPTIMIZATION_VISUALIZER_H_

#include <xpp/zmp/i_observer.h>
#include <xpp/ros/marker_array_builder.h>

#include <xpp/zmp/optimization_variables.h>

#include <xpp/zmp/continuous_spline_container.h>

#include <ros/ros.h>

namespace xpp {
namespace ros {

/** @brief Visualizes the current values of the optimization variables.
  *
  * This class is responsible for getting the current values of the optimization
  * variables and generating ROS messages for rviz to visualize. It delegates
  * the actual generation of these messages to a specific member.
  */
class OptimizationVisualizer : public xpp::zmp::IObserver {
public:
  typedef xpp::zmp::OptimizationVariables OptimizationVariables;
  typedef xpp::zmp::ContinuousSplineContainer ContinuousSplineContainer;
  typedef OptimizationVariables::StdVecEigen2d StdVecEigen2d;
  typedef Eigen::VectorXd VectorXd;
  typedef xpp::hyq::LegID LegID;

  OptimizationVisualizer (OptimizationVariables& subject);
  virtual ~OptimizationVisualizer () {}

  void Init(const std::vector<LegID>& swing_leg_sequence,
            const ContinuousSplineContainer&);

  /** @brief Updates the values of the optimization variables. */
   void Update() override;




private:
  void PublishMsg();
  ::ros::Publisher ros_publisher_;

  MarkerArrayBuilder msg_builder_;
  OptimizationVariables& subject_;

  // optimization variables
  VectorXd x_coeff_;
  StdVecEigen2d footholds_xy_;

  ContinuousSplineContainer splines_;
  std::vector<LegID> leg_ids_; ///< these are predefined, not part of optimization
};

} /* namespace ros */
} /* namespace xpp */

#endif /* USER_TASK_DEPENDS_XPP_OPT_INCLUDE_XPP_ROS_OPTIMIZATION_VISUALIZER_H_ */
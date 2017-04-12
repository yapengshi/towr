/**
@file    motion_type.h
@author  Alexander W. Winkler (winklera@ethz.ch)
@date    Jan 11, 2017
@brief   Brief description
 */

#ifndef XPP_XPP_OPT_INCLUDE_XPP_OPT_MOTION_TYPE_H_
#define XPP_XPP_OPT_INCLUDE_XPP_OPT_MOTION_TYPE_H_

#include <array>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <Eigen/Dense>

#include <xpp/endeffectors.h>

namespace xpp {
namespace opt {

enum MotionTypeID    { WalkID, TrotID, PaceID, BoundID, PushRecID };
enum CostName        { ComCostID, RangOfMotionCostID, PolyCenterCostID,
                       FinalComCostID, FinalStanceCostID };
enum ConstraintName  { InitCom, FinalCom, JunctionCom, Convexity,
                       Dynamic, RomBox, Stance, Obstacle };

/** This class holds all the hardcoded values describing a motion.
  * This is specific to the robot and the type of motion desired.
  */
class MotionParameters {
public:
  using MotionTypePtr    = std::shared_ptr<MotionParameters>;
  using EEID             = EndeffectorID;
  using EEVec            = std::vector<EEID>;
  using ContactSequence  = std::vector<EndeffectorsBool>;
  using ContactTimings   = std::vector<double>;
  using Phase            = std::pair<EndeffectorsBool, double>;
  using ContactSchedule  = std::vector<Phase>;

  using PosXY            = Eigen::Vector2d;
  using PosXYZ           = Eigen::Vector3d;
  using NominalStance    = EndeffectorsPos;
  using ValXY            = std::array<double,2>;
  using CostWeights      = std::map<CostName, double>;
  using UsedConstraints  = std::vector<ConstraintName>;

  virtual ~MotionParameters();

  int GetEECount() const { return robot_ee_.size(); };
  NominalStance GetNominalStanceInBase() const { return nominal_stance_; };
  ContactSchedule GetContactSchedule() const;
  ValXY GetMaximumDeviationFromNominal() const;
  UsedConstraints GetUsedConstraints() const;
  CostWeights GetCostWeights() const;
  double GetTotalTime() const;


  MotionTypeID id_;
  int polynomials_per_second_;
  double geom_walking_height_;
  PosXYZ offset_geom_to_com_; ///< between CoM and geometric center


protected:
  EEVec robot_ee_;
  ValXY max_dev_xy_;
  ContactSequence contact_sequence_;
  NominalStance nominal_stance_;
  ContactTimings contact_timings_;
  UsedConstraints constraints_;
  CostWeights cost_weights_;
};

} // namespace opt
} // namespace hyq

#endif /* XPP_XPP_OPT_INCLUDE_XPP_OPT_MOTION_TYPE_H_ */

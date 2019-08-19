#pragma once
#include <ostream>
#include <variant>

#include "atan.h"
#include "equidist.h"
#include "pinhole.h"
#include "radtan.h"

#include "adjustable_cameras.h"
#include "alias.h"
#include "glog/logging.h"
#include "utils.h"
#include "json/json.h"

namespace feh {

template <typename T> using UnknownCamera = T;

class CameraManager {
public:
  using Unknown = UnknownCamera<ftype>;
  using ATAN = A_ATANCamera;
  using EquiDist = A_EquidistantCamera;
  // using EquiDist = EquidistantCamera<ftype>;
  using RadTan = A_RadialTangentialCamera;
  using Pinhole = A_PinholeCamera;

  static CameraManager *Create(const Json::Value &cfg);
  static CameraManager *instance() { return instance_.get(); }

  // project a point from camera coordinatex xc to pixel coordinates xp.
  // xc: a point in camera coordinates.
  // jac: jacobian matrix dxp/dxc
  // jacc: jacobian matrix of xp w.r.t. camera intrinsics
  template <typename Derived>
  Eigen::Matrix<typename Derived::Scalar, 2, 1> Project(
      const Eigen::MatrixBase<Derived> &xc,
      Eigen::Matrix<typename Derived::Scalar, 2, 2> *jac = nullptr,
      Eigen::Matrix<typename Derived::Scalar, 2, -1> *jacc = nullptr) const {
    if (std::holds_alternative<ATAN>(model_)) {
      return std::get<ATAN>(model_).Project(xc, jac, jacc);
    } else if (std::holds_alternative<EquiDist>(model_)) {
      return std::get<EquiDist>(model_).Project(xc, jac, jacc);
    } else if (std::holds_alternative<RadTan>(model_)) {
      return std::get<RadTan>(model_).Project(xc, jac, jacc);
    } else if (std::holds_alternative<Pinhole>(model_)) {
      return std::get<Pinhole>(model_).Project(xc, jac, jacc);
    } else {
      LOG(FATAL) << "unknown camera model";
    }
  }

  // unproject a point from pixel coordinatex xp to camera coordinates xc.
  // xp: a point in pixel coordinates.
  // jac: jacobian matrix dxc/dxp
  // jacc: jacobian matrix of xc w.r.t. camera intrinsics
  template <typename Derived>
  Eigen::Matrix<typename Derived::Scalar, 2, 1> UnProject(
      const Eigen::MatrixBase<Derived> &xp,
      Eigen::Matrix<typename Derived::Scalar, 2, 2> *jac = nullptr,
      Eigen::Matrix<typename Derived::Scalar, 2, -1> *jacc = nullptr) const {
    if (jacc != nullptr) {
      LOG(FATAL) << "jacobian w.r.t. camera intrinsics (jacc) NOT implemented";
    }

    if (std::holds_alternative<ATAN>(model_)) {
      return std::get<ATAN>(model_).UnProject(xp, jac, jacc);
    } else if (std::holds_alternative<EquiDist>(model_)) {
      return std::get<EquiDist>(model_).UnProject(xp, jac, jacc);
    } else if (std::holds_alternative<RadTan>(model_)) {
      return std::get<RadTan>(model_).UnProject(xp, jac, jacc);
    } else if (std::holds_alternative<Pinhole>(model_)) {
      return std::get<Pinhole>(model_).UnProject(xp, jac, jacc);
    } else {
      LOG(FATAL) << "unknown camera model";
    }
  }

  void Print(std::ostream &out) const {
    if (std::holds_alternative<ATAN>(model_)) {
      std::get<ATAN>(model_).Print(out);
    } else if (std::holds_alternative<EquiDist>(model_)) {
      std::get<EquiDist>(model_).Print(out);
    } else if (std::holds_alternative<RadTan>(model_)) {
      std::get<RadTan>(model_).Print(out);
    } else if (std::holds_alternative<Pinhole>(model_)) {
      std::get<Pinhole>(model_).Print(out);
    } else {
      LOG(FATAL) << "unknown camera model";
    }
  }

  void UpdateState(const VecX &dX) {
    if (std::holds_alternative<ATAN>(model_)) {
      std::get<ATAN>(model_).UpdateState(dX.head<ATAN::DIM>());
    } else if (std::holds_alternative<EquiDist>(model_)) {
      std::get<EquiDist>(model_).UpdateState(dX.head<EquiDist::DIM>());
    } else if (std::holds_alternative<RadTan>(model_)) {
      std::get<RadTan>(model_).UpdateState(dX.head<RadTan::DIM>());
    } else if (std::holds_alternative<Pinhole>(model_)) {
      std::get<Pinhole>(model_).UpdateState(dX.head<Pinhole::DIM>());
    } else {
      LOG(FATAL) << "unknown camera model";
    }
    // also update intrinsics for the camer manager ...
    fx_ += dX(0);
    fy_ += dX(1);
    cx_ += dX(2);
    cy_ += dX(3);
    fl_ = std::sqrt(0.5 * (fx_ * fx_ + fy_ * fy_));
  }

  ftype GetFocalLength() const { return fl_; }
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  ftype fx() const { return fx_; }
  ftype fy() const { return fy_; }
  ftype cx() const { return cx_; }
  ftype cy() const { return cy_; }
  int dim() const { return dim_; }

private:
  CameraManager &operator=(const CameraManager &) = delete;
  CameraManager(const CameraManager &) = delete;

  CameraManager(const Json::Value &cfg);
  static std::unique_ptr<CameraManager> instance_;

  int rows_, cols_;
  ftype fx_, fy_, cx_, cy_;
  ftype fl_; // focal length
  std::variant<Unknown, ATAN, EquiDist, RadTan, Pinhole> model_;
  int dim_; // number of intrinsic parameters
};

} // namespace feh
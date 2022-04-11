/**
 * @file   DynamicalSystem.cpp
 * @brief  DynamicalSystem class source file.
 * @author Freek Stulp
 *
 * This file is part of DmpBbo, a set of libraries and programs for the
 * black-box optimization of dynamical movement primitives.
 * Copyright (C) 2014 Freek Stulp, ENSTA-ParisTech
 *
 * DmpBbo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * DmpBbo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DmpBbo.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dynamicalsystems/DynamicalSystem.hpp"

#include <cmath>
#include <eigen3/Eigen/Core>
#include <iostream>
#include <nlohmann/json.hpp>

#include "dynamicalsystems/ExponentialSystem.hpp"
#include "dynamicalsystems/SigmoidSystem.hpp"
#include "dynamicalsystems/SpringDamperSystem.hpp"
#include "dynamicalsystems/TimeSystem.hpp"
#include "eigenutils/eigen_json.hpp"

using namespace std;
using namespace Eigen;

namespace DmpBbo {

DynamicalSystem::DynamicalSystem(int order, double tau,
                                 Eigen::VectorXd initial_state,
                                 Eigen::VectorXd attractor_state)
    :  // For 1st order systems, the dimensionality of the state vector 'x' is
       // 'dim' For 2nd order systems, the system is expanded to x = [y z],
       // where 'y' and 'z' are both of dimensionality 'dim'. Therefore dim(x)
       // is 2*dim
      dim_(initial_state.size() * order),
      // The dimensionality of the system before a potential rewrite
      dim_orig_(initial_state.size()),
      tau_(tau),
      initial_state_(initial_state),
      attractor_state_(attractor_state),
      integration_method_(RUNGE_KUTTA)
{
  assert(order == 1 || order == 2);
  assert(initial_state.size() == attractor_state.size());
}

DynamicalSystem::~DynamicalSystem(void) {}

void DynamicalSystem::integrateStart(const Eigen::VectorXd& x_init,
                                     Eigen::Ref<Eigen::VectorXd> x,
                                     Eigen::Ref<Eigen::VectorXd> xd)
{
  set_initial_state(x_init);
  integrateStart(x, xd);
}

void DynamicalSystem::integrateStart(Eigen::Ref<Eigen::VectorXd> x,
                                     Eigen::Ref<Eigen::VectorXd> xd) const
{
  // Check size. Leads to faster numerical integration and makes Eigen::Ref
  // easier to deal with
  assert(x.size() == dim());
  assert(xd.size() == dim());

  // Return value for state variables
  // Pad the end with zeros: Why? In the spring-damper system, the state
  // consists of x = [y z]. The initial state only applies to y. Therefore, we
  // set x = [y 0];
  x.fill(0);
  x.segment(0, initial_state_.size()) = initial_state_;

  // Return value (rates of change)
  differentialEquation(x, xd);
}

void DynamicalSystem::integrateStep(
    double dt, const Eigen::Ref<const Eigen::VectorXd> x,
    Eigen::Ref<Eigen::VectorXd> x_updated,
    Eigen::Ref<Eigen::VectorXd> xd_updated) const
{
  assert(dt > 0.0);
  // Check size. Leads to faster numerical integration and makes Eigen::Ref
  // easier to deal with
  assert(x.size() == dim());
  if (integration_method_ == RUNGE_KUTTA)
    integrateStepRungeKutta(dt, x, x_updated, xd_updated);
  else
    integrateStepEuler(dt, x, x_updated, xd_updated);
}

void DynamicalSystem::integrateStepEuler(double dt, const Ref<const VectorXd> x,
                                         Ref<VectorXd> x_updated,
                                         Ref<VectorXd> xd_updated) const
{
  // simple Euler integration
  differentialEquation(x, xd_updated);
  x_updated = x + dt * xd_updated;
}

void DynamicalSystem::integrateStepRungeKutta(double dt,
                                              const Ref<const VectorXd> x,
                                              Ref<VectorXd> x_updated,
                                              Ref<VectorXd> xd_updated) const
{
  // 4th order Runge-Kutta for a 1st order system
  // http://en.wikipedia.org/wiki/Runge-Kutta_method#The_Runge.E2.80.93Kutta_method

  int l = x.size();
  VectorXd k1(l), k2(l), k3(l), k4(l);
  differentialEquation(x, k1);
  VectorXd input_k2 = x + dt * 0.5 * k1;
  differentialEquation(input_k2, k2);
  VectorXd input_k3 = x + dt * 0.5 * k2;
  differentialEquation(input_k3, k3);
  VectorXd input_k4 = x + dt * k3;
  differentialEquation(input_k4, k4);

  x_updated = x + dt * (k1 + 2.0 * (k2 + k3) + k4) / 6.0;
  differentialEquation(x_updated, xd_updated);
}

void from_json(const nlohmann::json& j, DynamicalSystem*& obj)
{
  string class_name = j.at("py/object").get<string>();

  if (class_name.find("ExponentialSystem") != string::npos) {
    obj = j.get<ExponentialSystem*>();

  } else if (class_name.find("SigmoidSystem") != string::npos) {
    obj = j.get<SigmoidSystem*>();

  } else if (class_name.find("SpringDamperSystem") != string::npos) {
    obj = j.get<SpringDamperSystem*>();

  } else if (class_name.find("TimeSystem") != string::npos) {
    obj = j.get<TimeSystem*>();

  } else {
    cerr << __FILE__ << ":" << __LINE__ << ":";
    cerr << "Unknown DynamicalSystem: " << class_name << endl;
    obj = NULL;
  }
}

void DynamicalSystem::to_json_base(nlohmann::json& j) const
{
  j["dim_"] = dim_;
  j["dim_orig_"] = dim_orig_;
  j["tau_"] = tau_;
  j["initial_state_"] = initial_state_;
  j["attractor_state_"] = attractor_state_;
  // Avoiding NLOHMANN_JSON_SERIALIZE_ENUM
  j["integration_method_"] =
      (integration_method_ == EULER) ? "EULER" : "RUNGE_KUTTA";

  string c("DynamicalSystem");
  j["py/object"] = "dynamicalsystems." + c + "." + c;  // for jsonpickle
}

std::ostream& operator<<(std::ostream& output, const DynamicalSystem& d)
{
  nlohmann::json j;
  d.to_json_helper(j);
  // output << j.dump(4);
  output << j.dump();
  return output;
}

}  // namespace DmpBbo

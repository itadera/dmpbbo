/**
 * @file   FunctionApproximatorRBFN.cpp
 * @brief  FunctionApproximatorRBFN class source file.
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

#include "functionapproximators/FunctionApproximatorRBFN.hpp"
#include "functionapproximators/BasisFunction.hpp"

#include "eigen/eigen_json.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <eigen3/Eigen/SVD>
#include <eigen3/Eigen/LU>


using namespace std;
using namespace Eigen;

namespace DmpBbo {

FunctionApproximatorRBFN::FunctionApproximatorRBFN(const Eigen::MatrixXd& centers, const Eigen::MatrixXd& widths, const Eigen::MatrixXd& weights) 
:
  n_basis_functions_(centers.rows()),
  centers_(centers),
  widths_(widths),
  weights_(weights)
{
  assert(n_basis_functions_==widths_.rows());
  assert(n_basis_functions_==weights_.rows());
  assert(centers.cols()==widths_.cols()); // # number of dimensions should match
  assert(weights_.cols()==1);
  
  activations_one_prealloc_ = MatrixXd(1,n_basis_functions_);
};

void FunctionApproximatorRBFN::predict(const Eigen::Ref<const Eigen::MatrixXd>& inputs, MatrixXd& outputs) const
{
  
  int n_time_steps = inputs.rows();
  if (n_time_steps==1) // Only one sample
  {
    ENTERING_REAL_TIME_CRITICAL_CODE
    
    // Get the basis function activations  
    // false, false => normalized_basis_functions, asymmetric_kernels;
    BasisFunction::Gaussian::activations(centers_,widths_,inputs,activations_one_prealloc_,false,false);
      
    // Weight the basis function activations  
    for (int b=0; b<n_basis_functions_; b++)
      activations_one_prealloc_.col(b).array() *= weights_(b);
  
    // Sum over weighed basis functions
    outputs = activations_one_prealloc_.rowwise().sum();
    
    EXITING_REAL_TIME_CRITICAL_CODE
  }
  else 
  {

    // The next line is not be real-time, as it allocates memory.
    MatrixXd activations(n_time_steps,n_basis_functions_);
    
    // Get the basis function activations  
    // false, false => normalized_basis_functions, asymmetric_kernels;
    BasisFunction::Gaussian::activations(centers_,widths_,inputs,activations,false,false);
      
    // Weight the basis function activations  
    for (int b=0; b<n_basis_functions_; b++)
      activations.col(b).array() *= weights_(b);
  
    // Sum over weighed basis functions
    outputs = activations.rowwise().sum();
  }
    
}

FunctionApproximatorRBFN* FunctionApproximatorRBFN::from_jsonpickle(nlohmann::json json) 
{
  nlohmann::json j = json.at("_model_params");
  MatrixXd centers = j.at("centers").at("values");
  MatrixXd widths = j.at("widths").at("values");
  MatrixXd weights = j.at("weights").at("values");  
  return new FunctionApproximatorRBFN(centers,widths,weights);
}

void to_json(nlohmann::json& j, const FunctionApproximatorRBFN& obj) 
{
  j["centers_"] = obj.centers_;
  j["widths_"] = obj.widths_;
  j["weights_"] = obj.weights_;
  j["py/object"] = "dynamicalsystems.ModelParametersRBFN.ModelParametersRBFN"; // for jsonpickle
}

string FunctionApproximatorRBFN::toString(void) const
{
  nlohmann::json j;
  to_json(j,*this);
  return j.dump(4);
}

}

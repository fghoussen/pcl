/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2015, Michael 'v4hn' Goerner
 *  Copyright (c) 2015-, Open Perception, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PCL_REGISTRATION_IMPL_INCREMENTAL_REGISTRATION_HPP_
#define PCL_REGISTRATION_IMPL_INCREMENTAL_REGISTRATION_HPP_

namespace pcl {

namespace registration {

template <typename PointT, typename Scalar>
IncrementalRegistration<PointT, Scalar>::IncrementalRegistration()
: delta_transform_ (Matrix4::Identity()), abs_transform_ (Matrix4::Identity())
{}

template <typename PointT, typename Scalar>
bool
IncrementalRegistration<PointT, Scalar>::registerCloud (const PointCloudConstPtr& cloud,
                                                        const Matrix4& delta_estimate)
{
  assert (registration_);

  if (!last_cloud_) {
    last_cloud_ = cloud;
    abs_transform_ = delta_transform_ = delta_estimate;
    return (true);
  }

  registration_->setInputSource (cloud);
  registration_->setInputTarget (last_cloud_);

  {
    pcl::PointCloud<PointT> p;
    registration_->align (p, delta_estimate);
  }

  bool converged = registration_->hasConverged();

  if (converged) {
    delta_transform_ = registration_->getFinalTransformation();
    abs_transform_ *= delta_transform_;
    last_cloud_ = cloud;
  }

  return (converged);
}

template <typename PointT, typename Scalar>
inline typename pcl::registration::IncrementalRegistration<PointT, Scalar>::Matrix4
IncrementalRegistration<PointT, Scalar>::getDeltaTransform() const
{
  return (delta_transform_);
}

template <typename PointT, typename Scalar>
inline typename pcl::registration::IncrementalRegistration<PointT, Scalar>::Matrix4
IncrementalRegistration<PointT, Scalar>::getAbsoluteTransform() const
{
  return (abs_transform_);
}

template <typename PointT, typename Scalar>
inline void
IncrementalRegistration<PointT, Scalar>::reset()
{
  last_cloud_.reset();
  delta_transform_ = abs_transform_ = Matrix4::Identity();
}

template <typename PointT, typename Scalar>
inline void
IncrementalRegistration<PointT, Scalar>::setRegistration (RegistrationPtr registration)
{
  registration_ = registration;
}

} // namespace registration
} // namespace pcl

#endif /*PCL_REGISTRATION_IMPL_INCREMENTAL_REGISTRATION_HPP_*/

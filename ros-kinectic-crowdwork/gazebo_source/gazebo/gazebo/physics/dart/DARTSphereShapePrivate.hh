/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef _GAZEBO_DARTSPHERESHAPE_PRIVATE_HH_
#define _GAZEBO_DARTSPHERESHAPE_PRIVATE_HH_

#include "gazebo/physics/dart/dart_inc.h"
#include "gazebo/physics/dart/DARTTypes.hh"

namespace gazebo
{
  namespace physics
  {
    /// \internal
    /// \brief Private data class for DARTSphereShape
    class DARTSphereShapePrivate
    {
      /// \brief Constructor
      public: DARTSphereShapePrivate()
        : dtEllipsoidShape(
            new dart::dynamics::EllipsoidShape(Eigen::Vector3d(1, 1, 1)))
      {
      }

      /// \brief Default destructor
      public: ~DARTSphereShapePrivate() = default;

      /// \brief DART sphere shape
      public: std::shared_ptr<dart::dynamics::EllipsoidShape> dtEllipsoidShape;
    };
  }
}
#endif

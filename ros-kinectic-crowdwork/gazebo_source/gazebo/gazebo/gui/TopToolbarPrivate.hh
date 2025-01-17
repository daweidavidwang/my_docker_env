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
#ifndef GAZEBO_GUI_TOPTOOLBARPRIVATE_HH_
#define GAZEBO_GUI_TOPTOOLBARPRIVATE_HH_

#include <vector>

#include "gazebo/gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    /// \internal
    /// \brief Private data for the TopToolbar class
    class TopToolbarPrivate
    {
      /// \brief Widget for the top toolbar
      public: QToolBar *toolbar;

      /// \brief Event based connections.
      public: std::vector<event::ConnectionPtr> connections;
    };
  }
}
#endif

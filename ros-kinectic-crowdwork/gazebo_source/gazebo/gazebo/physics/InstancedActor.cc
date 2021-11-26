/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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
#ifdef _WIN32
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
#endif

#include <sstream>
#include <limits>
#include <algorithm>

#include "gazebo/common/BVHLoader.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/KeyFrame.hh"
#include "gazebo/common/MeshManager.hh"
#include "gazebo/common/Mesh.hh"
#include "gazebo/common/Skeleton.hh"


#include "gazebo/msgs/msgs.hh"

#include "gazebo/physics/InstancedActor.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/Model.hh"
#include "gazebo/physics/World.hh"

#include "gazebo/transport/Node.hh"

using namespace gazebo;
using namespace physics;
using namespace common;

//////////////////////////////////////////////////
InstancedActor::InstancedActor(BasePtr _parent)
  : Model(_parent)
{
  this->AddType(ACTOR);
  this->pathLength = 0.0;
  this->lastTraj = 1e+5;
  this->skinScale = 1.0;
}

//////////////////////////////////////////////////
InstancedActor::~InstancedActor()
{
  this->customTrajectoryInfo.reset();
  this->trajInfo.clear();
  this->trajectories.clear();

  this->mainLink.reset();

  // mesh and skeleton should be deleted by the MeshManager
}

//////////////////////////////////////////////////
void InstancedActor::Load(sdf::ElementPtr _sdf)
{
  // Name
  auto actorName = _sdf->Get<std::string>("name");
  this->SetName(actorName);

      if (_sdf->HasElement("animation"))
      {
        sdf::ElementPtr animSdf = _sdf->GetElement("animation");
        this-> animName = animSdf->Get<std::string>("name");
        std::string animFile = animSdf->Get<std::string>("filename");
        std::string extension = animFile.substr(animFile.rfind(".") + 1,
            animFile.size());
        std::transform(extension.begin(), extension.end(), extension.begin(),
            ::tolower);

        double animScale = animSdf->Get<double>("scale");

        Skeleton *skel = nullptr;
        if (extension == "bvh")
        {
          BVHLoader loader;
          skel = loader.Load(animFile, animScale);
        }
        else if (extension == "dae")
        {
          MeshManager::Instance()->Load(animFile);

          const Mesh *animMesh = nullptr;
          if (MeshManager::Instance()->HasMesh(animFile))
          {
            animMesh = MeshManager::Instance()->GetMesh(animFile);

            if (animMesh && animMesh->HasSkeleton())
            {
              skel = animMesh->GetSkeleton();
              skel->Scale(animScale);
            }
            else
            {
              gzwarn << "Mesh [" << animFile << "] is missing a skeleton."
                  << std::endl;
            }
          }
          else
          {
            gzwarn << "Couldn't load animation file [" << animFile << "]"
                << std::endl;
          }
        }
        else
        {
          gzerr << "Unknown animation file extension [" << extension << "]"
              << std::endl;
        }
        //this->interpolateX[animName] = _sdf->Get<bool>("interpolate_x");
      }
      else
      {
        gzwarn << "Using default animation in skin file. If the skin provided doesn't contain animations, "
            << "the instanced actor's skeleton will not be "
            << "animated." << std::endl;
      }
  // Parse skin
  if (_sdf->HasElement("skin"))
  {
    if (this->LoadSkin(_sdf->GetElement("skin")) )
    {


    }
  }

  Model::Load(_sdf);

  // If there is a skin, check that the skin visual was created and save its id
  std::string actorLinkName = actorName + "::" + actorName + "_pose";
  LinkPtr actorLinkPtr = Model::GetLink(actorLinkName);
  if (actorLinkPtr)
  {
    msgs::Visual actorVisualMsg = actorLinkPtr->GetVisualMessage(
        this->visualName);
    if (actorVisualMsg.has_id())
      this->visualId = actorVisualMsg.id();
    else
    {
      gzerr << "Message for actor visual [" << actorLinkName << "] not found."
          << std::endl;
    }
  }

}

//////////////////////////////////////////////////
bool InstancedActor::LoadSkin(sdf::ElementPtr _skinSdf)
{
  this->skinFile = _skinSdf->Get<std::string>("filename");
  this->skinScale = _skinSdf->Get<double>("scale");
  std::string extension = this->skinFile.substr(this->skinFile.rfind(".") + 1,
      this->skinFile.size());
  if (extension!="mesh")
  {
    gzwarn << "skin for instanced actor is not Ogre3d mesh [" << this->skinFile << "]. " <<
        "Not loading skin." << std::endl;
    return false;
  }


  auto actorName = this->GetName();

  // Create a link to hold the skin visual
  auto linkSdf = _skinSdf->GetParent()->GetElement("link");
  linkSdf->GetAttribute("name")->Set(actorName + "_pose");
  linkSdf->GetElement("gravity")->Set(false);
  linkSdf->GetElement("self_collide")->Set(false);

  std::string actorLinkName = actorName + "::" + actorName + "_pose";
  this->visualName = actorLinkName + "::" + actorName + "_visual";
  this->AddInstancedActorVisual(linkSdf, actorName + "_visual",
      ignition::math::Pose3d::Zero);

  return true;
}


//////////////////////////////////////////////////
void InstancedActor::Init()
{
  this->scriptTime = 0;
  this->prevFrameTime = this->world->GetSimTime();
  if (this->autoStart)
    this->Play();
  this->mainLink = this->GetChildLink(this->GetName() + "_pose");
}

//////////////////////////////////////////////////
void InstancedActor::Play()
{
  this->active = true;
  this->playStartTime = this->world->GetSimTime();
}

//////////////////////////////////////////////////
void InstancedActor::Stop()
{
  this->active = false;
}

//////////////////////////////////////////////////
bool InstancedActor::IsActive() const
{
  return this->active;
}

///////////////////////////////////////////////////
void InstancedActor::Update()
{
  if (!this->active)
    return;

  if ( this->trajectories.empty())
    return;

  common::Time currentTime = this->world->GetSimTime();

  // do not refresh animation faster than 30 Hz sim time
  if ((currentTime - this->prevFrameTime).Double() < (1.0 / 30.0))
    return;

  // Get trajectory
  TrajectoryInfo *tinfo = nullptr;
  if (!this->customTrajectoryInfo)
  {
    this->scriptTime = currentTime.Double() - this->startDelay -
              this->playStartTime.Double();

    // waiting for delayed start
    if (this->scriptTime < 0)
      return;

    if (this->scriptTime >= this->scriptLength)
    {
      if (!this->loop)
      {
        return;
      }
      else
      {
        this->scriptTime = this->scriptTime - this->scriptLength;
        this->playStartTime = currentTime - this->scriptTime;
      }
    }

    // Pick trajectory which should be played at this time
    for (unsigned int i = 0; i < this->trajInfo.size(); ++i)
    {
      if (this->trajInfo[i].startTime <= this->scriptTime &&
          this->trajInfo[i].endTime >= this->scriptTime)
      {
        tinfo = &this->trajInfo[i];
        break;
      }
    }

    if (tinfo == nullptr)
    {
      gzerr << "Trajectory not found at time [" << this->scriptTime << "]"
          << std::endl;
      return;
    }

    this->scriptTime = this->scriptTime - tinfo->startTime;
  }
  else
  {
    tinfo = this->customTrajectoryInfo.get();
  }

  // at this point we are certain that a new frame will be animated
  this->prevFrameTime = currentTime;

  // Update global trajectory (not skeleton animation)
  ignition::math::Pose3d modelPose;
  if (!this->customTrajectoryInfo &&
      this->trajectories.find(tinfo->id) != this->trajectories.end())
  {
    // Get the pose keyframe calculated for this script time
    common::PoseKeyFrame posFrame(0.0);
    this->trajectories[tinfo->id]->SetTime(this->scriptTime);
    this->trajectories[tinfo->id]->GetInterpolatedKeyFrame(posFrame);

    modelPose.Pos() = posFrame.Translation();
    modelPose.Rot() = posFrame.Rotation();

    // Calculate the path length.
    // If we're still in the same trajectory, compare to last position
    if (this->lastTraj == tinfo->id)
    {
      this->pathLength += this->lastPos.Distance(modelPose.Pos());
    }
    // Otherwise, compare to first frame of this trajectory - ?
    else
    {
      auto frame0 = dynamic_cast<common::PoseKeyFrame *>
        (this->trajectories[tinfo->id]->GetKeyFrame(0));
      ignition::math::Vector3d vector3Ign = frame0->Translation();
      this->pathLength = modelPose.Pos().Distance(vector3Ign);
    }
    this->lastPos = modelPose.Pos();
  }
 this->lastTraj = tinfo->id;
    this->SetWorldPose(modelPose);
//FIXME: need send time to render side
 // msgs::Time *stamp = msg.add_time();
  //stamp->CopyFrom(msgs::Convert(_time));

    return;


}


//////////////////////////////////////////////////
void InstancedActor::Fini()
{
  this->ResetCustomTrajectory();
  Model::Fini();
}

//////////////////////////////////////////////////
void InstancedActor::UpdateParameters(sdf::ElementPtr /*_sdf*/)
{
//  Model::UpdateParameters(_sdf);
}

//////////////////////////////////////////////////
const sdf::ElementPtr InstancedActor::GetSDF()
{
  return Model::GetSDF();
}

//////////////////////////////////////////////////
void InstancedActor::SetScriptTime(const double _time)
{
  this->scriptTime = _time;
}

//////////////////////////////////////////////////
double InstancedActor::ScriptTime() const
{
  return this->scriptTime;
}


//////////////////////////////////////////////////
void InstancedActor::SetCustomTrajectory(TrajectoryInfoPtr &_trajInfo)
{
  this->customTrajectoryInfo = _trajInfo;
}

//////////////////////////////////////////////////
void InstancedActor::ResetCustomTrajectory()
{
  this->customTrajectoryInfo.reset();
}


//////////////////////////////////////////////////
void InstancedActor::AddInstancedActorVisual(const sdf::ElementPtr &_linkSdf,
    const std::string &_name, const ignition::math::Pose3d &_pose)
{
  if (this->skinFile.empty())
  {
    gzerr << "Can't add an actor visual without a skin file." << std::endl;
    return;
  }

  // Add visual
  sdf::ElementPtr visualSdf = _linkSdf->AddElement("visual");

  // Set name
  visualSdf->GetAttribute("name")->Set(_name);

  // Set pose
  sdf::ElementPtr visualPoseSdf = visualSdf->GetElement("pose");
  visualPoseSdf->Set(_pose);
  sdf::ElementPtr visualAnimSdf = visualSdf->GetElement("animation");
  visualAnimSdf->GetAttribute("name")->Set(this->animName);
  // Set mesh geometry (skin file)
  sdf::ElementPtr geomVisSdf = visualSdf->GetElement("geometry");
  sdf::ElementPtr meshSdf = geomVisSdf->GetElement("mesh");
  meshSdf->GetElement("uri")->Set(this->skinFile);
  meshSdf->GetElement("scale")->Set(ignition::math::Vector3d(this->skinScale,
      this->skinScale, this->skinScale));
}

/////////////////////////////////////////////////
void InstancedActor::SetSelfCollide(bool /*_self_collide*/)
{
  // InstancedActors don't support self collide
}

/////////////////////////////////////////////////
bool InstancedActor::GetSelfCollide() const
{
  return false;
}

/////////////////////////////////////////////////
void InstancedActor::SetWindMode(bool /*_enable*/)
{
  // InstancedActors don't support wind mode
}

/////////////////////////////////////////////////
bool InstancedActor::WindMode() const
{
  return false;
}


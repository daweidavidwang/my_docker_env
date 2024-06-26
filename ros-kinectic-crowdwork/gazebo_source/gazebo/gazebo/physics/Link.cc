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

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <sstream>
#include <functional>

#include "gazebo/msgs/msgs.hh"

#include "gazebo/transport/TransportIface.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

#include "gazebo/util/OpenAL.hh"
#include "gazebo/common/Events.hh"
#include "gazebo/math/Quaternion.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Assert.hh"
#include "gazebo/common/Battery.hh"

#include "gazebo/physics/PhysicsIface.hh"
#include "gazebo/physics/Model.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/physics/ContactManager.hh"
#include "gazebo/physics/PhysicsEngine.hh"
#include "gazebo/physics/Collision.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/Wind.hh"

#include "gazebo/util/IntrospectionManager.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
Link::Link(EntityPtr _parent)
    : Entity(_parent), initialized(false)
{
  this->AddType(Base::LINK);
  this->inertial.reset(new Inertial);
  this->parentJoints.clear();
  this->childJoints.clear();
  this->publishData = false;
  this->publishDataMutex = new boost::recursive_mutex();
}

//////////////////////////////////////////////////
Link::~Link()
{
  this->Fini();
}

//////////////////////////////////////////////////
void Link::Load(sdf::ElementPtr _sdf)
{
  Entity::Load(_sdf);

  // before loading child collision, we have to figure out if selfCollide is
  // true and modify parent class Entity so this body has its own spaceId
  if (this->sdf->HasElement("self_collide"))
  {
    this->SetSelfCollide(this->sdf->Get<bool>("self_collide"));
  }
  else
  {
    this->SetSelfCollide(this->GetModel()->GetSelfCollide());
  }
  this->sdf->GetElement("self_collide")->GetValue()->SetUpdateFunc(
      std::bind(&Link::GetSelfCollide, this));

  // Parse visuals from SDF
  this->ParseVisuals();

  // Load the geometries
  if (this->sdf->HasElement("collision"))
  {
    sdf::ElementPtr collisionElem = this->sdf->GetElement("collision");
    while (collisionElem)
    {
      // Create and Load a collision, which will belong to this body.
      this->LoadCollision(collisionElem);
      collisionElem = collisionElem->GetNextElement("collision");
    }
  }

  if (this->sdf->HasElement("sensor"))
  {
    sdf::ElementPtr sensorElem = this->sdf->GetElement("sensor");
    while (sensorElem)
    {
      /// \todo This if statement is a hack to prevent Links from creating
      /// a force torque sensor. We should make this more generic.
      if (sensorElem->Get<std::string>("type") == "force_torque")
      {
        gzerr << "A link cannot load a [" <<
          sensorElem->Get<std::string>("type") << "] sensor.\n";
      }
      else if (sensorElem->Get<std::string>("type") != "__default__")
      {
        // This must match the implementation in Sensors::GetScopedName
        std::string sensorName = this->GetScopedName(true) + "::" +
          sensorElem->Get<std::string>("name");

        // Tell the sensor library to create a sensor.
        event::Events::createSensor(sensorElem,
            this->GetWorld()->GetName(), this->GetScopedName(), this->GetId());

        this->sensors.push_back(sensorName);
      }
      sensorElem = sensorElem->GetNextElement("sensor");
    }
  }

  if (!this->IsStatic())
  {
    this->inertial->Load(this->sdf->GetElement("inertial"));
  }

#ifdef HAVE_OPENAL
  if (_sdf->HasElement("audio_source"))
  {
    // bool onContact = false;
    sdf::ElementPtr audioElem = this->sdf->GetElement("audio_source");
    std::vector<std::string> collisionNames;

    while (audioElem)
    {
      util::OpenALSourcePtr source = util::OpenAL::Instance()->CreateSource(
          audioElem);

      std::vector<std::string> names = source->CollisionNames();
      std::copy(names.begin(), names.end(), std::back_inserter(collisionNames));

      audioElem = audioElem->GetNextElement("audio_source");
      this->audioSources.push_back(source);
    }

    if (!collisionNames.empty())
    {
      for (std::vector<std::string>::iterator iter = collisionNames.begin();
          iter != collisionNames.end(); ++iter)
      {
        (*iter) = this->GetScopedName() + "::" + (*iter);
      }

      std::string topic =
        this->world->GetPhysicsEngine()->GetContactManager()->CreateFilter(
            this->GetScopedName() + "/audio_collision", collisionNames);
      this->audioContactsSub = this->node->Subscribe(topic,
          &Link::OnCollision, this);
    }
  }

  if (_sdf->HasElement("audio_sink"))
  {
    this->audioSink = util::OpenAL::Instance()->CreateSink(
        _sdf->GetElement("audio_sink"));
  }
#endif

  if (this->sdf->HasElement("battery"))
  {
    sdf::ElementPtr batteryElem = this->sdf->GetElement("battery");
    while (batteryElem)
    {
      this->LoadBattery(batteryElem);
      batteryElem = batteryElem->GetNextElement("battery");
    }
  }

  if (this->sdf->HasElement("enable_wind"))
  {
    this->SetWindMode(this->sdf->Get<bool>("enable_wind"));
  }
  else
  {
    this->SetWindMode(this->GetModel()->WindMode());
  }
  this->sdf->GetElement("enable_wind")->GetValue()->SetUpdateFunc(
      std::bind(&Link::WindMode, this));

  this->connections.push_back(event::Events::ConnectWorldUpdateBegin(
      boost::bind(&Link::Update, this, _1)));

  this->SetStatic(this->IsStatic());
}

//////////////////////////////////////////////////
void Link::Init()
{
  this->linearAccel.Set(0, 0, 0);
  this->angularAccel.Set(0, 0, 0);

  this->enabled = true;

  // Set Link pose before setting pose of child collisions
  this->SetRelativePose(this->sdf->Get<math::Pose>("pose"));
  this->SetInitialRelativePose(this->sdf->Get<math::Pose>("pose"));

  // Call Init for child collisions, which whill set their pose
  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
  {
    if ((*iter)->HasType(Base::COLLISION))
    {
      CollisionPtr collision = boost::static_pointer_cast<Collision>(*iter);
      this->collisions.push_back(collision);
      collision->Init();
    }
  }

  // Initialize all the batteries
  for (auto &battery : this->batteries)
  {
    battery->Init();
  }

  if (this->WindMode() && this->world->WindEnabled())
    this->SetWindEnabled(true);

  this->initialized = true;
}

//////////////////////////////////////////////////
void Link::Fini()
{
  this->attachedModels.clear();
  this->parentJoints.clear();
  this->childJoints.clear();
  this->collisions.clear();
  this->inertial.reset();
  this->batteries.clear();

  // Remove all the sensors attached to the link
  for (auto const &sensor : this->sensors)
  {
    event::Events::removeSensor(sensor);
  }

  this->sensors.clear();

  // Clean up visuals
  // FIXME: Do we really need to send 2 msgs to delete a visual?!
  if (this->visPub && this->requestPub)
  {
    for (auto iter : this->visuals)
    {
      auto deleteMsg = msgs::CreateRequest("entity_delete",
          std::to_string(iter.second.id()));
      this->requestPub->Publish(*deleteMsg, true);
      delete deleteMsg;

      msgs::Visual msg;
      msg.set_name(iter.second.name());
      msg.set_id(iter.second.id());
      if (this->parent)
      {
        msg.set_parent_name(this->parent->GetScopedName());
        msg.set_parent_id(this->parent->GetId());
      }
      else
      {
        msg.set_parent_name("");
        msg.set_parent_id(0);
      }
      msg.set_delete_me(true);
      this->visPub->Publish(msg);
    }
  }
  this->visuals.clear();

#ifdef HAVE_OPENAL
  if (this->world && this->world->GetPhysicsEngine() &&
      this->world->GetPhysicsEngine()->GetContactManager())
  {
    this->world->GetPhysicsEngine()->GetContactManager()->RemoveFilter(
        this->GetScopedName() + "/audio_collision");
  }
  this->audioContactsSub.reset();
  this->audioSink.reset();
  this->audioSources.clear();
#endif

  // Clean transport
  {
    this->dataPub.reset();
    this->visPub.reset();

    this->wrenchSub.reset();
  }
  this->connections.clear();

  delete this->publishDataMutex;
  this->publishDataMutex = NULL;

  Entity::Fini();
}

//////////////////////////////////////////////////
void Link::Reset()
{
  // resets pose
  Entity::Reset();

  // resets velocity, acceleration, wrench
  this->ResetPhysicsStates();
}

//////////////////////////////////////////////////
void Link::ResetPhysicsStates()
{
  this->SetAngularVel(math::Vector3(0, 0, 0));
  this->SetLinearVel(math::Vector3(0, 0, 0));
  this->SetAngularAccel(math::Vector3(0, 0, 0));
  this->SetLinearAccel(math::Vector3(0, 0, 0));
  this->SetForce(math::Vector3(0, 0, 0));
  this->SetTorque(math::Vector3(0, 0, 0));
}

//////////////////////////////////////////////////
void Link::UpdateParameters(sdf::ElementPtr _sdf)
{
  Entity::UpdateParameters(_sdf);

  if (this->sdf->HasElement("inertial"))
  {
    sdf::ElementPtr inertialElem = this->sdf->GetElement("inertial");
    this->inertial->UpdateParameters(inertialElem);
  }

  this->sdf->GetElement("gravity")->GetValue()->SetUpdateFunc(
      boost::bind(&Link::GetGravityMode, this));
  this->sdf->GetElement("enable_wind")->GetValue()->SetUpdateFunc(
      std::bind(&Link::WindMode, this));
  this->sdf->GetElement("kinematic")->GetValue()->SetUpdateFunc(
      boost::bind(&Link::GetKinematic, this));

  if (this->sdf->Get<bool>("gravity") != this->GetGravityMode())
    this->SetGravityMode(this->sdf->Get<bool>("gravity"));

  this->SetWindMode(this->sdf->Get<bool>("enable_wind"));

  // before loading child collision, we have to figure out if
  // selfCollide is true and modify parent class Entity so this
  // body has its own spaceId
  this->SetSelfCollide(this->sdf->Get<bool>("self_collide"));

  // TODO: this shouldn't be in the physics sim
  if (this->sdf->HasElement("visual"))
  {
    sdf::ElementPtr visualElem = this->sdf->GetElement("visual");
    while (visualElem)
    {
      // TODO: Update visuals properly
      msgs::Visual msg = msgs::VisualFromSDF(visualElem);

      msg.set_name(this->GetScopedName() + "::" + msg.name());
      msg.set_parent_name(this->GetScopedName());
      msg.set_is_static(this->IsStatic());
      msg.set_type(msgs::Visual::VISUAL);

      this->visPub->Publish(msg);

      visualElem = visualElem->GetNextElement("visual");
    }
  }

  if (this->sdf->HasElement("collision"))
  {
    sdf::ElementPtr collisionElem = this->sdf->GetElement("collision");
    while (collisionElem)
    {
      CollisionPtr collision = boost::dynamic_pointer_cast<Collision>(
          this->GetChild(collisionElem->Get<std::string>("name")));

      if (collision)
        collision->UpdateParameters(collisionElem);
      collisionElem = collisionElem->GetNextElement("collision");
    }
  }

  // Update the battery information
  if (this->sdf->HasElement("battery"))
  {
    sdf::ElementPtr batteryElem = this->sdf->GetElement("battery");
    while (batteryElem)
    {
      common::BatteryPtr battery = this->Battery(
          batteryElem->Get<std::string>("name"));

      if (battery)
        battery->UpdateParameters(batteryElem);
      batteryElem = batteryElem->GetNextElement("battery");
    }
  }
}

//////////////////////////////////////////////////
void Link::SetCollideMode(const std::string &_mode)
{
  unsigned int categoryBits;
  unsigned int collideBits;

  if (_mode == "all")
  {
    categoryBits =  GZ_ALL_COLLIDE;
    collideBits =  GZ_ALL_COLLIDE;
  }
  else if (_mode == "none")
  {
    categoryBits =  GZ_NONE_COLLIDE;
    collideBits =  GZ_NONE_COLLIDE;
  }
  else if (_mode == "sensors")
  {
    categoryBits = GZ_SENSOR_COLLIDE;
    collideBits = ~GZ_SENSOR_COLLIDE;
  }
  else if (_mode == "fixed")
  {
    categoryBits = GZ_FIXED_COLLIDE;
    collideBits = ~GZ_FIXED_COLLIDE;
  }
  else if (_mode == "ghost")
  {
    categoryBits = GZ_GHOST_COLLIDE;
    collideBits = ~GZ_GHOST_COLLIDE;
  }
  else
  {
    gzerr << "Unknown collide mode[" << _mode << "]\n";
    return;
  }

  for (Collision_V::iterator iter = this->collisions.begin();
       iter != this->collisions.end(); ++iter)
  {
    if ((*iter))
    {
      (*iter)->SetCategoryBits(categoryBits);
      (*iter)->SetCollideBits(collideBits);
    }
  }
}

//////////////////////////////////////////////////
bool Link::GetSelfCollide() const
{
  GZ_ASSERT(this->sdf != NULL, "Link sdf member is NULL");
  if (this->sdf->HasElement("self_collide"))
    return this->sdf->Get<bool>("self_collide");
  else
    return false;
}

//////////////////////////////////////////////////
void Link::SetLaserRetro(float _retro)
{
  for (Collision_V::iterator iter = this->collisions.begin();
       iter != this->collisions.end(); ++iter)
  {
    (*iter)->SetLaserRetro(_retro);
  }
}

//////////////////////////////////////////////////
void Link::Update(const common::UpdateInfo & /*_info*/)
{
#ifdef HAVE_OPENAL
  if (this->audioSink)
  {
    this->audioSink->SetPose(this->GetWorldPose().Ign());
    this->audioSink->SetVelocity(this->GetWorldLinearVel().Ign());
  }

  // Update all the audio sources
  for (std::vector<util::OpenALSourcePtr>::iterator iter =
      this->audioSources.begin(); iter != this->audioSources.end(); ++iter)
  {
    (*iter)->SetPose(this->GetWorldPose().Ign());
    (*iter)->SetVelocity(this->GetWorldLinearVel().Ign());
  }
#endif

  // FIXME: race condition on factory-based model loading!!!!!
   /*if (this->GetEnabled() != this->enabled)
   {
     this->enabled = this->GetEnabled();
     this->enabledSignal(this->enabled);
   }*/

  if (!this->IsStatic() && !this->wrenchMsgs.empty())
  {
    std::vector<msgs::Wrench> messages;
    {
      boost::mutex::scoped_lock lock(this->wrenchMsgMutex);
      messages = this->wrenchMsgs;
      this->wrenchMsgs.clear();
    }

    for (auto it : messages)
    {
      this->ProcessWrenchMsg(it);
    }
  }

  // Update the batteries.
  for (auto &battery : this->batteries)
  {
    battery->Update();
  }
}

//////////////////////////////////////////////////
void Link::UpdateWind(const common::UpdateInfo & /*_info*/)
{
  this->windLinearVel = this->world->Wind().WorldLinearVel(this);
}

/////////////////////////////////////////////////
Joint_V Link::GetParentJoints() const
{
  return this->parentJoints;
}

/////////////////////////////////////////////////
Joint_V Link::GetChildJoints() const
{
  return this->childJoints;
}

/////////////////////////////////////////////////
Link_V Link::GetChildJointsLinks() const
{
  Link_V links;
  for (std::vector<JointPtr>::const_iterator iter = this->childJoints.begin();
                                             iter != this->childJoints.end();
                                             ++iter)
  {
    if ((*iter)->GetChild())
      links.push_back((*iter)->GetChild());
  }
  return links;
}

/////////////////////////////////////////////////
Link_V Link::GetParentJointsLinks() const
{
  Link_V links;
  for (std::vector<JointPtr>::const_iterator iter = this->parentJoints.begin();
                                             iter != this->parentJoints.end();
                                             ++iter)
  {
    if ((*iter)->GetParent())
      links.push_back((*iter)->GetParent());
  }
  return links;
}

//////////////////////////////////////////////////
void Link::LoadCollision(sdf::ElementPtr _sdf)
{
  CollisionPtr collision;
  std::string geomType =
    _sdf->GetElement("geometry")->GetFirstElement()->GetName();

  if (geomType == "heightmap" || geomType == "map")
    this->SetStatic(true);

  collision = this->GetWorld()->GetPhysicsEngine()->CreateCollision(geomType,
      boost::static_pointer_cast<Link>(shared_from_this()));

  if (!collision)
    gzthrow("Unknown Collisionetry Type[" + geomType + "]");

  collision->Load(_sdf);
}

//////////////////////////////////////////////////
CollisionPtr Link::GetCollisionById(unsigned int _id) const
{
  return boost::dynamic_pointer_cast<Collision>(this->GetById(_id));
}

//////////////////////////////////////////////////
CollisionPtr Link::GetCollision(const std::string &_name)
{
  CollisionPtr result;
  Base_V::const_iterator biter;
  for (biter = this->children.begin(); biter != this->children.end(); ++biter)
  {
    if ((*biter)->GetName() == _name)
    {
      result = boost::dynamic_pointer_cast<Collision>(*biter);
      break;
    }
  }

  return result;
}

//////////////////////////////////////////////////
Collision_V Link::GetCollisions() const
{
  return this->collisions;
}

//////////////////////////////////////////////////
CollisionPtr Link::GetCollision(unsigned int _index) const
{
  CollisionPtr collision;
  if (_index <= this->GetChildCount())
    collision = boost::static_pointer_cast<Collision>(this->GetChild(_index));
  else
    gzerr << "Index is out of range\n";

  return collision;
}

//////////////////////////////////////////////////
void Link::SetLinearAccel(const math::Vector3 &_accel)
{
  this->SetEnabled(true);
  this->linearAccel = _accel;
}

//////////////////////////////////////////////////
void Link::SetAngularAccel(const math::Vector3 &_accel)
{
  this->SetEnabled(true);
  this->angularAccel = _accel;
}

//////////////////////////////////////////////////
math::Pose Link::GetWorldCoGPose() const
{
  math::Pose pose = this->GetWorldPose();
  pose.pos += pose.rot.RotateVector(this->inertial->GetCoG());
  return pose;
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeLinearVel() const
{
  return this->GetWorldPose().rot.RotateVectorReverse(
      this->GetWorldLinearVel());
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeAngularVel() const
{
  return this->GetWorldPose().rot.RotateVectorReverse(
         this->GetWorldAngularVel());
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeLinearAccel() const
{
  return this->GetRelativeForce() / this->inertial->GetMass();
}

//////////////////////////////////////////////////
math::Vector3 Link::GetWorldLinearAccel() const
{
  return this->GetWorldForce() / this->inertial->GetMass();
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeAngularAccel() const
{
  return this->GetWorldPose().rot.RotateVectorReverse(
    this->GetWorldAngularAccel());
}

//////////////////////////////////////////////////
math::Vector3 Link::GetWorldAngularAccel() const
{
  // I: inertia matrix in world frame
  // T: sum of external torques in world frame
  // L: angular momentum of CoG in world frame
  // w: angular velocity in world frame
  // return I^-1 * (T - w x L)
  return this->GetWorldInertiaMatrix().Inverse() * (this->GetWorldTorque()
    - this->GetWorldAngularVel().Cross(this->GetWorldAngularMomentum()));
}

//////////////////////////////////////////////////
math::Vector3 Link::GetWorldAngularMomentum() const
{
  return this->GetWorldInertiaMatrix() * this->GetWorldAngularVel();
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeForce() const
{
  return this->GetWorldPose().rot.RotateVectorReverse(this->GetWorldForce());
}

//////////////////////////////////////////////////
math::Vector3 Link::GetRelativeTorque() const
{
  return this->GetWorldPose().rot.RotateVectorReverse(this->GetWorldTorque());
}

//////////////////////////////////////////////////
ModelPtr Link::GetModel() const
{
  return boost::dynamic_pointer_cast<Model>(this->GetParent());
}

//////////////////////////////////////////////////
math::Box Link::GetBoundingBox() const
{
  math::Box box;

  box.min.Set(GZ_DBL_MAX, GZ_DBL_MAX, GZ_DBL_MAX);
  box.max.Set(0, 0, 0);

  for (Collision_V::const_iterator iter = this->collisions.begin();
       iter != this->collisions.end(); ++iter)
  {
    box += (*iter)->GetBoundingBox();
  }

  return box;
}

//////////////////////////////////////////////////
void Link::SetWindMode(const bool _mode)
{
  this->sdf->GetElement("enable_wind")->Set(_mode);

  if (!this->WindMode() && this->updateConnection)
    this->SetWindEnabled(false);
  else if (this->WindMode() && !this->updateConnection)
    this->SetWindEnabled(true);
}

/////////////////////////////////////////////////
void Link::SetWindEnabled(const bool _enable)
{
  if (_enable)
  {
    this->updateConnection = event::Events::ConnectWorldUpdateBegin(
        std::bind(&Link::UpdateWind, this, std::placeholders::_1));
  }
  else
  {
    this->updateConnection.reset();
    // Make sure wind velocity is null
    this->windLinearVel.Set(0, 0, 0);
  }
}

//////////////////////////////////////////////////
const ignition::math::Vector3d Link::WorldWindLinearVel() const
{
  return this->windLinearVel;
}

//////////////////////////////////////////////////
bool Link::WindMode() const
{
  return this->sdf->Get<bool>("enable_wind");
}

//////////////////////////////////////////////////
bool Link::SetSelected(bool _s)
{
  Entity::SetSelected(_s);

  if (_s == false)
    this->SetEnabled(true);

  return true;
}

//////////////////////////////////////////////////
void Link::SetInertial(const InertialPtr &/*_inertial*/)
{
  gzwarn << "Link::SetMass is empty\n";
}

//////////////////////////////////////////////////
math::Pose Link::GetWorldInertialPose() const
{
  math::Pose inertialPose;
  if (this->inertial)
    inertialPose = this->inertial->GetPose();
  return inertialPose + this->GetWorldPose();
}

//////////////////////////////////////////////////
math::Matrix3 Link::GetWorldInertiaMatrix() const
{
  math::Matrix3 moi;
  if (this->inertial)
  {
    math::Vector3 pos = this->inertial->GetPose().pos;
    math::Quaternion rot = this->GetWorldPose().rot.GetInverse();
    moi = this->inertial->GetMOI(math::Pose(pos, rot));
  }
  return moi;
}

//////////////////////////////////////////////////
void Link::AddParentJoint(JointPtr _joint)
{
  this->parentJoints.push_back(_joint);
}

//////////////////////////////////////////////////
void Link::AddChildJoint(JointPtr _joint)
{
  this->childJoints.push_back(_joint);
}

//////////////////////////////////////////////////
void Link::RemoveParentJoint(const std::string &_jointName)
{
  for (std::vector<JointPtr>::iterator iter = this->parentJoints.begin();
                                       iter != this->parentJoints.end();
                                       ++iter)
  {
    /// @todo: can we assume there are no repeats?
    if ((*iter)->GetName() == _jointName)
    {
      this->parentJoints.erase(iter);
      break;
    }
  }
}

//////////////////////////////////////////////////
void Link::RemoveChildJoint(const std::string &_jointName)
{
  for (std::vector<JointPtr>::iterator iter = this->childJoints.begin();
                                       iter != this->childJoints.end();
                                       ++iter)
  {
    /// @todo: can we assume there are no repeats?
    if ((*iter)->GetName() == _jointName)
    {
      this->childJoints.erase(iter);
      break;
    }
  }
}

//////////////////////////////////////////////////
void Link::FillMsg(msgs::Link &_msg)
{
  math::Pose relPose = this->GetRelativePose();

  _msg.set_id(this->GetId());
  _msg.set_name(this->GetScopedName());
  _msg.set_self_collide(this->GetSelfCollide());
  _msg.set_gravity(this->GetGravityMode());
  _msg.set_enable_wind(this->WindMode());
  _msg.set_kinematic(this->GetKinematic());
  _msg.set_enabled(this->GetEnabled());
  msgs::Set(_msg.mutable_pose(), relPose.Ign());

  // The visual msgs name might not have been set if the link was created
  // dynamically without using SDF.
  if (!this->visualMsg->has_name())
    this->visualMsg->set_name(this->GetScopedName());

  msgs::Set(this->visualMsg->mutable_pose(), relPose.Ign());
  _msg.add_visual()->CopyFrom(*this->visualMsg);

  _msg.mutable_inertial()->set_mass(this->inertial->GetMass());
  _msg.mutable_inertial()->set_ixx(this->inertial->GetIXX());
  _msg.mutable_inertial()->set_ixy(this->inertial->GetIXY());
  _msg.mutable_inertial()->set_ixz(this->inertial->GetIXZ());
  _msg.mutable_inertial()->set_iyy(this->inertial->GetIYY());
  _msg.mutable_inertial()->set_iyz(this->inertial->GetIYZ());
  _msg.mutable_inertial()->set_izz(this->inertial->GetIZZ());
  msgs::Set(_msg.mutable_inertial()->mutable_pose(),
      this->inertial->GetPose().Ign());

  for (auto &child : this->children)
  {
    if (child->HasType(Base::COLLISION))
    {
      CollisionPtr collision = boost::static_pointer_cast<Collision>(child);
      collision->FillMsg(*_msg.add_collision());
    }
  }

  // Add in the sensor data.
  if (this->sdf->HasElement("sensor"))
  {
    sdf::ElementPtr sensorElem = this->sdf->GetElement("sensor");
    while (sensorElem)
    {
      msgs::Sensor *msg = _msg.add_sensor();
      msg->CopyFrom(msgs::SensorFromSDF(sensorElem));
      msg->set_parent(this->GetScopedName());
      msg->set_parent_id(this->GetId());
      sensorElem = sensorElem->GetNextElement("sensor");
    }
  }

  if (this->visuals.empty())
    this->ParseVisuals();
  else
    this->UpdateVisualMsg();

  for (Visuals_M::iterator iter = this->visuals.begin();
      iter != this->visuals.end(); ++iter)
  {
    msgs::Visual *vis = _msg.add_visual();
    vis->CopyFrom(iter->second);
  }

  if (this->sdf->HasElement("projector"))
  {
    sdf::ElementPtr elem = this->sdf->GetElement("projector");

    msgs::Projector *proj = _msg.add_projector();
    proj->set_name(
        this->GetScopedName() + "::" + elem->Get<std::string>("name"));
    proj->set_texture(elem->Get<std::string>("texture"));
    proj->set_fov(elem->Get<double>("fov"));
    proj->set_near_clip(elem->Get<double>("near_clip"));
    proj->set_far_clip(elem->Get<double>("far_clip"));
    msgs::Set(proj->mutable_pose(), elem->Get<ignition::math::Pose3d>("pose"));
  }

  if (this->IsCanonicalLink())
    _msg.set_canonical(true);

  // Fill message with battery information
  for (auto &battery : this->batteries)
  {
    msgs::Battery *bat = _msg.add_battery();
    bat->set_name(battery->Name());
    bat->set_voltage(battery->Voltage());
  }
}

//////////////////////////////////////////////////
void Link::ProcessMsg(const msgs::Link &_msg)
{
  if (_msg.id() != this->GetId())
  {
    return;
  }

  this->SetName(_msg.name());

  if (_msg.has_self_collide())
    this->SetSelfCollide(_msg.self_collide());
  if (_msg.has_gravity())
  {
    this->SetGravityMode(_msg.gravity());
    this->SetEnabled(true);
  }
  if (_msg.has_enable_wind())
    this->SetWindMode(_msg.enable_wind());
  if (_msg.has_kinematic())
  {
    this->SetKinematic(_msg.kinematic());
    this->SetEnabled(true);
  }
  if (_msg.has_inertial())
  {
    this->inertial->ProcessMsg(_msg.inertial());
    this->SetEnabled(true);
    // Only update the Center of Mass if object is dynamic
    if (!this->GetKinematic())
      this->UpdateMass();
  }

  if (_msg.has_pose())
  {
    this->SetEnabled(true);
    this->SetRelativePose(msgs::ConvertIgn(_msg.pose()));
  }

  for (int i = 0; i < _msg.collision_size(); i++)
  {
    CollisionPtr coll = this->GetCollisionById(_msg.collision(i).id());
    if (coll)
      coll->ProcessMsg(_msg.collision(i));
  }
  if (_msg.collision_size()>0)
    this->UpdateSurface();
}

//////////////////////////////////////////////////
unsigned int Link::GetSensorCount() const
{
  return this->sensors.size();
}

//////////////////////////////////////////////////
std::string Link::GetSensorName(unsigned int _i) const
{
  if (_i < this->sensors.size())
    return this->sensors[_i];

  return std::string();
}

//////////////////////////////////////////////////
void Link::AttachStaticModel(ModelPtr &_model, const math::Pose &_offset)
{
  if (!_model->IsStatic())
  {
    gzerr << "AttachStaticModel requires a static model\n";
    return;
  }

  this->attachedModels.push_back(_model);
  this->attachedModelsOffset.push_back(_offset);
}

//////////////////////////////////////////////////
void Link::DetachStaticModel(const std::string &_modelName)
{
  for (unsigned int i = 0; i < this->attachedModels.size(); i++)
  {
    if (this->attachedModels[i]->GetName() == _modelName)
    {
      this->attachedModels.erase(this->attachedModels.begin()+i);
      this->attachedModelsOffset.erase(this->attachedModelsOffset.begin()+i);
      break;
    }
  }
}

//////////////////////////////////////////////////
void Link::DetachAllStaticModels()
{
  this->attachedModels.clear();
  this->attachedModelsOffset.clear();
}

//////////////////////////////////////////////////
void Link::OnPoseChange()
{
  math::Pose p;
  for (unsigned int i = 0; i < this->attachedModels.size(); i++)
  {
    p = this->GetWorldPose();
    p.pos += this->attachedModelsOffset[i].pos;
    p.rot = p.rot * this->attachedModelsOffset[i].rot;

    this->attachedModels[i]->SetWorldPose(p, true);
  }
}

//////////////////////////////////////////////////
void Link::SetState(const LinkState &_state)
{
  this->SetWorldPose(_state.GetPose());
  this->SetLinearVel(_state.GetVelocity().pos);
  this->SetAngularVel(_state.GetVelocity().rot.GetAsEuler());
  this->SetLinearAccel(_state.GetAcceleration().pos);
  this->SetAngularAccel(_state.GetAcceleration().rot.GetAsEuler());
  this->SetForce(_state.GetWrench().pos);
  this->SetTorque(_state.GetWrench().rot.GetAsEuler());

  /*
  for (unsigned int i = 0; i < _state.GetCollisionStateCount(); ++i)
  {
    CollisionState collisionState = _state.GetCollisionState(i);
    CollisionPtr collision = this->GetCollision(collisionState.GetName());
    if (collision)
      collision->SetState(collisionState);
    else
      gzerr << "Unable to find collision[" << collisionState.GetName() << "]\n";
  }*/
}

/////////////////////////////////////////////////
double Link::GetLinearDamping() const
{
  if (this->sdf->HasElement("velocity_decay"))
    return this->sdf->GetElement("velocity_decay")->Get<double>("linear");
  else
    return 0.0;
}

/////////////////////////////////////////////////
double Link::GetAngularDamping() const
{
  if (this->sdf->HasElement("velocity_decay"))
    return this->sdf->GetElement("velocity_decay")->Get<double>("angular");
  else
    return 0.0;
}

/////////////////////////////////////////////////
void Link::SetKinematic(const bool &/*_kinematic*/)
{
}

/////////////////////////////////////////////////
void Link::SetPublishData(bool _enable)
{
  // Skip if we're trying to disable after the publisher has already been
  // cleared
  if (!_enable && !this->dataPub)
    return;

  {
    boost::recursive_mutex::scoped_lock lock(*this->publishDataMutex);
    if (this->publishData == _enable)
      return;

    this->publishData = _enable;
  }
  if (_enable)
  {
    std::string topic = "~/" + this->GetScopedName();
    this->dataPub = this->node->Advertise<msgs::LinkData>(topic);
    this->connections.push_back(
      event::Events::ConnectWorldUpdateEnd(
        boost::bind(&Link::PublishData, this)));
  }
  else
  {
    this->dataPub.reset();
    // Do we want to clear all of them though?
    this->connections.clear();
  }
}

/////////////////////////////////////////////////
void Link::PublishData()
{
  if (this->publishData && this->dataPub->HasConnections())
  {
    msgs::Set(this->linkDataMsg.mutable_time(), this->world->GetSimTime());
    linkDataMsg.set_name(this->GetScopedName());
    msgs::Set(this->linkDataMsg.mutable_linear_velocity(),
        this->GetWorldLinearVel().Ign());
    msgs::Set(this->linkDataMsg.mutable_angular_velocity(),
        this->GetWorldAngularVel().Ign());
    this->dataPub->Publish(this->linkDataMsg);
  }
}

//////////////////////////////////////////////////
common::BatteryPtr Link::Battery(const std::string &_name) const
{
  common::BatteryPtr result;

  for (auto &battery : this->batteries)
  {
    if (battery->Name() == _name)
    {
      result = battery;
      break;
    }
  }

  return result;
}

/////////////////////////////////////////////////
common::BatteryPtr Link::Battery(const size_t _index) const
{
  if (_index < this->batteries.size())
    return this->batteries[_index];
  else
    return common::BatteryPtr();
}

/////////////////////////////////////////////////
size_t Link::BatteryCount() const
{
  return this->batteries.size();
}

//////////////////////////////////////////////////
bool Link::VisualId(const std::string &_visName, uint32_t &_visualId) const
{
  for (auto &iter : this->visuals)
  {
    if (iter.second.name() == _visName ||
        iter.second.name() == this->GetScopedName() + "::" + _visName)
    {
      _visualId = iter.first;
      return true;
    }
  }
  gzerr << "Trying to get unique ID of visual from invalid visual name["
        << _visName << "] for link [" << this->GetScopedName() << "]\n";
  return false;
}

//////////////////////////////////////////////////
bool Link::VisualPose(const uint32_t _id, ignition::math::Pose3d &_pose) const
{
  auto iter = this->visuals.find(_id);
  if (iter == this->visuals.end())
  {
    gzerr << "Trying to get pose of visual from invalid visual id[" << _id
          << "] for link [" << this->GetScopedName() << "]\n";
    return false;
  }
  const msgs::Visual &msg = iter->second;
  if (msg.has_pose())
  {
    _pose = msgs::ConvertIgn(msg.pose());
  }
  else
  {
    // Pose wasn't specified on SDF, use default value
    _pose = ignition::math::Pose3d::Zero;
  }
  return true;
}

//////////////////////////////////////////////////
bool Link::SetVisualPose(const uint32_t _id,
                         const ignition::math::Pose3d &_pose)
{
  auto iter = this->visuals.find(_id);
  if (iter == this->visuals.end())
  {
    gzerr << "Trying to set pose of visual from invalid visual id[" << _id
          << "] for link [" << this->GetScopedName() << "]\n";
    return false;
  }
  msgs::Visual &msg = iter->second;
  msgs::Set(msg.mutable_pose(), _pose);
  std::string linkName = this->GetScopedName();
  if (this->sdf->HasElement("visual"))
  {
    sdf::ElementPtr visualElem = this->sdf->GetElement("visual");
    while (visualElem)
    {
      std::string visName = linkName + "::" +
        visualElem->Get<std::string>("name");

      // update visual msg if it exists
      if (msg.name() == visName)
      {
        visualElem->GetElement("pose")->Set(_pose);
        break;
      }

      visualElem = visualElem->GetNextElement("visual");
    }
  }
  msgs::Visual visual;
  visual.set_name(msg.name());
  visual.set_id(_id);
  visual.set_parent_name(linkName);
  visual.set_parent_id(this->GetId());
  msgs::Set(visual.mutable_pose(), _pose);
  this->visPub->Publish(visual);
  return true;
}

//////////////////////////////////////////////////
void Link::OnCollision(ConstContactsPtr &_msg)
{
  std::string collisionName1;
  std::string collisionName2;
  std::string::size_type pos1, pos2;

  for (int i = 0; i < _msg->contact_size(); ++i)
  {
    collisionName1 = _msg->contact(i).collision1();
    collisionName2 = _msg->contact(i).collision2();
    pos1 = collisionName1.rfind("::");
    pos2 = collisionName2.rfind("::");

    GZ_ASSERT(pos1 != std::string::npos, "Invalid collision name");
    GZ_ASSERT(pos2 != std::string::npos, "Invalid collision name");

    collisionName1 = collisionName1.substr(pos1+2);
    collisionName2 = collisionName2.substr(pos2+2);

#ifdef HAVE_OPENAL
    for (std::vector<util::OpenALSourcePtr>::iterator iter =
        this->audioSources.begin(); iter != this->audioSources.end(); ++iter)
    {
      if ((*iter)->HasCollisionName(collisionName1) ||
          (*iter)->HasCollisionName(collisionName2))
        (*iter)->Play();
    }
#endif
  }
}

/////////////////////////////////////////////////
void Link::ParseVisuals()
{
  this->UpdateVisualMsg();

  for (auto const it : this->visuals)
    this->visPub->Publish(it.second);
}

/////////////////////////////////////////////////
void Link::RemoveChild(EntityPtr _child)
{
  if (_child->HasType(COLLISION))
  {
    this->RemoveCollision(_child->GetScopedName());
  }

  Entity::RemoveChild(_child->GetId());

  this->SetEnabled(true);
}

/////////////////////////////////////////////////
void Link::RemoveCollision(const std::string &_name)
{
  for (Collision_V::iterator iter = this->collisions.begin();
       iter != this->collisions.end(); ++iter)
  {
    if ((*iter)->GetName() == _name || (*iter)->GetScopedName() == _name)
    {
      this->collisions.erase(iter);
      break;
    }
  }
}

/////////////////////////////////////////////////
void Link::SetScale(const math::Vector3 &_scale)
{
  Base_V::const_iterator biter;
  for (biter = this->children.begin(); biter != this->children.end(); ++biter)
  {
    if ((*biter)->HasType(Base::COLLISION))
    {
      boost::static_pointer_cast<Collision>(*biter)->SetScale(_scale);
    }
  }

  // update the visual sdf to ensure cloning and saving has the correct values.
  this->UpdateVisualGeomSDF(_scale);

  this->scale = _scale.Ign();
}

//////////////////////////////////////////////////
void Link::UpdateVisualGeomSDF(const math::Vector3 &_scale)
{
  // TODO: this shouldn't be in the physics sim
  if (this->sdf->HasElement("visual"))
  {
    sdf::ElementPtr visualElem = this->sdf->GetElement("visual");
    while (visualElem)
    {
      sdf::ElementPtr geomElem = visualElem->GetElement("geometry");

      if (geomElem->HasElement("box"))
      {
        math::Vector3 size =
            geomElem->GetElement("box")->Get<math::Vector3>("size");
        geomElem->GetElement("box")->GetElement("size")->Set(
            _scale/this->scale*size);
      }
      else if (geomElem->HasElement("sphere"))
      {
        // update radius the same way as collision shapes
        double radius = geomElem->GetElement("sphere")->Get<double>("radius");
        double newRadius = std::max(_scale.z, std::max(_scale.x, _scale.y));
        double oldRadius = std::max(this->scale.Z(),
            std::max(this->scale.X(), this->scale.Y()));
        geomElem->GetElement("sphere")->GetElement("radius")->Set(
            newRadius/oldRadius*radius);
      }
      else if (geomElem->HasElement("cylinder"))
      {
        // update radius the same way as collision shapes
        double radius = geomElem->GetElement("cylinder")->Get<double>("radius");
        double newRadius = std::max(_scale.x, _scale.y);
        double oldRadius = std::max(this->scale.X(), this->scale.Y());

        double length = geomElem->GetElement("cylinder")->Get<double>("length");
        geomElem->GetElement("cylinder")->GetElement("radius")->Set(
            newRadius/oldRadius*radius);
        geomElem->GetElement("cylinder")->GetElement("length")->Set(
            _scale.z/this->scale.Z()*length);
      }
      else if (geomElem->HasElement("mesh"))
        geomElem->GetElement("mesh")->GetElement("scale")->Set(_scale);

      visualElem = visualElem->GetNextElement("visual");
    }
  }
}

//////////////////////////////////////////////////
void Link::UpdateVisualMsg()
{
  // TODO: this shouldn't be in the physics sim
  if (this->sdf->HasElement("visual"))
  {
    sdf::ElementPtr visualElem = this->sdf->GetElement("visual");
    while (visualElem)
    {
      msgs::Visual msg = msgs::VisualFromSDF(visualElem);

      bool newVis = true;
      std::string linkName = this->GetScopedName();

      // update visual msg if it exists
      for (auto &iter : this->visuals)
      {
        std::string visName = linkName + "::" +
            visualElem->Get<std::string>("name");
        if (iter.second.name() == visName)
        {
          iter.second.mutable_geometry()->CopyFrom(msg.geometry());
          newVis = false;
          break;
        }
      }

      // add to visual msgs if not found.
      if (newVis)
      {
        std::string visName = this->GetScopedName() + "::" + msg.name();
        msg.set_name(visName);
        msg.set_id(physics::getUniqueId());
        msg.set_parent_name(this->GetScopedName());
        msg.set_parent_id(this->GetId());
        msg.set_is_static(this->IsStatic());
        msg.set_type(msgs::Visual::VISUAL);

        auto iter = this->visuals.find(msg.id());
        if (iter != this->visuals.end())
          gzthrow(std::string("Duplicate visual name[")+msg.name()+"]\n");
        this->visuals[msg.id()] = msg;
      }

      visualElem = visualElem->GetNextElement("visual");
    }
  }
}

/////////////////////////////////////////////////
double Link::GetWorldEnergyPotential() const
{
  // compute gravitational potential energy for link CG location
  // use origin as reference position
  // E = -m g^T z
  double m = this->GetInertial()->GetMass();
  auto g = this->GetWorld()->Gravity();
  math::Vector3 z = this->GetWorldCoGPose().pos;
  return -m * g.Dot(z.Ign());
}

/////////////////////////////////////////////////
double Link::GetWorldEnergyKinetic() const
{
  double energy = 0.0;

  // compute linear kinetic energy
  // E = 1/2 m v^T v
  {
    double m = this->GetInertial()->GetMass();
    math::Vector3 v = this->GetWorldCoGLinearVel();
    energy += 0.5 * m * v.Dot(v);
  }

  // compute angular kinetic energy
  // E = 1/2 w^T I w
  {
    math::Vector3 w = this->GetWorldAngularVel();
    math::Matrix3 I = this->GetWorldInertiaMatrix();
    energy += 0.5 * w.Dot(I * w);
  }

  return energy;
}

/////////////////////////////////////////////////
double Link::GetWorldEnergy() const
{
  return this->GetWorldEnergyPotential() + this->GetWorldEnergyKinetic();
}

/////////////////////////////////////////////////
void Link::MoveFrame(const math::Pose &_worldReferenceFrameSrc,
                     const math::Pose &_worldReferenceFrameDst)
{
  math::Pose targetWorldPose = (this->GetWorldPose() - _worldReferenceFrameSrc)
    + _worldReferenceFrameDst;
  this->SetWorldPose(targetWorldPose);
  this->SetWorldTwist(math::Vector3(0, 0, 0), math::Vector3(0, 0, 0));
}

/////////////////////////////////////////////////
bool Link::FindAllConnectedLinksHelper(const LinkPtr &_originalParentLink,
  Link_V &_connectedLinks, bool _fistLink)
{
  // debug
  // std::string pn;
  // if (_originalParentLink) pn = _originalParentLink->GetName();
  // gzerr << "subsequent call to find connected links: "
  //       << " parent " << pn
  //       << " this link " << this->GetName() << "\n";

  // get all child joints from this link
  Link_V childLinks = this->GetChildJointsLinks();

  // gzerr << "debug: child links are: ";
  // for (Link_V::iterator li = childLinks.begin();
  //                       li != childLinks.end(); ++li)
  //   std::cout << (*li)->GetName() << " ";
  // std::cout << "\n";

  // loop through all joints where this link is a parent link of the joint
  for (Link_V::iterator li = childLinks.begin();
                        li != childLinks.end(); ++li)
  {
    // gzerr << "debug: checking " << (*li)->GetName() << "\n";

    // check child link of each child joint recursively
    if ((*li).get() == _originalParentLink.get())
    {
      // if parent is a child, failed search to find a nice subset of links
      gzdbg << "we have a loop! cannot find nice subset of connected links,"
            << " this link " << this->GetName() << " connects back to"
            << " parent " << _originalParentLink->GetName() << ".\n";
      _connectedLinks.clear();
      return false;
    }
    else if (this->ContainsLink(_connectedLinks, (*li)))
    {
      // do nothing
      // gzerr << "debug: do nothing with " << (*li)->GetName() << "\n";
    }
    else
    {
      // gzerr << "debug: add and recurse " << (*li)->GetName() << "\n";
      // add child link to list
      _connectedLinks.push_back((*li));

      // recursively check if child link has already been checked
      // if it returns false, it looped back to parent, mark flag and break
      // from current for-loop.
      if (!(*li)->FindAllConnectedLinksHelper(_originalParentLink,
        _connectedLinks))
      {
        // one of the recursed link is the parent link
        return false;
      }
    }
  }

  /// \todo: later we can optimize loop below by merging and using a flag.

  // search parents, but if this is the first search, keep going, otherwise
  // flag failure
  // get all parent joints from this link
  Link_V parentLinks = this->GetParentJointsLinks();

  // loop through all joints where this link is a parent link of the joint
  for (Link_V::iterator li = parentLinks.begin();
                        li != parentLinks.end(); ++li)
  {
    // check child link of each child joint recursively
    if ((*li).get() == _originalParentLink.get())
    {
      if (_fistLink)
      {
        // this is the first child link, simply skip if the parent is
        // the _originalParentLink
      }
      else
      {
        // if parent is a child, failed search to find a nice subset of links
        gzdbg << "we have a loop! cannot find nice subset of connected links,"
              << " this link " << this->GetName() << " connects back to"
              << " parent " << _originalParentLink->GetName() << ".\n";
        _connectedLinks.clear();
        return false;
      }
    }
    else if (this->ContainsLink(_connectedLinks, (*li)))
    {
      // do nothing
    }
    else
    {
      // add parent link to list
      _connectedLinks.push_back((*li));

      // recursively check if parent link has already been checked
      // if it returns false, it looped back to parent, mark flag and break
      // from current for-loop.
      if (!(*li)->FindAllConnectedLinksHelper(_originalParentLink,
        _connectedLinks))
      {
        // one of the recursed link is the parent link
        return false;
      }
    }
  }

  return true;
}

/////////////////////////////////////////////////
bool Link::ContainsLink(const Link_V &_vector, const LinkPtr &_value)
{
  for (Link_V::const_iterator iter = _vector.begin();
       iter != _vector.end(); ++iter)
  {
    if ((*iter).get() == _value.get())
      return true;
  }
  return false;
}

/////////////////////////////////////////////////
msgs::Visual Link::GetVisualMessage(const std::string &_name) const
{
  msgs::Visual result;

  Visuals_M::const_iterator iter;
  for (iter = this->visuals.begin(); iter != this->visuals.end(); ++iter)
    if (iter->second.name() == _name)
      break;

  if (iter != this->visuals.end())
    result = iter->second;

  return result;
}

//////////////////////////////////////////////////
void Link::SetStatic(const bool &_static)
{
  if (!_static && !this->wrenchSub)
  {
    std::string topicName = "~/" + this->GetScopedName() + "/wrench";
    boost::replace_all(topicName, "::", "/");
    this->wrenchSub = this->node->Subscribe(topicName, &Link::OnWrenchMsg,
        this);
  }
  else if (_static)
  {
    this->wrenchSub.reset();
  }

  Entity::SetStatic(_static);
}

//////////////////////////////////////////////////
void Link::OnWrenchMsg(ConstWrenchPtr &_msg)
{
  // Sanity check
  if (this->IsStatic())
  {
    gzerr << "Link [" << this->GetName() <<
        "] received a wrench message, but it is static." << std::endl;
    return;
  }

  boost::mutex::scoped_lock lock(this->wrenchMsgMutex);
  this->wrenchMsgs.push_back(*_msg);
}

//////////////////////////////////////////////////
void Link::ProcessWrenchMsg(const msgs::Wrench &_msg)
{
  // Sanity check
  if (this->IsStatic())
  {
    gzerr << "Link [" << this->GetName() <<
        "] received a wrench message, but it is static." << std::endl;
    return;
  }

  math::Vector3 pos = math::Vector3::Zero;
  if (_msg.has_force_offset())
  {
    pos = msgs::ConvertIgn(_msg.force_offset());
  }

  const ignition::math::Vector3d force = msgs::ConvertIgn(_msg.force());
  this->AddLinkForce(force, pos);

  const ignition::math::Vector3d torque = msgs::ConvertIgn(_msg.torque());
  this->AddRelativeTorque(torque);
}

//////////////////////////////////////////////////
void Link::LoadBattery(sdf::ElementPtr _sdf)
{
  common::BatteryPtr battery(new common::Battery());
  battery->Load(_sdf);
  this->batteries.push_back(battery);
}

/////////////////////////////////////////////////
const ignition::math::Vector3d Link::RelativeWindLinearVel() const
{
  return this->GetWorldPose().Ign().Rot().Inverse().RotateVector(
      this->windLinearVel);
}

/////////////////////////////////////////////////
void Link::RegisterIntrospectionItems()
{
  auto uri = this->URI();

  // Callbacks.
  auto fLinkPose = [this]()
  {
    return this->GetWorldPose().Ign();
  };

  auto fLinkLinVel = [this]()
  {
    return this->GetWorldLinearVel().Ign();
  };

  auto fLinkAngVel = [this]()
  {
    return this->GetWorldAngularVel().Ign();
  };

  auto fLinkLinAcc = [this]()
  {
    return this->GetWorldLinearAccel().Ign();
  };

  auto fLinkAngAcc = [this]()
  {
    return this->GetWorldAngularAccel().Ign();
  };

  // Register items.
  common::URI poseURI(uri);
  poseURI.Query().Insert("p", "pose3d/world_pose");
  this->introspectionItems.push_back(poseURI);
  gazebo::util::IntrospectionManager::Instance()->Register
      <ignition::math::Pose3d>(poseURI.Str(), fLinkPose);

  common::URI linVelURI(uri);
  linVelURI.Query().Insert("p", "vector3d/world_linear_velocity");
  this->introspectionItems.push_back(linVelURI);
  gazebo::util::IntrospectionManager::Instance()->Register
      <ignition::math::Vector3d>(linVelURI.Str(), fLinkLinVel);

  common::URI angVelURI(uri);
  angVelURI.Query().Insert("p", "vector3d/world_angular_velocity");
  this->introspectionItems.push_back(angVelURI);
  gazebo::util::IntrospectionManager::Instance()->Register
      <ignition::math::Vector3d>(angVelURI.Str(), fLinkAngVel);

  common::URI linAccURI(uri);
  linAccURI.Query().Insert("p", "vector3d/world_linear_acceleration");
  this->introspectionItems.push_back(linAccURI);
  gazebo::util::IntrospectionManager::Instance()->Register
      <ignition::math::Vector3d>(linAccURI.Str(), fLinkLinAcc);

  common::URI angAccURI(uri);
  angAccURI.Query().Insert("p", "vector3d/world_angular_acceleration");
  this->introspectionItems.push_back(angAccURI);
  gazebo::util::IntrospectionManager::Instance()->Register
      <ignition::math::Vector3d>(angAccURI.Str(), fLinkAngAcc);
}

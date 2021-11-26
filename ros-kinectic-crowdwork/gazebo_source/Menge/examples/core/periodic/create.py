import random
import rospkg
import rospy
import yaml

import numpy as np

from lxml import etree
from lxml.etree import Element
from copy import deepcopy
from scenarios import Scenarios


class EnvironmentCreator(object):
    def __init__(self, num_agents=100, env_size=15.):
        self.num_agents = num_agents
        self.env_size = env_size
        rospack = rospkg.RosPack()
        self.actor_pkg_path = rospack.get_path("actor_services")
        world_name = "empty.world"
        self.tree = etree.parse(self.actor_pkg_path+'/worlds/'+world_name)

        self.scenarios = Scenarios(num_agents=num_agents, num_obstacles=0, agent_size=0.12, env_size=env_size)
        
    def run(self):
        world = self.tree.getroot().getchildren()[0]
        starts, goals = self.scenarios.random_scene()
        # starts, goals = self.scenarios.circle_scene_uniform()
        for item in range(self.num_agents):
            actor = Element("instanced_actor", name="actor"+str(item))

            skin = Element("skin")
            skin_fn = Element("filename")
            skin_fn.text="/home/tingxfan/lib/gazebo/media/models/Gamechar-male.mesh"
            skin_scale = Element("scale")
            skin_scale.text = "1"
            skin.append(skin_fn)
            skin.append(skin_scale)
            actor.append(skin)

            pose = Element("pose")
            x = str(starts[item][0])
            y = str(starts[item][1])
            pose.text = x+" "+y+" "+"1.02 0 0 0"
            actor.append(pose)

            animation = Element("animation", name="walking")
            animate_fn = Element("filename")
            animate_fn.text = "/home/tingxfan/lib/gazebo/media/models/Gamechar-male.skeleton"
            interpolate_x = Element("interpolate_x")
            interpolate_x.text = "true"
            animate_scale = Element("scale")
            animate_scale.text = "1"
            animation.append(animate_fn)
            animation.append(animate_scale)
            animation.append(interpolate_x)
            actor.append(animation)

            # static = Element("static")
            # static.text = "0"
            # actor.append(static)

            # link = Element("link", name="link"+str(item))
            # visual = Element("visual", name="visual"+str(item))
            # geometry = Element("geometry")
            # box = Element("box")
            # box_size = Element("size")
            # box_size.text = "0.2 0.2 0.2"
            # box.append(box_size)
            # geometry.append(box)
            # visual.append(geometry)
            # link.append(visual)
            # actor.append(link)

            plugin = Element("plugin", name="actor_"+str(item)+"_plugin", filename="/home/tingxfan/lib/actor_plugin/lib/libactorplugin_ros.so")
            
            target = Element("target")
            x = str(goals[item][0])
            y = str(goals[item][1])
            target.text =  x+" "+y+" "+"1.02"

            target_weight = Element("target_weight")
            target_weight.text = "1.15"

            obstacle_weight = Element("obstacle_weight")
            obstacle_weight.text = "1.8"

            animation_factor = Element("animation_factor")
            animation_factor.text = "5.1"

            speed = Element("speed")
            speed.text = "0.6"

            ignore_obstacle = Element("ignore_obstacles")
            model_ground_plane = Element("model")
            model_ground_plane.text = "ground_plane"
            ignore_obstacle.append(model_ground_plane)

            plugin.append(target)
            # plugin.append(target_weight)
            # plugin.append(obstacle_weight)
            # plugin.append(animation_factor)
            plugin.append(speed)
            plugin.append(ignore_obstacle)

            actor.append(plugin)

            world.append(actor)

        self.tree.write(self.actor_pkg_path+'/worlds/ped_world.world', pretty_print=True, xml_declaration=True, encoding="utf-8")


if __name__ == "__main__":
    creator = EnvironmentCreator()
    creator.run()


















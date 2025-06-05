docker run -it \
-v /home/dawei/code/:/code \
-v /home/dawei/limx_ws/:/limx_ws \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
--privileged=true \
-v $HOME/.Xauthority:/root/.Xauthority \
-v /dev:/dev \
--device-cgroup-rule "c 81:* rmw" \
--device-cgroup-rule "c 189:* rmw" \
--net=host \
wangdawei1996/jetson_ros_noetic:v2 bash
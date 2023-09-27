docker run -it --gpus all \
-v $HOME/code/:/home/code \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
-v $HOME/.Xauthority:/root/.Xauthority \
--shm-size=10.01gb \
wangdawei1996/ray_sumo:beta6 bash

docker run -it --gpus all \
-v $HOME/code/TrafficRW:/tf/code \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
-v $HOME/.Xauthority:/root/.Xauthority \
wangdawei1996/rw:v1.1 bash
docker run -it --gpus all \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
-v $HOME/.Xauthority:/root/.Xauthority \
--net=host \
wangdawei1996/crowdwork:v1.0 bash
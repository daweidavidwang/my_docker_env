docker run -it --gpus all \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
-v $HOME/.Xauthority:/root/.Xauthority \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
--net=host  \
wangdawei1996/flow-latest:v1.0 bash
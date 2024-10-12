docker run -it --gpus all \
-v $HOME/code/:/code \
--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
--env="QT_X11_NO_MITSHM=1" \
--env="DISPLAY" \
--privileged=true \
-v $HOME/.Xauthority:/root/.Xauthority \
wangdawei1996/3dgs:latest bash
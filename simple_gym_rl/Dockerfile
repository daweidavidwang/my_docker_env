FROM pytorch/pytorch:2.0.1-cuda11.7-cudnn8-runtime

RUN apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get install -y python3-opencv

RUN pip install gym tianshou opencv-python

RUN pip install "gymnasium[atari, accept-rom-license]"


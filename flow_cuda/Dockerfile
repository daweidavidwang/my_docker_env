FROM fywu85/flow-desktop:latest
SHELL ["/bin/bash", "--login", "-c"]
RUN sudo apt-get update
RUN rm -r flow && \
    git clone https://github.com/daweidavidwang/flow.git && \
    cd flow && \
    conda env create -f environment.yml &&\
    conda init bash && \
    conda activate flow &&\
    python setup.py develop && \
    pip install -e .
RUN mkdir -p /headless/ray_results

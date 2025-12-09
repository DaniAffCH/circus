FROM ros:jazzy-ros-core

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libtinyxml2-dev && \
    rm -rf /var/lib/apt/lists/*

CMD ["bash", "-c", "/app/fake_framework/fake_framework & /app/BridgeSubscriber/BridgeSubscriber"]

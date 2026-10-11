# Local dev image: Chatterino's official Ubuntu build image plus ccache, ninja
# and the two test servers the test suite expects (see .github/workflows/test.yml).
FROM ghcr.io/chatterino/chatterino2-build-ubuntu-26.04@sha256:d016195d9be109539758aa4ce2a6c1813807e8d5d893586b46ef5da82444bf37

ARG HTTPBOX_TAG=v0.2.1
ARG HTTPBOX_SHA256=bbb4de44c60f531dc4e37cd451c169837360239acd1abf5ffe70860d98adc5fa
ARG PUBSUB_TAG=v1.0.12
ARG PUBSUB_SHA256=effadf8c485dcfcde65ce6d53cf467e0813a2773f47640cfd76d59dfbfe6ab50

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ccache ninja-build curl xz-utils ca-certificates python3 \
    && rm -rf /var/lib/apt/lists/*

RUN set -eu; cd /tmp; \
    curl -fsSL -o httpbox.tar.xz "https://github.com/Chatterino/httpbox/releases/download/${HTTPBOX_TAG}/httpbox-x86_64-unknown-linux-gnu.tar.xz"; \
    echo "${HTTPBOX_SHA256}  httpbox.tar.xz" | sha256sum -c -; \
    tar -xJf httpbox.tar.xz; \
    mv httpbox-x86_64-unknown-linux-gnu/httpbox /usr/local/bin/; \
    curl -fsSL -o pubsub.tar.gz "https://github.com/Chatterino/twitch-pubsub-server-test/releases/download/${PUBSUB_TAG}/server-${PUBSUB_TAG}-linux-amd64.tar.gz"; \
    echo "${PUBSUB_SHA256}  pubsub.tar.gz" | sha256sum -c -; \
    mkdir -p /opt/pubsub-server; tar -xzf pubsub.tar.gz -C /opt/pubsub-server; \
    chmod -R a+rX /opt/pubsub-server; \
    rm -rf /tmp/*

# Dockerfile
FROM ubuntu:22.04

# Install tools
COPY .pre-commit-config.yaml .

RUN apt-get update && apt-get install -y \
    clang-tidy \
    oclint \
    uncrustify \
    cppcheck \
    && rm -rf /var/lib/apt/lists/*
    git init . && pre-commit install-hooks

# Set the working directory
WORKDIR /src

# Default command (can be overridden)
CMD ["echo", "Custom Dockerfile ready!"]
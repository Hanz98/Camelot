# Dockerfile
FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman-key --init

RUN pacman -S --noconfirm base-devel cmake clang
RUN  pacman -S --noconfirm cppcheck git

WORKDIR /src

COPY .pre-commit-config.yaml .

RUN git init . && pre-commit install-hooks

# Default command (can be overridden)
CMD ["echo", "Custom Dockerfile ready!"]
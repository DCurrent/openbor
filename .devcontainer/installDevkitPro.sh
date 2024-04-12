#!/usr/bin/env bash

curl -L https://apt.devkitpro.org/install-devkitpro-pacman > install-devkitpro-pacman || exit 1
chmod +x install-devkitpro-pacman || exit 1

./install-devkitpro-pacman << END_OF_INPUT
y
END_OF_INPUT

rm install-devkitpro-pacman || exit 1
ln -s /proc/self/mounts /etc/mtab

/opt/devkitpro/pacman/bin/pacman -S wii-dev << END_OF_INPUT

y
END_OF_INPUT


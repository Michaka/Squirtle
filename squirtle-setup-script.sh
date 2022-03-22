#!/bin/bash/
#######################################
# Bash script to install apps on a new system (Ubuntu)
#######################################

## Update packages and Upgrade system
sudo apt-get update -y

## Git ##
echo '###Installing Git..'
sudo apt-get install git -y

# Git Configuration
echo '###Congigure Git..'

echo "Enter the Global Username for Git:";
read GITUSER;
git config --global user.name "$GITUSER"

echo "Enter the Global Email for Git:";
read GITEMAIL;
git config --global user.email "$GITEMAIL"

echo 'Git has been configured!'
git config --list

# Removing old install of docker
sudo apt-get remove docker docker-engine docker.io containerd runc

# Update the apt package index and install packages to allow apt to use a repository over HTTPS
sudo apt-get install ca-certificates curl gnupg lsb-release

# Add Docker’s official GPG key:
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | \
    sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo \
  "deb [arch=$(dpkg --print-architecture) \
  signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] \
  https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# Install Docker engine
sudo apt-get update
sudo apt-get install docker-ce docker-ce-cli containerd.io

# Download the current stable release of Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/download/1.29.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose

#Apply executable permissions to the binary
sudo chmod +x /usr/local/bin/docker-compose

#Create the docker group.
sudo groupadd docker

#Add your user to the docker group.
sudo usermod -aG docker akatim

newgrp docker

# Configure docker run on boot
sudo systemctl enable docker.service
sudo systemctl enable containerd.service

mkdir /opt/homeassistant
mkdir /opt/portainer
mkdir /opt//mosquitto/config

git clone https://github.com/Michaka/Squirtle.git ~/Squirtle

docker-compose -f ~Squirtle/docker-compose.yaml up -d

cp -rf ~/Squirtle/mosquitto/config /opt/mosquitto/config

sudo apt-get update -y

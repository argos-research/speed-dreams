
#######################
#
# This is a provision script
# it will be called once when the vagrant vm is first provisioned
# If you have commands that you want to run always please have a
# look at the bootstrap.sh script
#
# Contributor: Bernhard Blieninger
######################

sudo apt-get update -q=2
sudo apt-get install -q=2 make cmake g++ libogg-dev libvorbis-dev libsdl1.2-dev libexpat1-dev libjpeg9-dev libplib-dev libopenal-dev libenet-dev

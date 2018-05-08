
#######################
#
# This is a provision script
# it will be called once when the vagrant vm is first provisioned
# If you have commands that you want to run always please have a
# look at the bootstrap.sh script
#
# Contributor: Bernhard Blieninger
######################

sudo apt-get -qy update
sudo apt-get -qy install cmake g++ libopenal-dev libogg-dev libvorbis-dev libenet-dev zlib1g-dev libpng-dev libjpeg-dev libsdl2-dev libopenscenegraph-dev libexpat1-dev libplib-dev

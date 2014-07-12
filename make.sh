#!/bin/bash
DIR="$(cd "$( dirname "${BASH_SOURCE[0]}")" &&pwd)"

#MAKE CLEAN DE LOS PROYECTOS

cd $DIR/Commons
make clean

cd $DIR/parser
make clean

cd $DIR/UMV/Debug
make clean

cd $DIR/KERNEL/Debug
make clean

cd $DIR/PROGRAMA/Debug
make clean

cd $DIR/CPU/Debug
make clean


# MAKE DE LOS PROYECTOS

cd $DIR/Commons
make all
make install

cd $DIR/parser
make all
make install

cd $DIR/UMV/Debug
make

cd $DIR/KERNEL/Debug
make

cd $DIR/PROGRAMA/Debug
make

cd $DIR/CPU/Debug
make


# CREACION DISCO.BIN EN /home/utnso
dd if=/dev/urandom of=/home/utnso/disco.bin bs=1024 count=102400

# Se hace el export en el .bashrc
echo >> /home/utnso/.bashrc
echo export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$DIR/Commons >> /home/utnso/.bashrc
echo export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$DIR/parser/Debug >> /home/utnso/.bashrc

#Se hace EL EXPORT DE LINKEO DE LA COMMONS EN CADA PROYECTO QUE LA USE
export LD_LIBRARY_PATH=$DIR/Commons:$DIR/UMV/Debug
export LD_LIBRARY_PATH=$DIR/Commons:$DIR/KERNEL/Debug
export LD_LIBRARY_PATH=$DIR/Commons:$DIR/PROGRAMA/Debug
export LD_LIBRARY_PATH=$DIR/Commons:$DIR/CPU/Debug
export LD_LIBRARY_PATH=$DIR/parser:$DIR/UMV/Debug
export LD_LIBRARY_PATH=$DIR/parser:$DIR/KERNEL/Debug
export LD_LIBRARY_PATH=$DIR/parser:$DIR/CPU/Debug

#Crear link symbolic
echo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop

cd $DIR

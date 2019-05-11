# Linux Rootkit


## Description

This project is a Linux Kernel Module (LKM) Rootkit for educational purposes. For a 
complete introduction, see the talk presented at Toorcamp 2018 or Thotcon 2019. This 
rootkit is deliberately simplified to teach the basics of rootkit development. It 
demonstrates the following subversive techniques: 

 * Starts a kernel thread which executes a userland process as root periodically
 * Dynamically finds the runtime address of the syscall table using kallsyms
 * Demonstrates writing to read-only pages of memory using CR0 and PTE methods
 * Hijacks the execve system call
 * Hides from procfs and sysfs and lsmod

## Presentation

This is the code associated with the presentation from the *Toorcamp 2018* and *Thotcon 2019*.

 * [Thotcon 2019 Slides](https://github.com/rootfoo/pub/blob/master/Developing%20a%20Linux%20Rootkit%20-%20Thotcon%20-%202019-05-03.pdf)


## Compiling, Loading

Use `dmesg -w` to see the diagnostic output. After loading, experiment running various
shell commands to see execve being hijacked in real time.


```
make
sudo insmod rootkit.ko
lsmod
sudo rmmod rootkit.ko
```

## Status

This project was last developed and tested on Ubuntu 18.04 (Linux kernel 4.15.0-48-generic).


## Installing binary modules

Generally you should always compile kernel modules on the same host they will be installed
on. However, it is possible to compile it offline and install it on a target system. Note 
that it must be compiled with the same kernel version and Linux distribution for this to 
work. The script below outlines the process.

```
#!/bin/bash
NAME="rootkit"
DIR="/lib/modules/`uname -r`/kernel/drivers/$NAME/"
sudo mkdir -p $DIR
sudo cp $NAME.ko $DIR
sudo depmod
sudo bash -c 'cat << EOF > /etc/modules-load.d/rootkit.conf
rootkit
EOF'
```


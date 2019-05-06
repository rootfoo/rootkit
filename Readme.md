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
 

## Compile / Load

Use `dmesg -w` to see the diagnostic output. After loading, experiment running various
shell commands to see execve being hijacked in real time.


```
make
sudo insmod rootkit.ko
lsmod
sudo rmmod rootkit.ko
```


## Install at boot (optional) 

The following script installs the rootkit to load at boot

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


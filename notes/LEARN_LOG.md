# xdp learning log

## day 1: setting up stuff

### devcontainers

- had to set up some cursed combo of stuff. i'm running [colima](https://github.com/abiosoft/colima) bc mac.
- running a devcontainer on top to enable faster dev in vscode
- devcontainers are docker containers that serve as dev envs
    - i chose debian as a base image bc of the packages
- install all the good dependencies here inside the container by ssh'ing in: https://github.com/xdp-project/xdp-tutorial/blob/main/setup_dependencies.org

## setting up veth interfaces

ssh into the devcontainer. it has the `NET_ADMIN` capability (as defined in [`devcontainer.json`](.devcontainer/devcontainer.json)) so it should be able to do this stuff.

in order to test things, we bind to fake interfaces (veth interfaces).

```bash
ip link add yap0 type veth peer name yap1 
ip link set yap0 up && ip link set yap1 up
```

running `ifconfig` should give you the following:

```bash
[...]
yap0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::acfa:76ff:fe45:8457  prefixlen 64  scopeid 0x20<link>
        ether ae:fa:76:45:84:57  txqueuelen 1000  (Ethernet)
        RX packets 3  bytes 266 (266.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 3  bytes 266 (266.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

yap1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::241a:61ff:fe76:77a  prefixlen 64  scopeid 0x20<link>
        ether 26:1a:61:76:07:7a  txqueuelen 1000  (Ethernet)
        RX packets 3  bytes 266 (266.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 3  bytes 266 (266.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

let's assign them IPs:

```bash
ip addr add 192.168.1.1/24 dev yap0
ip addr add 192.168.1.2/24 dev yap1
```

now let's yap on these interfaces :) 
install netcat using `apt install netcat-openbsd`.
on one terminal listen on port 4444 using `netcat -l 4444` 
and on the other run `nc 192.168.1.2 4444` to transmit whatever.
make sure it appears on the other side.


## cool tutorial

https://github.com/xdp-project/xdp-tutorial/blob/main/packet01-parsing/README.org
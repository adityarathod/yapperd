# xdp learning log

## day 1: setting up stuff

### devcontainers

- had to set up some cursed combo of stuff. i'm running [colima](https://github.com/abiosoft/colima) bc mac.
- running a devcontainer on top to enable faster dev in vscode
- devcontainers are docker containers that serve as dev envs
    - i chose ubuntu as a base image bc of the packages
- install all the good dependencies here inside the container by ssh'ing in: https://github.com/xdp-project/xdp-tutorial/blob/main/setup_dependencies.org

### setting up veth interfaces

ssh into the devcontainer. it runs privileged so it should be able to do this stuff.

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

## day 2: compiling things

### building a simple program

let's ssh into the container and build the `xdp_pass.c` program:

```bash
$ make
clang -O2 -target bpf -c -Wall -c -o xdp_pass.o xdp_pass.c
$ ls
Makefile  xdp_pass.c  xdp_pass.o
```

you can examine the loadable object. notice the xdp section!

```bash
$ readelf -a xdp_pass.o
```
```text
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Linux BPF
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          216 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         6
  Section header string table index: 1

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .strtab           STRTAB           0000000000000000  00000099
       000000000000003d  0000000000000000           0     0     1
  [ 2] .text             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  AX       0     0     4
  [ 3] xdp               PROGBITS         0000000000000000  00000040
       0000000000000010  0000000000000000  AX       0     0     8
  [ 4] .llvm_addrsig     LOOS+0xfff4c03   0000000000000000  00000098
       0000000000000001  0000000000000000   E       5     0     1
  [ 5] .symtab           SYMTAB           0000000000000000  00000050
       0000000000000048  0000000000000018           1     2     8
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), p (processor specific)

There are no section groups in this file.

There are no program headers in this file.

There is no dynamic section in this file.

There are no relocations in this file.

The decoding of unwind sections for machine type Linux BPF is not currently supported.

Symbol table '.symtab' contains 3 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS xdp_pass.c
     2: 0000000000000000    16 FUNC    GLOBAL DEFAULT    3 xdp_pass

```

### loading in the program

we can use our friend `xdp-loader`!

```bash
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
yap1                   <No XDP program loaded!>
yap0                   <No XDP program loaded!>
eth0                   <No XDP program loaded!>
```

load in our XDP program:

```bash
$ xdp-loader load -m native -s xdp yap0 xdp_pass.o
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
yap1                   <No XDP program loaded!>
yap0                   xdp_dispatcher    native   162  4d7e87c0d30db711
 =>              50     xdp_pass                  171  3b185187f1855c4c  XDP_PASS
eth0                   <No XDP program loaded!>
```

the netcat stuff from earlier should still work.

to unload:

```bash
$ xdp-loader unload --all yap0
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
yap1                   <No XDP program loaded!>
yap0                   <No XDP program loaded!>
eth0                   <No XDP program loaded!>
```


## resources
- cool tutorial: https://github.com/xdp-project/xdp-tutorial/blob/main/packet01-parsing/README.org
- useful docs: https://docs.ebpf.io/
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

use the mounted xdp tutorial testbed in `/tutorial/testenv` or just use `t` in the terminal.

create a test env:

```bash
$ t setup --legacy-ip
Setting up new environment 'xdptut-6cfa'
Setup environment 'xdptut-6cfa' with peer ip fc00:dead:cafe:1::2 and 10.11.1.2.
Waiting for interface configuration to settle...

Running ping from inside test environment:

PING fc00:dead:cafe:1::1 (fc00:dead:cafe:1::1) 56 data bytes
64 bytes from fc00:dead:cafe:1::1: icmp_seq=1 ttl=64 time=0.111 ms

--- fc00:dead:cafe:1::1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.111/0.111/0.111/0.000 ms
```

a small architecture diagram:

```
+-------------------------------------------+
| your container                            |
|                                           |
|                    +--------------------+ |
|                    | namespace          | |
|                    |                    | |
|         packets    |                    | |
|       +------------+-veth0 interface    | |
|       v            | ---------------    | |
|  xdptut-4fc0       |                    | |
|  interface         |                    | |
|  -----------       |                    | |
|           ^        +--------------------+ |
|           |                               |
|           |                               |
|           +- this will be your            |
|              XDP attachment point         |
|                                           |
+-------------------------------------------+
```

the `xdptut-xxxx` interface is called the "outer" interface.


in a separate container, enter the test environment:

```bash
$ t enter
$ ping 10.11.1.1
PING 10.11.1.1 (10.11.1.1) 56(84) bytes of data.
64 bytes from 10.11.1.1: icmp_seq=1 ttl=64 time=0.392 ms
64 bytes from 10.11.1.1: icmp_seq=2 ttl=64 time=0.203 ms
64 bytes from 10.11.1.1: icmp_seq=3 ttl=64 time=0.294 ms
64 bytes from 10.11.1.1: icmp_seq=4 ttl=64 time=0.047 ms
^C
--- 10.11.1.1 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3083ms
rtt min/avg/max/mdev = 0.047/0.234/0.392/0.126 ms
```

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
xdptut-4fc0            <No XDP program loaded!>
eth0                   <No XDP program loaded!>
```

load in our XDP program:

```bash
$ xdp-loader load -m native -s xdp xdptut-4fc0 xdp_pass.o
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
xdptut-4fc0            xdp_dispatcher    native   162  4d7e87c0d30db711
 =>              50     xdp_pass                  171  3b185187f1855c4c  XDP_PASS
eth0                   <No XDP program loaded!>
```

use 


to unload:

```bash
$ xdp-loader unload --all xdptut-4fc0
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
xdptut-4fc0            <No XDP program loaded!>
eth0                   <No XDP program loaded!>
```

now try the same ping stuff from before and it should just work :)

## day 3: dropping packets indiscriminately

aka 100% accurate DDoS mitigation yippee

compile `xdp_drop.o`:

```bash
$ make xdp_drop.o
```

load it in onto the outer interface:

```bash
$ xdp-loader load -m native -s xdp xdptut-6cfa xdp_drop.o
$ xdp-loader status
CURRENT XDP PROGRAM STATUS:

Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
xdptut-6cfa            xdp_dispatcher    native   112  4d7e87c0d30db711
 =>              50     xdp_drop                  121  57cd311f2e27366b  XDP_PASS
eth0                   <No XDP program loaded!>
```

and `t enter` the network namespace and run a ping:

```bash
$ ping 10.11.1.1
PING 10.11.1.1 (10.11.1.1) 56(84) bytes of data.
^C
--- 10.11.1.1 ping statistics ---
8 packets transmitted, 0 received, 100% packet loss, time 7191ms
```

it doesn't work! as expected.

note that `ping`s from outside the namespace WILL work:

```bash
$ ping 10.11.1.1
PING 10.11.1.1 (10.11.1.1) 56(84) bytes of data.
64 bytes from 10.11.1.1: icmp_seq=1 ttl=64 time=0.301 ms
64 bytes from 10.11.1.1: icmp_seq=2 ttl=64 time=0.074 ms
64 bytes from 10.11.1.1: icmp_seq=3 ttl=64 time=0.068 ms
64 bytes from 10.11.1.1: icmp_seq=4 ttl=64 time=0.186 ms
^C
--- 10.11.1.1 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3077ms
rtt min/avg/max/mdev = 0.068/0.157/0.301/0.095 ms
```

this is because XDP process packets on _ingress_ into the `xdptut-xxxx` interface from the network
namespace, but not at _egress_ into the namespace.

## day 4: dropping packets selectively (packet parsing)

things get a bit weird here. we're using some linux headers to help with some stuff, but otherwise we are completely parsing the packet from raw bytes :)

the code for this day is `xdp_drop_ipv6_non_tcp.c` (a mouthful, i know).

the key thing to understand is that we parse the packet iteratively, moving from L2 (Ethernet) up the layers in the OSI model.
to make things easier, we maintain a cursor containing a `void *` (fancy word for "idfk which type") of our current position and of the end, that we pass around everywhere. we increment as we move up the stack. (idk if passing around the end in the cursor is standard, but i liked how it looked so idrc)

there is a generalized process for parsing packets that i've gathered.

1. initialize pointer of header type, set to the current position
2. perform header struct bounds check (see below)
3. compute total header size (if variable)
4. advance cursor by total header size
5. set function-external struct pointers to the pointer initialized in step 1
6. return something from the header, usually something to use to determine what you want to do next

what does the header struct bounds check look like? here's an example from ethernet header parsing:

```c
if (eth + 1 > (struct ethhdr *)nh->end)
{
     return -1;
}
```

this is saying that if the current location plus the size of 1 instance of `eth` (which is a `ethhdr` in this case and of fixed size) is beyond the end of the packet buffer, we know that we will be out of bounds if we try to access things, so we fail fast.

computing the total header size will depend on the header type. for instance, for IPv4, we use the IHL field to determine the combined size of the standard header + options.

anyways, this is what the program does:
- parses L2 (Ethernet) headers and determines if the L3 protocol (EtherType) is IPv4.
     - if not, we immediately return a `XDP_DROP` decision.
- otherwise, we parse L3 (IPv4) headers and determine if the L4 protocol is TCP.
     - if not, we immediately return a `XDP_DROP` decision.
- otherwise, we return an `XDP_PASS` decision.

if we comment out the IPv4 parts, we can test just the L3 protocol checking logic by `ping`ing the IPv6 address inside the namespace:

```bash
$ ping fc00:dead:cafe:1::1
PING fc00:dead:cafe:1::1 (fc00:dead:cafe:1::1) 56 data bytes
^C
--- fc00:dead:cafe:1::1 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 1037ms
```

with the L4 protocol checking logic enabled, we can test the happy path by using `nc -s 10.11.1.1 -l -p 8080 -v` outside the namespace to listen, and `nc -u 10.11.1.1 8080 -v` to connect. (this should work and pass traffic as expected.)

we can add `-u` to test the UDP version to ensure those packets are dropped.

## resources
- cool tutorial: https://github.com/xdp-project/xdp-tutorial/blob/main/packet01-parsing/README.org
- useful docs: https://docs.ebpf.io/
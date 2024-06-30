# matebook-16s-sound-fix

This is my first attempt to write a linux kernel module.

The goal of this project is to implement the workaround for a well-known bug 
with sound cards SoC pins connection leading to the nessesity to send hda-verbs 
to the driver every time the headphone jack connected or disconnected.

More specificaly the idea was to find a `snd_soc_jack` object associated with 
the sound card, attach a `notifier_block` to it and somehow reconfigure the 
driver on that notification.

I successfuly found `snd_card`, `snd_soc_card`, `snd_jack` and `hda_codec` 
objects but it seems like the sound driver uses polling by timer instead of 
notifications and I have no idea how to attach my module to it. So I'm
kind of stuck and would be glad to get some advices.

Meanwhile, I somewhat solved the problem in 
[user-space](https://github.com/AlxndrMkrv/matebook-16s-jack-daemon) by using
`poll()` on `/dev/input/` events

### Compile

``` bash
$ cmake <path_to>/matebook-16s-sound-fix
$ make module
```

### Run and view logs

``` bash
$ sudo insmod matebook-16s-sound-fix.ko
$ sudo dmesg | tail -n 100
```


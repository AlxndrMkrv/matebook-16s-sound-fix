# matebook-16s-sound-fix

This is my first attempt to write a linux kernel module.

The goal of this project is to implement the workaround for a well-known bug 
with sound cards SoC pins connection leading to the nessesity to send hda-verbs 
to the driver every time the headphone jack connected or disconnected.

More specificaly the idea was to find a `snd_soc_jack` object associated with 
the sound card, attach a `notifier_block` to it and somehow reconfigure the 
driver on that notification.

I successfuly found `snd_card`, `snd_soc_card`, `snd_jack`, `hda_codec` but 
that's all.

So I will be glad to hear any suggestions =)


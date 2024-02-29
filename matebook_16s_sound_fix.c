#include <linux/compiler_types.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <sound/core.h>
#include <sound/hdaudio.h>
#include <sound/hdaudio_ext.h>
#include <sound/hda_codec.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include <sound/soc-card.h>
#include <sound/soc-dapm.h>
#include <sound/soc-jack.h>

static const unsigned int VENDOR_ID = 0x8086;
static const unsigned int DEVICE_ID = 0x51CA;
static const char *CHIP_NAME = "SN6140";
static const char *MIC_JACK_NAME = "Mic";
static const char *HEADPHONE_JACK_NAME = "Headphone";
static const char *MIC_KCTL_NAME = "Mic Jack";
static const char *HEADPHONE_KCTL_NAME = "Headphone Jack";
//static struct notifier_block nb;

/*static irqreturn_t interrupt_handler(int irq, void *dev_id)
{
	printk(KERN_INFO "Interrupt occurred (IRQ %d)\n", irq);

	return IRQ_HANDLED;
}*/

static bool is_sound_card_valid(struct snd_card *card)
{
	if (!card)
		return false;

	struct device *dev = card->ctl_dev;
	struct pci_dev *pdev;

	while (true) {
		if (dev->bus != NULL && !strncmp(dev->bus->name, "pci", 3)) {
			pdev = container_of(dev, struct pci_dev, dev);

			if (pdev != NULL) {
				if (pdev->vendor == VENDOR_ID &&
				    pdev->device == DEVICE_ID)
					return true;
			}
		}

		if (dev->parent == NULL)
			break;

		dev = dev->parent;
	}

	return false;
}

static struct snd_card *get_sound_card(void)
{
	struct snd_card *card;
	for (int i = 0;; ++i) {
		card = snd_card_ref(i);

		if (is_sound_card_valid(card))
			return card;
	}
	return NULL;
}

static struct hda_codec *get_codec(struct snd_card *card)
{
	struct hda_codec *codec;
	struct snd_device *dev;

	list_for_each_entry(dev, &card->devices, list) {
		if (dev->type == SNDRV_DEV_CODEC) {
			codec = dev->device_data;
			if (codec && codec->card == dev->card &&
			    !strncmp(codec->core.chip_name, CHIP_NAME,
				     strlen(CHIP_NAME)))
				return codec;
		}
	}

	return NULL;
}

static struct snd_jack *get_jack(struct snd_card *card, const char *name)
{
	struct snd_jack *jack;
	struct snd_device *dev;

	list_for_each_entry(dev, &card->devices, list) {
		if (dev->type == SNDRV_DEV_JACK) {
			jack = dev->device_data;
			if (jack && !strncmp(jack->id, name, strlen(name)))
				return jack;
		}
	}

	return NULL;
}

// MIC_JACK_NAME or HEADPHONE_JACK_NAME should be passed
static struct snd_kcontrol *get_kcontrol(struct snd_soc_card *card,
					 const char *name)
{
	return snd_soc_card_get_kcontrol(card, name);
}

static struct snd_soc_card *get_soc_sound_card(struct snd_card *card)
{
	struct snd_soc_card *soc_card = dev_get_drvdata(card->dev);

	if (soc_card->snd_card != card) {
		return NULL;
	}

	return soc_card;
}

static void set_inverted_jack_detection(struct hda_codec *codec)
{
	if (!codec)
		return;

	codec->inv_jack_detect = 1;
}

static void print_card_components(struct snd_card *card)
{
	struct snd_device *dev = NULL;

	printk(KERN_INFO "snd_device on %s:", card->longname);

	list_for_each_entry(dev, &card->devices, list) {
		printk(KERN_INFO "    device on card: %d", dev->type);
	}
}

static void print_soc_card_components(struct snd_soc_card *card)
{
	struct snd_soc_component *comp = NULL;

	printk(KERN_INFO "soc_card_component on %s:", card->name);

	for_each_card_components(card, comp) {
		printk(KERN_INFO "    %s, driver: %s", comp->name,
		       comp->driver->name);
	}
}

static int __init init(void)
{
	printk(KERN_INFO "%s is looking for snd_soc_jack...",
	       module_name(THIS_MODULE));

	struct snd_card *card = get_sound_card();

	if (!card) {
		printk(KERN_ERR "%s: failed to find sound card",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct snd_soc_card *soc_card = get_soc_sound_card(card);

	if (!soc_card) {
		printk(KERN_ERR "%s: failed go get SoC from sound card",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct snd_jack *mic_jack = get_jack(card, MIC_JACK_NAME);

	if (!mic_jack) {
		printk(KERN_ERR "%s: failed to get MIC Jack from sound card",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct snd_kcontrol *mic_jack_kctl =
		get_kcontrol(soc_card, MIC_KCTL_NAME);

	if (!mic_jack_kctl) {
		printk(KERN_ERR "%s: failed to get MIC Jack kcontrol from SoC",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct snd_jack *headp_jack = get_jack(card, HEADPHONE_JACK_NAME);

	if (!headp_jack) {
		printk(KERN_ERR
		       "%s: failed to get Headphone Jack from sound card",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct snd_kcontrol *headp_jack_kctl =
		get_kcontrol(soc_card, HEADPHONE_KCTL_NAME);

	if (!headp_jack_kctl) {
		printk(KERN_ERR
		       "%s: failed to get Headphone Jack kcontrol from SoC",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	struct hda_codec *codec = get_codec(card);

	if (!codec) {
		printk(KERN_ERR "%s: failed to get Codec from sound card",
		       module_name(THIS_MODULE));
		return -ENODEV;
	}

	// set_inverted_jack_detection(codec);
	print_card_components(card);
	print_soc_card_components(soc_card);

	printk(KERN_INFO "%s: initialization complete",
	       module_name(THIS_MODULE));

	return 0;
}

static void __exit cleanup(void)
{
	printk(KERN_ALERT "%s: cleanup\n", module_name(THIS_MODULE));
}

module_init(init);
module_exit(cleanup);

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: snd_hda_codec_conexant");
MODULE_DESCRIPTION("SN6140 reconfigurator");
MODULE_AUTHOR("Alexander Makarov");

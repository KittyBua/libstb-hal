/*
 * determine the capabilities of the hardware.
 * part of libstb-hal
 *
 * (C) 2010-2012 Stefan Seyfried
 *
 * License: GPL v2 or later
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <hardware_caps.h>

static int initialized = 0;
static hw_caps_t caps;

hw_caps_t *get_hwcaps(void)
{
	if (initialized)
		return &caps;

	memset(&caps, 0, sizeof(hw_caps_t));

	caps.pip_devs = 0;
	if (access("/dev/dvb/adapter0/video1", F_OK) != -1)
		caps.pip_devs = 1;
	if (access("/dev/dvb/adapter0/video2", F_OK) != -1)
		caps.pip_devs = 2;
	if (access("/dev/dvb/adapter0/video3", F_OK) != -1)
		caps.pip_devs = 3;
	if (caps.pip_devs > 0)
		caps.can_pip = 1;

#if BOXMODEL_VUSOLO4K
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0; // needs test
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	caps.pip_devs = 2; // has only 3 real usable video devices
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vusolo4k");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "SOLO4K");
	strcpy(caps.boxarch, "BCM7376");
#endif
#if BOXMODEL_VUDUO4K
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0; // needs test
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	caps.has_HDMI_input = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuduo4k");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "DUO4K");
	strcpy(caps.boxarch, "BCM7278");
#endif
#if BOXMODEL_VUDUO4KSE
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0; // needs test
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	caps.has_HDMI_input = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuduo4kse");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "DUO4KSE");
	strcpy(caps.boxarch, "BCM7444S");
#endif
#if BOXMODEL_VUULTIMO4K
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 800;
	caps.display_yres = 480;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0; // needs test
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	caps.has_HDMI_input = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuultimo4k");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "ULTIMO4K");
	strcpy(caps.boxarch, "BCM7444S");
#endif
#if BOXMODEL_VUZERO4K
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_type = HW_DISPLAY_LED_ONLY;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuzero4k");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "ZERO4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_VUUNO4KSE
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 400;
	caps.display_yres = 240;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0; // needs test
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	caps.has_HDMI_input = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuuno4kse");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "UNO4KSE");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_VUUNO4K
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_type = HW_DISPLAY_LED_ONLY;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "vuuno4k");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "UNO4K");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_HD51
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 16;
	caps.display_type = HW_DISPLAY_LINE_TEXT;
	caps.display_can_umlauts = 1;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "hd51");
	strcpy(caps.boxvendor, "AX");
	strcpy(caps.boxname, "HD51");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_BRE2ZE4K
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "breeze4k");
	strcpy(caps.boxvendor, "WWIO");
	strcpy(caps.boxname, "BRE2ZE4K");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_H7
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "h7");
	strcpy(caps.boxvendor, "AirDigital");
	strcpy(caps.boxname, "Zgemma H7");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_E4HDULTRA
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 220;
	caps.display_yres = 176;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "e4hdultra");
	strcpy(caps.boxvendor, "AXAS");
	strcpy(caps.boxname, "E4HD 4K ULTRA");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_PROTEK4K
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 220;
	caps.display_yres = 176;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;    // 0 because we use graphlcd
	caps.display_can_set_brightness = 0; // 0 because we use graphlcd
	caps.display_has_statusline = 0;     // 0 because we use graphlcd
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "protek4k");
	strcpy(caps.boxvendor, "Protek");
	strcpy(caps.boxname, "Protek 4K UHD");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_HD60
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxmodel, "hd60");
	strcpy(caps.boxvendor, "AX");
	strcpy(caps.boxname, "HD60");
	strcpy(caps.boxarch, "HI3798MV200");
#endif
#if BOXMODEL_HD61
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxmodel, "hd61");
	strcpy(caps.boxvendor, "AX");
	strcpy(caps.boxname, "HD61");
	strcpy(caps.boxarch, "HI3798MV200");
#endif
#if BOXMODEL_MULTIBOX
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 0;
	caps.display_type = HW_DISPLAY_NONE;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 0;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxmodel, "multibox");
	strcpy(caps.boxvendor, "Maxytec");
	strcpy(caps.boxname, "Multibox 4K");
	strcpy(caps.boxarch, "HI3798MV200");
#endif
#if BOXMODEL_MULTIBOXSE
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 0;
	caps.display_type = HW_DISPLAY_NONE;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 0;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxmodel, "multiboxse");
	strcpy(caps.boxvendor, "Maxytec");
	strcpy(caps.boxname, "Multibox SE 4K");
	strcpy(caps.boxarch, "HI3798MV200");
#endif
#if BOXMODEL_OSMINI4K
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 1;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "osmini4k");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mini 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_OSMIO4K
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_umlauts = 0;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 1;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "osmio4k");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mio 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_OSMIO4KPLUS
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 128;
	caps.display_yres = 32;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_umlauts = 0;        // needs test
	caps.display_can_deepstandby = 0;    // evaluation is required with usage of graphlcd/lcd4linux, in this case = 0
	caps.display_can_set_brightness = 1; // evaluation is required with usage of graphlcd/lcd4linux, in this case = 0
	caps.display_has_statusline = 0;     // evaluation is required with usage of graphlcd/lcd4linux, in this case = 0
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxmodel, "osmio4kplus");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mio+ 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif

	initialized = 1;
	return &caps;
}

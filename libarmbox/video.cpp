/*
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 * (C) 2010-2013, 2015 Stefan Seyfried
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <ctype.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <pthread.h>

#include <linux/dvb/video.h>
#include <linux/fb.h>
#include "video_lib.h"
#include "lt_debug.h"

#include <proc_tools.h>

#define lt_debug(args...) _lt_debug(TRIPLE_DEBUG_VIDEO, this, args)
#define lt_info(args...) _lt_info(TRIPLE_DEBUG_VIDEO, this, args)
#define lt_debug_c(args...) _lt_debug(TRIPLE_DEBUG_VIDEO, NULL, args)
#define lt_info_c(args...) _lt_info(TRIPLE_DEBUG_VIDEO, NULL, args)

#define fop(cmd, args...) ({				\
	int _r;						\
	if (fd >= 0) { 					\
		if ((_r = ::cmd(fd, args)) < 0)		\
			lt_info(#cmd"(fd, "#args")\n");	\
		else					\
			lt_debug(#cmd"(fd, "#args")\n");\
	}						\
	else { _r = fd; } 				\
	_r;						\
})

cVideo * videoDecoder = NULL;
cVideo * pipDecoder = NULL;

int system_rev = 0;

static bool stillpicture = false;

static const char *VDEV[] = {
	"/dev/dvb/adapter0/video0",
	"/dev/dvb/adapter0/video1"
};
static const char *VMPEG_aspect[] = {
	"/proc/stb/vmpeg/0/aspect",
	"/proc/stb/vmpeg/1/aspect"
};

static const char *VMPEG_xres[] = {
	"/proc/stb/vmpeg/0/xres",
	"/proc/stb/vmpeg/1/xres"
};

static const char *VMPEG_yres[] = {
	"/proc/stb/vmpeg/0/yres",
	"/proc/stb/vmpeg/1/yres"
};

static const char *VMPEG_dst_all[] = {
	"/proc/stb/vmpeg/0/dst_all",
	"/proc/stb/vmpeg/1/dst_all"
};

static const char *VMPEG_dst_height[] = {
	"/proc/stb/vmpeg/0/dst_height",
	"/proc/stb/vmpeg/1/dst_height"
};

static const char *VMPEG_dst_width[] = {
	"/proc/stb/vmpeg/0/dst_width",
	"/proc/stb/vmpeg/1/dst_width"
};

static const char *VMPEG_dst_top[] = {
	"/proc/stb/vmpeg/0/dst_top",
	"/proc/stb/vmpeg/1/dst_top"
};

static const char *VMPEG_dst_left[] = {
	"/proc/stb/vmpeg/0/dst_left",
	"/proc/stb/vmpeg/1/dst_left"
};

static const char *VMPEG_framerate[] = {
	"/proc/stb/vmpeg/0/framerate",
	"/proc/stb/vmpeg/1/framerate"
};

static const char *vid_modes[] = {
	"pal",		// VIDEO_STD_NTSC
	"pal",		// VIDEO_STD_SECAM
	"pal",		// VIDEO_STD_PAL
	"480p",		// VIDEO_STD_480P
	"576p50",	// VIDEO_STD_576P
	"720p60",	// VIDEO_STD_720P60
	"1080i60",	// VIDEO_STD_1080I60
	"720p50",	// VIDEO_STD_720P50
	"1080i50",	// VIDEO_STD_1080I50
	"1080p30",	// VIDEO_STD_1080P30
	"1080p24",	// VIDEO_STD_1080P24
	"1080p25",	// VIDEO_STD_1080P25
	"1080p50",	// VIDEO_STD_1080P50
	"1080p60",	// VIDEO_STD_1080P60
	"1080p2397",	// VIDEO_STD_1080P2397
	"1080p2997",	// VIDEO_STD_1080P2997
	"2160p24",	// VIDEO_STD_2160P24
	"2160p25",	// VIDEO_STD_2160P25
	"2160p30",	// VIDEO_STD_2160P30
	"2160p50",	// VIDEO_STD_2160P50
	"720p50"	// VIDEO_STD_AUTO
};

#define VIDEO_STREAMTYPE_MPEG2 0
#define VIDEO_STREAMTYPE_MPEG4_H264 1
#define VIDEO_STREAMTYPE_VC1 3
#define VIDEO_STREAMTYPE_MPEG4_Part2 4
#define VIDEO_STREAMTYPE_VC1_SM 5
#define VIDEO_STREAMTYPE_MPEG1 6
#define VIDEO_STREAMTYPE_H265_HEVC 7
#define VIDEO_STREAMTYPE_AVS 16

cVideo::cVideo(int, void *, void *, unsigned int unit)
{
	lt_debug("%s unit %u\n", __func__, unit);

	brightness = -1;
	contrast = -1;
	saturation = -1;
	hue = -1;
	video_standby = 0;
	if (unit > 1) {
		lt_info("%s: unit %d out of range, setting to 0\n", __func__, unit);
		devnum = 0;
	} else
		devnum = unit;
	fd = -1;
	openDevice();
}

cVideo::~cVideo(void)
{
	closeDevice();
}

void cVideo::openDevice(void)
{
	int n = 0;
	lt_debug("#%d: %s\n", devnum, __func__);
	/* todo: this fd checking is racy, should be protected by a lock */
	if (fd != -1) /* already open */
		return;
retry:
	if ((fd = open(VDEV[devnum], O_RDWR|O_CLOEXEC|O_NONBLOCK)) < 0)
	{
		if (errno == EBUSY)
		{
			/* sometimes we get busy quickly after close() */
			usleep(50000);
			if (++n < 10)
				goto retry;
		}
		lt_info("#%d: %s cannot open %s: %m, retries %d\n", devnum, __func__, VDEV[devnum], n);
	}
	playstate = VIDEO_STOPPED;
}

void cVideo::closeDevice(void)
{
	lt_debug("%s\n", __func__);
	/* looks like sometimes close is unhappy about non-empty buffers */
	Start();
	if (fd >= 0)
		close(fd);
	fd = -1;
	playstate = VIDEO_STOPPED;
}

int cVideo::setAspectRatio(int aspect, int mode)
{
	static const char *a[] = { "n/a", "4:3", "14:9", "16:9" };
	static const char *m[] = { "panscan", "letterbox", "bestfit", "nonlinear", "(unset)" };
	int n;
	lt_debug("%s: a:%d m:%d  %s\n", __func__, aspect, mode, m[(mode < 0||mode > 3) ? 4 : mode]);

	if (aspect > 3 || aspect == 0)
		lt_info("%s: invalid aspect: %d\n", __func__, aspect);
	else if (aspect > 0) /* -1 == don't set */
	{
		lt_debug("%s: /proc/stb/video/aspect -> %s\n", __func__, a[aspect]);
		n = proc_put("/proc/stb/video/aspect", a[aspect], strlen(a[aspect]));
		if (n < 0)
			lt_info("%s: proc_put /proc/stb/video/aspect (%m)\n", __func__);
	}

	if (mode == -1)
		return 0;

	lt_debug("%s: /proc/stb/video/policy -> %s\n", __func__, m[mode]);
	n = proc_put("/proc/stb/video/policy", m[mode], strlen(m[mode]));
	if (n < 0)
		return 1;
	return 0;
}

int cVideo::getAspectRatio(void)
{
	video_size_t s;
	if (fd == -1)
	{
		/* in movieplayer mode, fd is not opened -> fall back to procfs */
		int n = proc_get_hex(VMPEG_aspect[devnum]);
		return n * 2 + 1;
	}
	if (fop(ioctl, VIDEO_GET_SIZE, &s) < 0)
	{
		lt_info("%s: VIDEO_GET_SIZE %m\n", __func__);
		return -1;
	}
	lt_debug("#%d: %s: %d\n", devnum, __func__, s.aspect_ratio);
	return s.aspect_ratio * 2 + 1;
}

int cVideo::setCroppingMode(int /*vidDispMode_t format*/)
{
	return 0;
#if 0
	croppingMode = format;
	const char *format_string[] = { "norm", "letterbox", "unknown", "mode_1_2", "mode_1_4", "mode_2x", "scale", "disexp" };
	const char *f;
	if (format >= VID_DISPMODE_NORM && format <= VID_DISPMODE_DISEXP)
		f = format_string[format];
	else
		f = "ILLEGAL format!";
	lt_debug("%s(%d) => %s\n", __FUNCTION__, format, f);
	return fop(ioctl, MPEG_VID_SET_DISPMODE, format);
#endif
}

int cVideo::Start(void * /*PcrChannel*/, unsigned short /*PcrPid*/, unsigned short /*VideoPid*/, void * /*hChannel*/)
{
	lt_debug("#%d: %s playstate=%d\n", devnum, __func__, playstate);
#if 0
	if (playstate == VIDEO_PLAYING)
		return 0;
	if (playstate == VIDEO_FREEZED)  /* in theory better, but not in practice :-) */
		fop(ioctl, MPEG_VID_CONTINUE);
#endif
	playstate = VIDEO_PLAYING;
	fop(ioctl, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX);
	int res = fop(ioctl, VIDEO_PLAY);
	if (brightness > -1) {
		SetControl(VIDEO_CONTROL_BRIGHTNESS, brightness);
		brightness = -1;
	}
	if (contrast > -1) {
		SetControl(VIDEO_CONTROL_CONTRAST, contrast);
		contrast = -1;
	}
	if (saturation > -1) {
		SetControl(VIDEO_CONTROL_SATURATION, saturation);
		saturation = -1;
	}
	if (hue > -1) {
		SetControl(VIDEO_CONTROL_HUE, hue);
		hue = -1;
	}
	return res;
}

int cVideo::Stop(bool blank)
{
	lt_debug("#%d: %s(%d)\n", devnum, __func__, blank);
	if (stillpicture)
	{
		lt_debug("%s: stillpicture == true\n", __func__);
		return -1;
	}
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
	return fop(ioctl, VIDEO_STOP, blank ? 1 : 0);
}

int cVideo::setBlank(int)
{
	fop(ioctl, VIDEO_PLAY);
	fop(ioctl, VIDEO_CONTINUE);
	video_still_picture sp = { NULL, 0 };
	fop(ioctl, VIDEO_STILLPICTURE, &sp);
	return Stop(1);
}

int cVideo::GetVideoSystem(void)
{
	char current[32];
	proc_get("/proc/stb/video/videomode", current, 32);
	for (int i = 0; vid_modes[i]; i++)
	{
		if (strcmp(current, vid_modes[i]) == 0)
			return i;
	}
	lt_info("%s: could not find '%s' mode, returning VIDEO_STD_720P50\n", __func__, current);
	return VIDEO_STD_720P50;
}

void cVideo::GetVideoSystemFormatName(cs_vs_format_t *format, int system)
{
	if (system == -1)
		system = GetVideoSystem();
	if (system < 0 || system > VIDEO_STD_1080P50) {
		lt_info("%s: invalid system %d\n", __func__, system);
		strcpy(format->format, "invalid");
	} else
		strcpy(format->format, vid_modes[system]);
}

int cVideo::SetVideoSystem(int video_system, bool remember)
{
	lt_debug("%s(%d, %d)\n", __func__, video_system, remember);
	char current[32];

	if (video_system > VIDEO_STD_MAX)
	{
		lt_info("%s: video_system (%d) > VIDEO_STD_MAX (%d)\n", __func__, video_system, VIDEO_STD_MAX);
		return -1;
	}
	int ret = proc_get("/proc/stb/video/videomode", current, 32);
	if (strcmp(current, vid_modes[video_system]) == 0)
	{
		lt_info("%s: video_system %d (%s) already set, skipping\n", __func__, video_system, current);
		return 0;
	}
	lt_info("%s: old: '%s' new: '%s'\n", __func__, current, vid_modes[video_system]);
	bool stopped = false;
	if (playstate == VIDEO_PLAYING)
	{
		lt_info("%s: playstate == VIDEO_PLAYING, stopping video\n", __func__);
		Stop();
		stopped = true;
	}
	ret = proc_put("/proc/stb/video/videomode", vid_modes[video_system],strlen(vid_modes[video_system]));
	if (stopped)
		Start();

	return ret;
}

int cVideo::getPlayState(void)
{
	return playstate;
}

void cVideo::SetVideoMode(analog_mode_t mode)
{
	lt_debug("#%d: %s(%d)\n", devnum, __func__, mode);
	if (!(mode & ANALOG_SCART_MASK))
	{
		lt_debug("%s: non-SCART mode ignored\n", __func__);
		return;
	}
	const char *m;
	switch(mode)
	{
		case ANALOG_SD_YPRPB_SCART:
			m = "yuv";
			break;
		case ANALOG_SD_RGB_SCART:
			m = "rgb";
			break;
		default:
			lt_info("%s unknown mode %d\n", __func__, mode);
			m = "rgb";
			break; /* default to rgb */
	}
	proc_put("/proc/stb/avs/0/colorformat", m, strlen(m));
}

void cVideo::ShowPicture(const char * fname, const char *_destname)
{
	lt_debug("%s(%s)\n", __func__, fname);
	static const unsigned char pes_header[] = { 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x80, 0x00, 0x00 };
	static const unsigned char seq_end[] = { 0x00, 0x00, 0x01, 0xB7 };
	char destname[512];
	char cmd[512];
	char *p;
	int mfd;
	struct stat st, st2;
	if (video_standby)
	{
		/* does not work and the driver does not seem to like it */
		lt_info("%s: video_standby == true\n", __func__);
		return;
	}
	const char *lastDot = strrchr(fname, '.');
	if (lastDot && !strcasecmp(lastDot + 1, "m2v"))
		strncpy(destname, fname, sizeof(destname));
	else {
		if (_destname)
			strncpy(destname, _destname, sizeof(destname));
		else {
			strcpy(destname, "/tmp/cache");
			if (stat(fname, &st2))
			{
				lt_info("%s: could not stat %s (%m)\n", __func__, fname);
				return;
			}
			mkdir(destname, 0755);
			/* the cache filename is (example for /share/tuxbox/neutrino/icons/radiomode.jpg):
			   /var/cache/share.tuxbox.neutrino.icons.radiomode.jpg.m2v
			   build that filename first...
			   TODO: this could cause name clashes, use a hashing function instead... */
			strcat(destname, fname);
			p = &destname[strlen("/tmp/cache/")];
			while ((p = strchr(p, '/')) != NULL)
				*p = '.';
			strcat(destname, ".m2v");
		}
		/* ...then check if it exists already... */
		if (stat(destname, &st) || (st.st_mtime != st2.st_mtime) || (st.st_size == 0))
		{
			struct utimbuf u;
			u.actime = time(NULL);
			u.modtime = st2.st_mtime;
			/* it does not exist or has a different date, so call ffmpeg... */
			sprintf(cmd, "ffmpeg -y -f mjpeg -i '%s' -s 1280x720 -aspect 16:9 '%s' </dev/null",
								fname, destname);
			system(cmd); /* TODO: use libavcodec to directly convert it */
			utime(destname, &u);
		}
	}
	mfd = open(destname, O_RDONLY);
	if (mfd < 0)
	{
		lt_info("%s cannot open %s: %m\n", __func__, destname);
		goto out;
	}
	fstat(mfd, &st);

	closeDevice();
	openDevice();

	if (fd >= 0)
	{
		stillpicture = true;

		if (ioctl(fd, VIDEO_SET_FORMAT, VIDEO_FORMAT_16_9) < 0)
			lt_info("%s: VIDEO_SET_FORMAT failed (%m)\n", __func__);
		bool seq_end_avail = false;
		off_t pos=0;
		unsigned char *iframe = (unsigned char *)malloc((st.st_size < 8192) ? 8192 : st.st_size);
		if (! iframe)
		{
			lt_info("%s: malloc failed (%m)\n", __func__);
			goto out;
		}
		read(mfd, iframe, st.st_size);
		ioctl(fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY);
		ioctl(fd, VIDEO_PLAY);
		ioctl(fd, VIDEO_CONTINUE);
		ioctl(fd, VIDEO_CLEAR_BUFFER);
		while (pos <= (st.st_size-4) && !(seq_end_avail = (!iframe[pos] && !iframe[pos+1] && iframe[pos+2] == 1 && iframe[pos+3] == 0xB7)))
			++pos;

		if ((iframe[3] >> 4) != 0xE) // no pes header
			write(fd, pes_header, sizeof(pes_header));
		write(fd, iframe, st.st_size);
		if (!seq_end_avail)
			write(fd, seq_end, sizeof(seq_end));
		memset(iframe, 0, 8192);
		write(fd, iframe, 8192);
		ioctl(fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX);
		free(iframe);
	}
 out:
	close(mfd);
	return;
}

void cVideo::StopPicture()
{
	lt_debug("%s\n", __func__);
	stillpicture = false;
	Stop(1);
}

void cVideo::Standby(unsigned int bOn)
{
	lt_debug("%s(%d)\n", __func__, bOn);
	if (bOn)
	{
		closeDevice();
	}
	else
	{
		openDevice();
	}
	video_standby = bOn;
}

int cVideo::getBlank(void)
{
	static unsigned int lastcount = 0;
	unsigned int count = 0;
	size_t n = 0;
	ssize_t r;
	char *line = NULL;
	/* hack: the "mailbox" irq is not increasing if
	 * no audio or video is decoded... */
	FILE *f = fopen("/proc/interrupts", "r");
	if (! f) /* huh? */
		return 0;
	while ((r = getline(&line, &n, f)) != -1)
	{
		if (r <= (ssize_t) strlen("mailbox")) /* should not happen... */
			continue;
		line[r - 1] = 0; /* remove \n */
		if (!strcmp(&line[r - 1 - strlen("mailbox")], "mailbox"))
		{
			count =  atoi(line + 5);
			break;
		}
	}
	free(line);
	fclose(f);
	int ret = (count == lastcount); /* no new decode -> return 1 */
	lt_debug("#%d: %s: %d (irq++: %d)\n", devnum, __func__, ret, count - lastcount);
	lastcount = count;
	return ret;
}

void cVideo::Pig(int x, int y, int w, int h, int osd_w, int osd_h, int startx, int starty, int endx, int endy)
{
	char buffer[64];
	int _x, _y, _w, _h;
	/* the target "coordinates" seem to be in a PAL sized plane
	 * TODO: check this in the driver sources */
	int xres = 720; /* proc_get_hex("/proc/stb/vmpeg/0/xres") */
	int yres = 576; /* proc_get_hex("/proc/stb/vmpeg/0/yres") */
	lt_debug("#%d %s: x:%d y:%d w:%d h:%d ow:%d oh:%d\n", devnum, __func__, x, y, w, h, osd_w, osd_h);
	if (x == -1 && y == -1 && w == -1 && h == -1)
	{
		_w = xres;
		_h = yres;
		_x = 0;
		_y = 0;
	}
	else
	{
		// need to do some additional adjustments because osd border is handled by blitter
		x += startx;
		x *= endx - startx + 1;
		y += starty;
		y *= endy - starty + 1;
		w *= endx - startx + 1;
		h *= endy - starty + 1;
		_x = x * xres / osd_w;
		_w = w * xres / osd_w;
		_y = y * yres / osd_h;
		_h = h * yres / osd_h;
		_x /= 1280;
		_y /= 720;
		_w /= 1280;
		_h /= 720;
	}
	lt_debug("#%d %s: x:%d y:%d w:%d h:%d xr:%d yr:%d\n", devnum, __func__, _x, _y, _w, _h, xres, yres);
	sprintf(buffer, "%x", _x);
	proc_put(VMPEG_dst_left[devnum], buffer, strlen(buffer));

	sprintf(buffer, "%x", _y);
	proc_put(VMPEG_dst_top[devnum], buffer, strlen(buffer));

	sprintf(buffer, "%x", _w);
	proc_put(VMPEG_dst_width[devnum], buffer, strlen(buffer));

	sprintf(buffer, "%x", _h);
	proc_put(VMPEG_dst_height[devnum], buffer, strlen(buffer));
}

static inline int rate2csapi(int rate)
{
	switch (rate)
	{
		case 23976:
			return 0;
		case 24000:
			return 1;
		case 25000:
			return 2;
		case 29976:
			return 3;
		case 30000:
			return 4;
		case 50000:
			return 5;
		case 50940:
			return 6;
		case 60000:
			return 7;
		default:
			break;
	}
	return -1;
}

void cVideo::getPictureInfo(int &width, int &height, int &rate)
{
	video_size_t s;
	int r;
	if (fd == -1)
	{
		/* in movieplayer mode, fd is not opened -> fall back to procfs */
		r      = proc_get_hex(VMPEG_framerate[devnum]);
		width  = proc_get_hex(VMPEG_xres[devnum]);
		height = proc_get_hex(VMPEG_yres[devnum]);
		rate   = rate2csapi(r);
		return;
	}
	ioctl(fd, VIDEO_GET_SIZE, &s);
	ioctl(fd, VIDEO_GET_FRAME_RATE, &r);
	rate = rate2csapi(r);
	height = s.h;
	width = s.w;
	lt_debug("#%d: %s: rate: %d, width: %d height: %d\n", devnum, __func__, rate, width, height);
}

void cVideo::SetSyncMode(AVSYNC_TYPE mode)
{
	lt_debug("%s %d\n", __func__, mode);
	/*
	 * { 0, LOCALE_OPTIONS_OFF },
	 * { 1, LOCALE_OPTIONS_ON  },
	 * { 2, LOCALE_AUDIOMENU_AVSYNC_AM }
	 */
};

int cVideo::SetStreamType(VIDEO_FORMAT type)
{
	static const char *VF[] = {
		"VIDEO_FORMAT_MPEG2",
		"VIDEO_FORMAT_MPEG4",
		"VIDEO_FORMAT_VC1",
		"VIDEO_FORMAT_JPEG",
		"VIDEO_FORMAT_GIF",
		"VIDEO_FORMAT_PNG"
	};
	int t;
	lt_debug("#%d: %s type=%s\n", devnum, __func__, VF[type]);

	switch (type)
	{
		case VIDEO_FORMAT_MPEG4_H264:
			t = VIDEO_STREAMTYPE_MPEG4_H264;
			break;
		case VIDEO_FORMAT_MPEG4_H265:
			t = VIDEO_STREAMTYPE_H265_HEVC;
			break;
		case VIDEO_FORMAT_AVS:
			t = VIDEO_STREAMTYPE_AVS;
			break;
		case VIDEO_FORMAT_VC1:
			t = VIDEO_STREAMTYPE_VC1;
			break;
		case VIDEO_FORMAT_MPEG2:
		default:
			t = VIDEO_STREAMTYPE_MPEG2;
			break;
	}

	if (ioctl(fd, VIDEO_SET_STREAMTYPE, t) < 0)
		lt_info("%s VIDEO_SET_STREAMTYPE(%d) failed: %m\n", __func__, t);
	return 0;
}

int64_t cVideo::GetPTS(void)
{
	int64_t pts = 0;
	if (ioctl(fd, VIDEO_GET_PTS, &pts) < 0)
		lt_info("%s: GET_PTS failed (%m)\n", __func__);
	return pts;
}

void cVideo::SetDemux(cDemux *)
{
	lt_debug("#%d %s not implemented yet\n", devnum, __func__);
}

void cVideo::SetControl(int control, int value) {
	const char *p = NULL;
	switch (control) {
	case VIDEO_CONTROL_BRIGHTNESS:
		brightness = value;
		p = "/proc/stb/vmpeg/0/pep_brightness";
		break;
	case VIDEO_CONTROL_CONTRAST:
		contrast = value;
		p = "/proc/stb/vmpeg/0/pep_contrast";
		break;
	case VIDEO_CONTROL_SATURATION:
		saturation = value;
		p = "/proc/stb/vmpeg/0/pep_saturation";
		break;
	case VIDEO_CONTROL_HUE:
		hue = value;
		p = "/proc/stb/vmpeg/0/pep_hue";
		break;
	}
	if (p) {
		char buf[20];
		int len = snprintf(buf, sizeof(buf), "%d", value);
		if (len < (int) sizeof(buf))
			proc_put(p, buf, len);
	}
}

void cVideo::SetColorFormat(COLOR_FORMAT color_format) {
	const char *p = NULL;
	switch(color_format) {
	case COLORFORMAT_RGB:
		p = "rgb";
		break;
	case COLORFORMAT_YUV:
		p = "yuv";
		break;
	case COLORFORMAT_CVBS:
		p = "cvbs";
		break;
	case COLORFORMAT_SVIDEO:
		p = "svideo";
		break;
	case COLORFORMAT_HDMI_RGB:
		p = "hdmi_rgb";
		break;
	case COLORFORMAT_HDMI_YCBCR444:
		p = "hdmi_yuv";
		break;
	case COLORFORMAT_HDMI_YCBCR422:
		p = "hdmi_422";
		break;
	}
	if (p)
		proc_put("/proc/stb/video/hdmi_colorspace", p, strlen(p));
}

/* TODO: aspect ratio correction and PIP */
bool cVideo::GetScreenImage(unsigned char * &video, int &xres, int &yres, bool get_video, bool get_osd, bool scale_to_video)
{
	lt_info("%s: video 0x%p xres %d yres %d vid %d osd %d scale %d\n",
		__func__, video, xres, yres, get_video, get_osd, scale_to_video);

	return true;
}
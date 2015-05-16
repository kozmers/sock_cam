#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/mman.h> 
#include <fcntl.h>
#include <sys/ioctl.h> 
#include <errno.h>
#include <assert.h>
#include <linux/videodev2.h> 


#define CLR(x) memset (&(x), 0, sizeof (x)) 
#define IF_RET(x) if( -1 == (x)) return -1


struct buffer { 
	void *                  start; 
	size_t                  length; 
}; 


static int xioctl(int  fd, int  request, void *arg) 
{ 
	int r; 

	do{
		r = ioctl (fd, request, arg); 
	}while (-1 == r && EINTR == errno);

	return r; 
} 


static int open_device(char * dev_name) 
{
	int fd;
	struct stat st;  
	if (-1 == stat (dev_name, &st)) { 
		fprintf (stderr, "Cannot identify '%s': %d, %s\n", 
				dev_name, errno, strerror (errno)); 
		return -1;
	} 
	if (!S_ISCHR (st.st_mode)) { 
		fprintf (stderr, "%s is no device\n", dev_name); 
		return -1;

	} 
	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0); 
	if (-1 == fd) { 
		fprintf (stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno)); 
		return -1;
	} 
	return fd;
} 


int init_device(int fd, char *dev_name) 
{ 
	struct v4l2_capability cap; 
	struct v4l2_cropcap cropcap; 
	struct v4l2_crop crop; 
	struct v4l2_format fmt; 
	unsigned int min; 

	IF_RET(fd = open_device(dev_name));

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) { 
			fprintf (stderr, "%s is no V4L2 device\n", 
					dev_name); 
		} else { 
			perror("QUERYCAP");
		} 
		return -1;
	} 
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) { 
		fprintf (stderr, "%s is no video capture device\n", 
				dev_name); 
		return -1; 
	} 
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) { 
		fprintf (stderr, "%s does not support streaming i/o\n", 
				dev_name); 
		return -1; 
	} 

	/* Select video input, video standard and tune here. */ 
	CLR (cropcap); 
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) { 
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
		crop.c = cropcap.defrect; /* reset to default */ 
		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) { 
			switch (errno) { 
				case EINVAL: 
					/* Cropping not supported. */ 
					break; 
				default: 
					/* Errors ignored. */ 
					break; 
			} 
		} 
	} else {         
		/* Errors ignored. */ 
	} 
	CLR (fmt);


	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	fmt.fmt.pix.width       = 640;  
	fmt.fmt.pix.height      = 480; 
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; 
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED; 


	IF_RET(xioctl (fd, VIDIOC_S_FMT, &fmt));
	/* Note VIDIOC_S_FMT may change width and height. */ 
	/* Buggy driver paranoia. */ 
	min = fmt.fmt.pix.width * 2; 
	if (fmt.fmt.pix.bytesperline < min) 
		fmt.fmt.pix.bytesperline = min; 
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height; 
	if (fmt.fmt.pix.sizeimage < min) 
		fmt.fmt.pix.sizeimage = min; 

	return 0;
}

int init_mmap(int fd, struct buffer *buffers) 
{ 
	struct v4l2_requestbuffers req; 
	int n_buffers;

	CLR (req); 

	req.count               = 4; 
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	req.memory              = V4L2_MEMORY_MMAP; 

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) { 
		if (EINVAL == errno) { 
			fprintf (stderr, "does not support " 
					"memory mapping\n"); 
		} else { 
			perror("REQBUFS");
		} 
		return -1;
	} 

	if (req.count < 2) { 
		fprintf (stderr, "Insufficient buffer memory \n"); 
		return -1;
	} 

	buffers = calloc (req.count, sizeof (*buffers)); 

	if (!buffers) { 
		fprintf (stderr, "Out of memory\n"); 
		return -1;
	} 

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) { 
		struct v4l2_buffer buf; 

		CLR (buf); 

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
		buf.memory      = V4L2_MEMORY_MMAP; 
		buf.index       = n_buffers; 

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf)) 
		{
			perror("QUREYBUF");
			return -1;
		}

		buffers[n_buffers].length = buf.length; 
		buffers[n_buffers].start = 
			mmap (NULL /* start anywhere */,

					buf.length, 
					PROT_READ | PROT_WRITE /* required */, 
					MAP_SHARED /* recommended */, 
					fd, buf.m.offset); 

		if (MAP_FAILED == buffers[n_buffers].start) 
			return -1;
	} 
	return 0;
} 


void clear_buff(struct buffer *buffers, unsigned int n_buffers) 
{
	unsigned int i; 
	for (i = 0; i < n_buffers; ++i) 
		if (-1 == munmap (buffers[i].start, buffers[i].length)) 
			printf("error munmap\n");
	return;
}


int start_capturing(int fd, int n_buffers) 
{
	enum v4l2_buf_type type; 
	int i;
	for (i = 0; i < n_buffers; ++i) { 
		struct v4l2_buffer buf; 

		CLR (buf); 

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
		buf.memory      = V4L2_MEMORY_MMAP; 
		buf.index       = i; 

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
		{
			perror("QBUF");
			return -1;
		}
	} 

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 

	if (-1 == xioctl (fd, VIDIOC_STREAMON, &type)) 
	{
		perror("STREAM_ON");
		return -1;
	}
	return 0;
}

int stop_capturing(int fd) 
{ 
	enum v4l2_buf_type type; 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type)) 
	{
		perror("STREAM_OFF");
		return -1;
	}
	return 0;
}


void process_image (const void *p,int len) 
{
}

static int read_frame(int fd, struct buffer *buffers, int n_buffers) 
{ 
	struct v4l2_buffer buf; 
	unsigned int i; 
	CLR (buf); 

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	buf.memory = V4L2_MEMORY_MMAP; 

	if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) { 
		switch (errno) { 
			case EAGAIN: 
				return 0; 

			case EIO: 
				/* Could ignore EIO, see spec. */ 

				/* fall through */ 

			default: 
				perror("DQBUF");
				return -1;
		} 
	} 

	assert (buf.index < n_buffers); 

	process_image (buffers[buf.index].start,buffers[buf.index].length); 

	if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
	{
		perror("QBUF");
		return -1;
	}
	return 0;

}

int mainloop (int fd){
	unsigned int count; 
	count = 100; 
	while (count-- > 0) { 
		for (;;) { 
			fd_set fds; 
			struct timeval tv; 
			int r; 
			FD_ZERO (&fds); 
			FD_SET (fd, &fds); 
			/* Timeout. */ 
			tv.tv_sec = 2; 
			tv.tv_usec = 0; 
			r = select (fd + 1, &fds, NULL, NULL, &tv);



			if (-1 == r) { 
				if (EINTR == errno) 
					continue; 
				perror("select");
				return -1; 
			} 

			if (0 == r) { 
				fprintf (stderr, "select timeout\n"); 
				return -1;
			} 

			//if (read_frame ()) 
			//                                break; 

			/* EAGAIN - continue select loop. */ 
		} 
	} 
	return 0;
} 

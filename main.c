#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/fb.h>

void uyvy_to_rgb(unsigned char *yuyvdata, unsigned char *rgbdata, int w, int h)
{
	//码流U0 Y0 V1 Y1 U2 Y2 V3 Y3 --》YUYV像素[Y0 U0 V1] [Y1 U0 V1] [Y2 U2 V3] [Y3 U2 V3]--》RGB像素
	int r1, g1, b1; 
	int r2, g2, b2;
	int Ymax=0;
	char data[4];
	/*for(int i=0; i<w*h/2; i++)
	{
		memcpy(data, yuyvdata+i*4, 4);
		if(data[1]>Ymax)Ymax=data[1];
		if(data[1]>Ymax)Ymax=data[3];
	}
	int Yg=255/(Ymax*0.7);*/
	int Yg=1;
	for(int i=0; i<w*h/2; i++)
	{
	    memcpy(data, yuyvdata+i*4, 4);
	    unsigned char U0=data[0];
	    unsigned char Y0=data[1]*Yg;Y0=(Y0<255)?Y0:255;
	    unsigned char V1=data[2];
	    unsigned char Y1=data[3]*Yg;Y1=(Y1<255)?Y1:255;
		//Y0U0Y1V1  -->[Y0 U0 V1] [Y1 U0 V1]
	   /* r1 = Y0+1.4075*(V1-128); if(r1>255)r1=255; if(r1<0)r1=0;
	    g1 =Y0- 0.3455 * (U0-128) - 0.7169*(V1-128); if(g1>255)g1=255; if(g1<0)g1=0;
	    b1 = Y0 + 1.779 * (U0-128);  if(b1>255)b1=255; if(b1<0)b1=0;
	 
	    r2 = Y1+1.4075*(V1-128);if(r2>255)r2=255; if(r2<0)r2=0;
	    g2 = Y1- 0.3455 * (U0-128) - 0.7169*(V1-128); if(g2>255)g2=255; if(g2<0)g2=0;
	    b2 = Y1 + 1.779 * (U0-128);  if(b2>255)b2=255; if(b2<0)b2=0;*/

		r1 = Y0+1.402*(U0-128); if(r1>255)r1=255; if(r1<0)r1=0;
	    g1 =Y0- 0.34414 * (V1-128) - 0.71414*(U0-128); if(g1>255)g1=255; if(g1<0)g1=0;
	    b1 = Y0 + 1.772 * (V1-128);  if(b1>255)b1=255; if(b1<0)b1=0;
	 
	    r2 = Y1+1.402*(U0-128);if(r2>255)r2=255; if(r2<0)r2=0;
	    g2 = Y1- 0.34414 * (V1-128) - 0.71414*(U0-128); if(g2>255)g2=255; if(g2<0)g2=0;
	    b2 = Y1 + 1.772 * (V1-128);  if(b2>255)b2=255; if(b2<0)b2=0;

		
		
	    rgbdata[i*6+0]=r1;
	    rgbdata[i*6+1]=g1;
	    rgbdata[i*6+2]=b1;
	    rgbdata[i*6+3]=r2;
	    rgbdata[i*6+4]=g2;
	    rgbdata[i*6+5]=b2;
	}
}

void readformat(int fd)
{
	struct v4l2_fmtdesc v4fmt;
	v4fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int i=0;
	while(1)
	{
		v4fmt.index = i++;
		int ret = ioctl(fd,VIDIOC_ENUM_FMT,&v4fmt);
		if(ret < 0)
		{
			perror("获取失败");
			break;
		}
		printf("index=%d\n", v4fmt.index);
		printf("flags=%d\n", v4fmt.flags);
		printf("description=%s\n", v4fmt.description);
		unsigned char *p = (unsigned char *)&v4fmt.pixelformat;
		printf("pixelformat=%c%c%c%c\n", p[0],p[1],p[2],p[3]);
		printf("reserved=%d\n", v4fmt.reserved[0]);
	}
}

void setformat(int fd,int mode,int width,int height)
{
	struct v4l2_format vfmt;
	if(mode == 0)
	{
		vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//摄像头采集
		vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;//设置视频采集格式
		vfmt.fmt.pix.width = width;//设置宽（不能任意）
		vfmt.fmt.pix.height = height;//设置高
	}
	else{
		vfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//设置视频输出格式
		vfmt.fmt.pix.width = width;//设置宽（不能任意）
		vfmt.fmt.pix.height = height;//设置高
	}
	
	int ret = ioctl(fd, VIDIOC_S_FMT, &vfmt);
	if(ret < 0)
	{
		perror("设置格式失败");
	}

	memset(&vfmt, 0, sizeof(vfmt));
	if(mode == 0){
		vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}
	else{
		vfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	}
	
	ret  = ioctl(fd, VIDIOC_G_FMT, &vfmt);
	if(ret < 0)
	{
		perror("获取格式失败");
	}
	if(mode == 0){
	if(vfmt.fmt.pix.width == width && vfmt.fmt.pix.height == height)
	{
		printf("vdev-->设置成功\n");
	}else
	{
		printf("vdev-->设置失败\n");
	}}
	if(mode == 1){
	if(vfmt.fmt.pix.width == 640 && vfmt.fmt.pix.height == 360)
	{
		printf("udev-->设置成功\n");
		
	}else
	{
		printf("udev-->设置失败\n");
	}}
}

void setframe(int fd,int Denominator,int Numerator)
{
	struct v4l2_streamparm Stream_Parm;
	struct v4l2_streamparm
 {
 enum v4l2_buf_type type;
 union
 {
 struct v4l2_captureparm capture;
 struct v4l2_outputparm output;
 __u8 raw_data[200];
 } parm;
};
	memset(&Stream_Parm,0,sizeof(Stream_Parm));
	Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	Stream_Parm.parm.capture.timeperframe.denominator=Denominator;
	Stream_Parm.parm.capture.timeperframe.numerator=Numerator;
	int ret = ioctl(fd,VIDIOC_S_PARM,&Stream_Parm);
	if(ret<0)
		perror("set frame error!");
}

void reqbuffer(int fd,int mode,int nbuff)
{
	struct v4l2_requestbuffers reqbuffer;
	memset(&reqbuffer,0,sizeof(reqbuffer));
	if(mode == 0){
		reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		reqbuffer.count = nbuff; //申请nbuff个缓冲区
		reqbuffer.memory = V4L2_MEMORY_MMAP ;//映射方式
	}
	else{
		reqbuffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		reqbuffer.count = nbuff; //申请nbuff个缓冲区
		reqbuffer.memory = V4L2_MEMORY_USERPTR ;//映射方式
	}
	
	int ret  = ioctl(fd, VIDIOC_REQBUFS, &reqbuffer);
	if(ret < 0)
	{
		perror("申请队列空间失败");
		if(mode == 0){
			printf("vdev-->申请队列空间失败\n");
		}
		else{
			printf("udev-->申请队列空间失败\n");
		}
	}else{
		if(mode == 0){
			printf("vdev-->申请队列空间成功\n");
		}
		else{
			printf("udev-->申请队列空间成功\n");
		}
	}
}

int lcdfd = 0;
unsigned int *lcdptr = NULL;
int lcd_w=800,lcd_h=480;
/*显示函数*/
void lcd_show_rgb(unsigned char *rgbdata,int w,int h)
{
	unsigned int *ptr = lcdptr;
	for(int i=0; i<h; i++)
	{
		for(int j=0; j<w; j++)
		{
			memcpy(ptr+j, rgbdata+j*3, 3);
		}
		ptr+=lcd_w;
		rgbdata+=w*3;
	}
}

int main(void)
{
	int ret;
	int nbuff = 4;
	int width=640,height=480;
	lcdfd = open("/dev/fb0",O_RDWR);
	/*获取LCD信息*/
	struct fb_var_screeninfo info;
	int lret = ioctl(lcdfd,FBIOGET_VSCREENINFO,&info);

	//虚拟机
	//lcd_w=info.xres_virtual;
	//lcd_h=info.yres_virtual;
	//开发板
	lcd_w=info.xres;
	lcd_h=info.yres;
	
	lcdptr = mmap(NULL,lcd_w*lcd_h*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	/*打开设备*/
	int vdev = open("/dev/video1", O_RDWR);
	if(vdev < 0)
	{
		perror("打开设备失败");
		return -1;
	}
	/*获取摄像头支持格式*/
	//readformat(fd);
	
	/*设置摄像头格式,0为capture，1位output*/
	setformat(vdev,0,width,height);
	
	/*申请内核空间*/
	reqbuffer(vdev,0,nbuff);
	/*映射*/
	unsigned char *mptr[nbuff];//保存映射后用户空间的首地址
    unsigned int  size[nbuff];
	struct v4l2_buffer mapbuffer;
	//初始化type, index
	mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	for(int i=0; i<nbuff; i++)
	{
		mapbuffer.index = i;
		ret = ioctl(vdev, VIDIOC_QUERYBUF, &mapbuffer);//从内核空间中查询一个空间做映射
		if(ret < 0)
		{
			perror("查询内核空间队列失败");
		}
		mptr[i] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ|PROT_WRITE, 
                                            MAP_SHARED, vdev, mapbuffer.m.offset);
            size[i]=mapbuffer.length;

		//通知使用完毕--‘放回去’
		ret  = ioctl(vdev, VIDIOC_QBUF, &mapbuffer);
		if(ret < 0)
		{
			perror("放回失败");
		}
	}
	printf("映射成功\n");
	setframe(vdev, 15, 1);

	/*开始采集*/
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vdev,VIDIOC_STREAMON,&type);
	if(ret < 0)
	{
		perror("开启失败");
	}
	printf("开启成功\n");
	//定义一个空间储存解码后的RGB数据
	unsigned char rgbdata[width*height*3];
	while(1)
	{
		/*从队列中提取一帧数据*/
		struct v4l2_buffer  readbuffer;
		readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(vdev, VIDIOC_DQBUF, &readbuffer);
		if(ret < 0)
		{
			perror("提取数据失败");
		}

		//显示到LCD上
		uyvy_to_rgb(mptr[readbuffer.index],rgbdata,width,height);//把yuyv数据解码为RGB数据
		lcd_show_rgb(rgbdata,width,height);
		printf("working!\n");


		//FILE *file=fopen("my.jpg", "w+");
		//fwrite(mptr[readbuffer.index], readbuffer.length, 1, file);
		//fclose(file);
		
		//通知内核已经使用完毕
		ret = ioctl(vdev, VIDIOC_QBUF, &readbuffer);
		if(ret < 0)
		{
			perror("放回队列失败");
		}
	}

	/*停止采集*/
	ret = ioctl(vdev,VIDIOC_STREAMOFF, &type);

	/*释放映射*/
	for(int i=0; i<nbuff; i++)
		munmap(mptr[i], size[i]);
	
	/*关闭设备*/
	close(vdev);
	return 0;
}


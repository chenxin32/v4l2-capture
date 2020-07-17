CC = arm-linux-gnueabihf-gcc
OBJS = main.c
v4l2_capture:$(OBJS)
	$(CC)  $< -o  $@ 

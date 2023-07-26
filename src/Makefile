# This makefile uses make recursively to allow for two separate
# builds: debug and opt (the latter being the default).

# If you don't want to get GLUI, GLE, or PLIB, or don't have glext.h,
# modify the EXTRA_FLAGS line before building.
#
# built/tested with:
# gcc 3.2 (Under Mandrake linux 9.0 3.2-1mdk)
# fmod 40424
# plib 1.8.4
# glui 2.01 (tweaked to fix compilation errors/warnings)
# flext.h containing GL_GLEXT_VERSION 7 (linux)
# gle 3.0.7
# glut 3.7
CC = g++
OPT_FLAGS = -O3 -ffast-math -finline-functions
# -Wall -pedantic
DEBUG_FLAGS = -g -Wall -pedantic
EXTRA_FLAGS = -L${HOME}/external_linux/include -I/usr/X11R6/include -DWITH_GL_EXT -DWITH_GLUI -DWITH_GLE -DWITH_PLIB -DUSE_FUNCTION -DWITH_FMOD
# use -lfmodex64 for 64bit platform
LIBFMOD     = -lfmodex
# Add  -lplibsm -lplibsl  to LDFLAGS if you're not using FMOD, but you are using PLIB
LDFLAGS = -L${HOME}/external_linux/lib -L/usr/X11R6/lib64 -L/usr/X11R6/lib -lpthread -lplibjs -lplibfnt -lplibsg -lplibul -lglui -lglut -lgle -lGLU -lGL -lX11 -lXmu $(LIBFMOD) -lpng

SRC = \
3ds.cpp \
aerofoil.cpp \
audio.cpp \
bludger.cpp \
body.cpp \
config.cpp \
config_file.cpp \
control_method.cpp \
environment.cpp \
explosion.cpp \
fft2d.cpp \
fuselage.cpp \
glider.cpp \
glider_aero.cpp \
glider_aero_component.cpp \
glider_aero_crrcsim.cpp \
glider_engine.cpp \
glider_graphics.cpp \
glider_graphics_3ds.cpp \
glider_graphics_component.cpp \
graphics_3ds.cpp \
gyro.cpp \
particle_engine.cpp \
particle_source.cpp \
propeller.cpp \
ski.cpp \
tree.cpp \
tree_collection.cpp \
glider_graphics_crrcsim.cpp \
glider_power.cpp \
glider_structure.cpp \
glider_structure_3ds.cpp \
glider_structure_component.cpp \
glider_structure_crrcsim.cpp \
gui.cpp \
heli_controller.cpp \
image_from_file.cpp \
joystick.cpp \
lod.cpp \
log_trace.cpp \
main.cpp \
missile.cpp \
object.cpp \
physics.cpp \
pilot_manager.cpp \
race_manager.cpp \
remote_sss_iface.cpp \
remote_sss_queue.cpp \
renderer.cpp \
robot_pilot.cpp \
sss.cpp \
sss_socket.cpp \
terrain_generator.cpp \
text_overlay.cpp \
texture.cpp \
thermal.cpp \
thermal_manager.cpp \
tracer.cpp \
tx_audio_input.cpp \
vario.cpp \
vertex.cpp \
wind_field_generator.cpp

# gets over-ridden on the real make
OBJDIR := .
OBJS := $(SRC:%.cpp=$(OBJDIR)/%.o)

#default target
opt: .opt
	@echo "############ Starting Optimised build #########################"
	$(MAKE) ../sss CFLAGS="$(OPT_FLAGS) $(EXTRA_FLAGS)" OBJDIR=.opt

debug: .debug
	@echo "############ Starting debug build ##############################"
	$(MAKE) ../sss_debug CFLAGS="$(DEBUG_FLAGS) $(EXTRA_FLAGS)" OBJDIR=.debug

gprof: .gprof
	@echo "############ Starting gprof build ##############################"
	$(MAKE) ../sss_gprof CFLAGS="-pg $(OPT_FLAGS) $(EXTRA_FLAGS)" OBJDIR=.gprof

../sss: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) 

../sss_debug: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) 

../sss_gprof: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) 

.debug:
	mkdir .debug

.opt:
	mkdir .opt

.gprof:
	mkdir .gprof

doc: FORCE
	doxygen sss.dox

bw_to_orog: bw_to_orog.cpp
	$(CC) -g -o $@ bw_to_orog.cpp

tiff_to_orog: tiff_to_orog.c
	$(CC) -g -o $@ tiff_to_orog.c -ltiff

FORCE:

clean: debug_clean opt_clean gprof_clean

debug_clean:
	\rm -f *~ .debug/* sss_debug

opt_clean:
	\rm -f *~ .opt/* sss

gprof_clean:
	\rm -f *~ .gprof/* sss_gprof

# How we get a .o from a .cpp
$(OBJDIR)/%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.d: %.cpp
	$(CC) -MM $(CFLAGS) $< | sed -e "s/\(^[a-zA-Z]\)/$(OBJDIR)\/\1/" -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

ifneq ($(OBJDIR),.)
-include $(OBJS:.o=.d)
endif




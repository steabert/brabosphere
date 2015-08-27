###########################
# Non-changeable entries  #
###########################
include(../brabosphere.pri)
release {
  TARGET = ../bin/crdview
  CONFIG += warn_off        
  MOC_DIR = ../output/crdview/moc/release
  OBJECTS_DIR = ../output/crdview/obj/release
  UI_DIR = ../output/crdview/ui/release
}
debug {
  TARGET = ../bin/crdview-debug
  CONFIG += warn_on
  MOC_DIR = ../output/crdview/moc/debug
  OBJECTS_DIR = ../output/crdview/obj/debug
  UI_DIR = ../output/crdview/ui/debug
}
###########################
# CrdView files           #
###########################
HEADERS += include/crdview.h \
           include/iconsets.h 
SOURCES += source/crdview.cpp \
           source/iconsets.cpp \
           source/main.cpp 
win32:RC_FILE = crdview.rc

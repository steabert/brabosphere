###########################
# User changeable entries #
###########################

# location of the QextMDI and OpenBabel directories (have to be absolute directories. 
# if you want to use relative directories, you'll have to prepend them by an extra '../')

  QEXTMDIDIR = home/username/qextmdi
  OPENBABELDIR = /usr/local

# choose between support for OpenBabel 1.100.2, OpenBabel 2.x as DLL, or no support

  #CONFIG += openbabel1
  CONFIG += openbabel2_dll

# choose between QextMDI and QextMDI as DLL

  #CONFIG += qextmdi
  CONFIG += qextmdi_dll

# choose between release or debug version

  CONFIG += release
  #CONFIG += debug

# have debug messages on the console? (change main.cpp if you want them in release builds)

  #CONFIG += console

# uncomment this line if building using Open Babel 2.x as a DLL and using the Visual C++ Toolkit 2003
# warning: this only works for release builds!

  #QMAKE_LIBS_WINDOWS -= libcmt.lib libcpmt.lib

###########################
# Non-changeable entries  #
###########################

CONFIG += qt thread opengl exceptions rtti
DEFINES += NO_KDE2
COMMONDIR = ../common
INCLUDEPATH += include $$COMMONDIR/include

###########################
# Common files            #
###########################
HEADERS += $$COMMONDIR/include/atomset.h \
           $$COMMONDIR/include/colorbutton.h \
           $$COMMONDIR/include/crdfactory.h \
           $$COMMONDIR/include/domutils.h \
           $$COMMONDIR/include/glbaseparameters.h \
           $$COMMONDIR/include/glmoleculeparameters.h \
           $$COMMONDIR/include/glsimplemoleculeview.h \
           $$COMMONDIR/include/glview.h \
           $$COMMONDIR/include/icons-shared.h \
           $$COMMONDIR/include/pixmaps.h \
           $$COMMONDIR/include/point3d.h \
           $$COMMONDIR/include/quaternion.h \
           $$COMMONDIR/include/vector3d.h \
           $$COMMONDIR/include/version.h
SOURCES += $$COMMONDIR/source/atomset.cpp \
           $$COMMONDIR/source/colorbutton.cpp \
           $$COMMONDIR/source/crdfactory.cpp \
           $$COMMONDIR/source/domutils.cpp \
           $$COMMONDIR/source/glsimplemoleculeview.cpp \
           $$COMMONDIR/source/glview.cpp \
           $$COMMONDIR/source/point3d.cpp \
           $$COMMONDIR/source/version.cpp
FORMS +=   $$COMMONDIR/ui/moleculepropertieswidget.ui \
           $$COMMONDIR/ui/textviewwidget.ui

###########################
# Open Babel support      #
###########################
openbabel1 {
  DEFINES += USE_OPENBABEL1
  win32:INCLUDEPATH += $$OPENBABELDIR/src
  unix:INCLUDEPATH += $$OPENBABELDIR/include
  LIBS += -L$$OPENBABELDIR/lib -lopenbabel
}
openbabel2 {
  DEFINES += USE_OPENBABEL2
  win32:INCLUDEPATH += $$OPENBABELDIR/src
  unix:INCLUDEPATH += $$OPENBABELDIR/include/openbabel-2.0
  LIBS += -L$$OPENBABELDIR/lib -lopenbabel
}
openbabel2_dll {
  DEFINES += USE_OPENBABEL2 USING_DYNAMIC_LIBS
  win32:INCLUDEPATH += $$OPENBABELDIR/src $$OPENBABELDIR/windows/VC8
  unix:INCLUDEPATH += $$OPENBABELDIR/include/openbabel-2.0
  win32:LIBS += -L$$OPENBABELDIR/lib -lopenbabel
  unix:LIBS += -L$$OPENBABELDIR/lib -lopenbabel
}

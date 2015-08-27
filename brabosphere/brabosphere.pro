###########################
# Non-changeable entries  #
###########################
include(../brabosphere.pri)
release {
  TARGET = ../bin/brabosphere 
  CONFIG += warn_off        
  MOC_DIR = ../output/brabosphere/moc/release
  OBJECTS_DIR = ../output/brabosphere/obj/release
  UI_DIR = ../output/brabosphere/ui/release
}
debug {
  TARGET = ../bin/brabosphere-debug
  CONFIG += warn_on
  MOC_DIR = ../output/brabosphere/moc/debug
  OBJECTS_DIR = ../output/brabosphere/obj/debug
  UI_DIR = ../output/brabosphere/ui/debug
}

###########################
# Brabosphere files       #
###########################
HEADERS += include/aboutbox.h \
           include/background.h \
           include/basisset.h \
           include/brabobase.h \
           include/calculation.h \
           include/densitybase.h \
           include/densityloadthread.h \
           include/glmoleculeview.h \
           include/globalbase.h \
           include/glorbitalview.h \
           include/icons.h \
           include/iconsets.h \
           include/isosurface.h \
           include/latin1validator.h \
           include/newatombase.h \
           include/orbitalthread.h \
           include/orbitalviewerbase.h \
           include/paths.h \
           include/plotmapbase.h \
           include/plotmaplabel.h \
           include/preferencesbase.h \
           include/relaxbase.h \
           include/splash.h \
           include/statustext.h \
           include/utils.h \
           include/xbrabo.h \
           include/xbraboview.h
SOURCES += source/aboutbox.cpp \
           source/basisset.cpp \
           source/brabobase.cpp \
           source/calculation.cpp \
           source/densitybase.cpp \
           source/densityloadthread.cpp \
           source/glmoleculeview.cpp \
           source/globalbase.cpp \
           source/glorbitalview.cpp \
           source/iconsets.cpp \
           source/isosurface.cpp \
           source/latin1validator.cpp \
           source/main.cpp \
           source/newatombase.cpp \
           source/orbitalthread.cpp \
           source/orbitalviewerbase.cpp \
           source/paths.cpp \
           source/plotmapbase.cpp \
           source/plotmaplabel.cpp \
           source/preferencesbase.cpp \
           source/relaxbase.cpp \
           source/statustext.cpp \
           source/utils.cpp \
           source/xbrabo.cpp \
           source/xbraboview.cpp
FORMS +=   ui/brabowidget.ui \
           ui/coordinateswidget.ui \
           ui/densitywidget.ui \
           ui/globalwidget.ui \
           ui/newatomwidget.ui \
           ui/outputchooserwidget.ui \
           ui/orbitaloptionswidget.ui \
           ui/plotmapwidget.ui \
           ui/plotmapextensionwidget.ui \
           ui/preferenceswidget.ui \
           ui/relaxwidget.ui 
win32:RC_FILE = brabosphere.rc

###########################
# QextMdi support         #
###########################
qextmdi_dll {
  DEFINES += USE_QEXTMDI_DLL
  INCLUDEPATH += $$QEXTMDIDIR/include
  LIBS += -L$$QEXTMDIDIR/lib -lqextmdi
}
qextmdi {
  DEFINES += USE_QEXTMDI
  INCLUDEPATH += $$QEXTMDIDIR/include
}

###########################
# KMdi support            #
###########################
kmdi {
  INCLUDEPATH += kmdi kmdi/res
  DEFINES += NO_INCLUDE_MOCFILES NO_MDI_DLL USE_KMDI
  HEADERS += kmdi/dummykmainwindow.h \
             kmdi/dummykpartsdockmainwindow.h \
             kmdi/dummyktoolbar.h \
             kmdi/kdockwidget.h \
             kmdi/kdockwidget_p.h \
             kmdi/kdockwidget_private.h \
             kmdi/kmdichildarea.h \
             kmdi/kmdichildfrm.h \
             kmdi/kmdichildfrmcaption.h \
             kmdi/kmdichildview.h \
             kmdi/kmdidefines.h \
             kmdi/kmdidockcontainer.h \
             kmdi/kmdidocumentviewtabwidget.h \
             kmdi/kmdifocuslist.h \
             kmdi/kmdiiterator.h \
             kmdi/kmdilistiterator.h \
             kmdi/kmdimainfrm.h \
             kmdi/kmdinulliterator.h \
             kmdi/kmditaskbar.h \
             kmdi/kmditoolviewaccessor.h \
             kmdi/kmditoolviewaccessor_p.h \
             kmdi/kmultitabbar.h \
             kmdi/kmultitabbar_p.h
  SOURCES += kmdi/kdockwidget.cpp \
             kmdi/kdockwidget_private.cpp \
             kmdi/kmdichildarea.cpp \
             kmdi/kmdichildfrm.cpp \
             kmdi/kmdichildfrmcaption.cpp \
             kmdi/kmdichildview.cpp \
             kmdi/kmdidockcontainer.cpp \
             kmdi/kmdidocumentviewtabwidget.cpp \
             kmdi/kmdifocuslist.cpp \
             kmdi/kmdimainfrm.cpp \
             kmdi/kmditaskbar.cpp \
             kmdi/kmditoolviewaccessor.cpp \
             kmdi/kmultitabbar.cpp
}
kdmi_dll {
  DEFINES += NO_INCLUDE_MOCFILES USE_KMDI_DLL
  INCLUDEPATH += $$KMDIDIR
  LIBS += -L$$KMDIDIR -lkmdi
}

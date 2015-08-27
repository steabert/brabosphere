# Locations of the iconv and libxml2 libraries
ICONV_DIR = ..\iconv-1.9.1.win32
LIBXML2_DIR = ..\libxml2-2.6.23.win32

# No changes needed here (at least not for version 2.0.1)
TEMPLATE = lib
DEFINES += INCHI_LINK_AS_DLL HAVE_CONFIG_H OBDLL_EXPORTS OBCONV_EXPORTS USING_DYNAMIC_LIBS
DEFINES -= UNICODE
CONFIG += dll release exceptions rtti thread
CONFIG -= qt
INCLUDEPATH += data src src\formats src\formats\xml windows windows\VC8
INCLUDEPATH += $$LIBXML2_DIR\include
INCLUDEPATH += $$ICONV_DIR\include
OBJECTS_DIR = obj
LIBS += -Lwindows -lzdll -llibinchi
LIBS += -L$$LIBXML2_DIR\lib -llibxml2 -llibxml2_a
LIBS += -L$$ICONV_DIR\lib -liconv -liconv_a
DESTDIR = lib

SOURCES += src\atom.cpp \
           src\base.cpp \
           src\bitvec.cpp \
           src\bond.cpp \
           src\bondtyper.cpp \
           src\chains.cpp \
           src\chiral.cpp \
           src\data.cpp \
           src\dlhandler_win32.cpp \
           src\fingerprint.cpp \
           src\generic.cpp \
           src\grid.cpp \
           src\kekulize.cpp \
           src\matrix.cpp \
           src\mol.cpp \
           src\molchrg.cpp \
           src\obconversion.cpp \
           src\oberror.cpp \
           src\obiter.cpp \
           src\obutil.cpp \
           src\parsmart.cpp \
           src\patty.cpp \
           src\phmodel.cpp \
           src\rand.cpp \
           src\residue.cpp \
           src\ring.cpp \
           src\rotamer.cpp \
           src\rotor.cpp \
           src\tokenst.cpp \
           src\transform.cpp \
           src\typer.cpp \
           src\fingerprints\finger2.cpp \
           src\fingerprints\finger3.cpp \
           src\formats\alchemyformat.cpp \
           src\formats\amberformat.cpp \
           src\formats\APIInterface.cpp \
           src\formats\balstformat.cpp \
           src\formats\bgfformat.cpp \
           src\formats\boxformat.cpp \
           src\formats\cacaoformat.cpp \
           src\formats\cacheformat.cpp \
           src\formats\carformat.cpp \
           src\formats\cccformat.cpp \
           src\formats\chem3dformat.cpp \
           src\formats\chemdrawformat.cpp \
           src\formats\chemtoolformat.cpp \
           src\formats\copyformat.cpp \
           src\formats\crkformat.cpp \
           src\formats\CSRformat.cpp \
           src\formats\cssrformat.cpp \
           src\formats\dmolformat.cpp \
           src\formats\fastsearchformat.cpp \
           src\formats\featformat.cpp \
           src\formats\fhformat.cpp \
           src\formats\fingerprintformat.cpp \
           src\formats\freefracformat.cpp \
           src\formats\gamessformat.cpp \
           src\formats\gaussformat.cpp \
           src\formats\ghemicalformat.cpp \
           src\formats\gromos96format.cpp \
           src\formats\hinformat.cpp \
           src\formats\inchiformat.cpp \
           src\formats\jaguarformat.cpp \
           src\formats\mdlformat.cpp \
           src\formats\mmodformat.cpp \
           src\formats\mol2format.cpp \
           src\formats\mopacformat.cpp \
           src\formats\mpdformat.cpp \
           src\formats\mpqcformat.cpp \
           src\formats\nwchemformat.cpp \
           src\formats\pcmodelformat.cpp \
           src\formats\pdbformat.cpp \
           src\formats\povrayformat.cpp \
           src\formats\PQSformat.cpp \
           src\formats\qchemformat.cpp \
           src\formats\reportformat.cpp \
           src\formats\rxnformat.cpp \
           src\formats\shelxformat.cpp \
           src\formats\smilesformat.cpp \
           src\formats\tinkerformat.cpp \
           src\formats\turbomoleformat.cpp \
           src\formats\unichemformat.cpp \
           src\formats\viewmolformat.cpp \
           src\formats\xedformat.cpp \
           src\formats\xyzformat.cpp \
           src\formats\yasaraformat.cpp \
           src\formats\zindoformat.cpp \
           src\math\matrix3x3.cpp \
           src\math\vector3.cpp \
           src\formats\xml\cmlreactlformat.cpp \
           src\formats\xml\pubchem.cpp \
           src\formats\xml\xcmlformat.cpp \
           src\formats\xml\xml.cpp \
           src\formats\xml\xmlformat.cpp
